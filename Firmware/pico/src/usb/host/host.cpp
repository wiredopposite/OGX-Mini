#include "board_config.h"
#if USBH_ENABLED

#include <cstring>
#include <atomic>
#include <memory>
#include <hardware/gpio.h>
#include <pico/mutex.h>
#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "pio_usb.h"

#include "log/log.h"
#include "usb/host/host.h"
#include "gamepad/gamepad.h"
#include "usb/host/host_private.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/host/tusb_host/tuh_xaudio.h"
#include "usb/host/tusb_host/tusb_config.h"

#define USBH_INTERFACES_MAX 8U /* Enough for Xbox360 Wireless */

static const usb_host_driver_t* USBH_DRIVERS[USBH_TYPE_COUNT] = {
    [USBH_TYPE_NONE]            = NULL,
    [USBH_TYPE_XID]             = &USBH_DRIVER_XBOXOG_GP,
    [USBH_TYPE_XBLC]            = NULL,
    [USBH_TYPE_XGIP]            = &USBH_DRIVER_XGIP_GP,
    [USBH_TYPE_XGIP_AUDIO]      = NULL,
    [USBH_TYPE_XGIP_CHATPAD]    = NULL,
    [USBH_TYPE_XINPUT]          = &USBH_DRIVER_XINPUT,
    [USBH_TYPE_XINPUT_AUDIO]    = NULL,
    [USBH_TYPE_XINPUT_CHATPAD]  = NULL,
    [USBH_TYPE_XINPUT_WL]       = &USBH_DRIVER_XINPUT_WL,
    [USBH_TYPE_XINPUT_WL_AUDIO] = NULL,
    [USBH_TYPE_HID_GENERIC]     = &USBH_DRIVER_HID,
    [USBH_TYPE_HID_SWITCH_PRO]  = &USBH_DRIVER_SWITCH_PRO,
    [USBH_TYPE_HID_SWITCH]      = &USBH_DRIVER_HID,
    [USBH_TYPE_HID_PSCLASSIC]   = &USBH_DRIVER_HID,
    [USBH_TYPE_HID_DINPUT]      = &USBH_DRIVER_HID,
    [USBH_TYPE_HID_PS3]         = &USBH_DRIVER_PS3,
    [USBH_TYPE_HID_PS4]         = &USBH_DRIVER_PS4,
    [USBH_TYPE_HID_PS5]         = &USBH_DRIVER_PS5,
    [USBH_TYPE_HID_N64]         = &USBH_DRIVER_HID,
};

class EmuInterface {
public:
    EmuInterface(usbh_type_t d_type, usbh_periph_t p_type, uint8_t dev_addr, uint8_t itf, uint8_t index, bool use_mutex) 
        :   dev_type(d_type), 
            periph_type(p_type),
            driver(USBH_DRIVERS[d_type]), 
            daddr(dev_addr), 
            itf_num(itf),
            gp_index(index) {
        if (use_mutex) {
            mutex_init(&mutex);
        }
    }
    ~EmuInterface() {
        if (mutex_is_initialized(&mutex)) {
            // Make sure no one is using this class
            mutex_enter_blocking(&mutex);
            mutex_exit(&mutex);
        }
    }

    volatile bool               disabled{false};
    // volatile bool               unmount_pending{false};
    uint8_t                     gp_index{0};
    usbh_type_t                 dev_type;
    usbh_periph_t               periph_type;
    const usb_host_driver_t*    driver;
    std::atomic<bool>           new_data{false};
    uint8_t                     daddr;
    uint8_t                     itf_num;
    uint8_t                     state_buffer[USBH_STATE_BUFFER_SIZE] __attribute__((aligned(4)));

    template <typename T>
    inline void get_data(T* buffer) {
        if (mutex_is_initialized(&mutex)) {
            mutex_enter_blocking(&mutex);
            std::memcpy(buffer, this->buffer, sizeof(T));
            new_data.store(false, std::memory_order_release);
            mutex_exit(&mutex);
        } else {
            std::memcpy(buffer, this->buffer, sizeof(T));
            new_data.store(false, std::memory_order_relaxed);
        }
    }

    template <typename T>
    inline void set_data(T* buffer) {
        if (mutex_is_initialized(&mutex)) {
            mutex_enter_blocking(&mutex);
            std::memcpy(this->buffer, buffer, sizeof(T));
            new_data.store(true, std::memory_order_release);
            mutex_exit(&mutex);
        } else {
            std::memcpy(this->buffer, buffer, sizeof(T));
            new_data.store(true, std::memory_order_relaxed);
        }
    }

private:
    mutex_t mutex;
    uint8_t buffer[MAX(sizeof(gamepad_pad_t), sizeof(gamepad_pcm_out_t))] __attribute__((aligned(4)));
};

typedef struct {
    volatile bool                   use_mutex{false};
    // volatile bool                   unmount_pending{false};
    usbh_hw_type_t                  hw_type{USBH_HW_PIO};
    usb_host_connect_cb_t           connect_cb{nullptr};
    usb_host_gamepad_cb_t           gamepad_cb{nullptr};
    usb_host_audio_cb_t             audio_cb{nullptr};
    std::unique_ptr<EmuInterface>   tuh_itf[CFG_TUH_DEVICE_MAX][USBH_INTERFACES_MAX]{nullptr};
    std::unique_ptr<EmuInterface>*  gp_itf_lookup[GAMEPADS_MAX][PERIPH_COUNT]{nullptr};
} usbh_system_t;

static usbh_system_t  usbh_system;
static const uint16_t usbh_sys_size = sizeof(usbh_system);

static void vcc_set_enabled(bool enabled) {
#if USBH_VCC_ENABLED
    if (enabled) {
        gpio_init(USBH_VCC_ENABLE_PIN);
        gpio_set_dir(USBH_VCC_ENABLE_PIN, GPIO_OUT);
        gpio_put(USBH_VCC_ENABLE_PIN, true);
    } else {
        gpio_put(USBH_VCC_ENABLE_PIN, false);
        gpio_deinit(USBH_VCC_ENABLE_PIN);
    }
#else
    (void)enabled;
#endif
}

static bool assign_emu_itf(uint8_t daddr, uint8_t itf_num, usbh_type_t dev_type, 
                           uint8_t gp_index, usbh_periph_t periph_type) {
    if (usbh_system.gp_itf_lookup[gp_index][periph_type] != nullptr) {
        ogxm_loge("Interface %d on device %d is already assigned\n", itf_num, daddr);
        return false;
    }
    usbh_system.tuh_itf[daddr - 1][itf_num] = 
        std::make_unique<EmuInterface>(
            dev_type, 
            periph_type,
            daddr, 
            itf_num, 
            gp_index, 
            usbh_system.use_mutex
        );
    if (usbh_system.tuh_itf[daddr - 1][itf_num] == nullptr) {
        ogxm_loge("Failed to allocate memory for interface %d on device %d\n", itf_num, daddr);
        return false;
    }
    usbh_system.gp_itf_lookup[gp_index][periph_type] = 
        &usbh_system.tuh_itf[daddr - 1][itf_num];
    return true;
}

static bool assign_emu_device(uint8_t daddr, uint8_t itf_num, usbh_type_t dev_type) {
    /* Combine gamepad, headset, and chatpad into a single emulated device */
    if (usbh_system.tuh_itf[daddr - 1][itf_num] != nullptr) {
        return false;
    }
    
    usbh_periph_t periph_type = PERIPH_COUNT;

    switch (dev_type) {
    case USBH_TYPE_XGIP_CHATPAD:
    case USBH_TYPE_XINPUT_CHATPAD:
        periph_type = PERIPH_CHATPAD;
        break;
    case USBH_TYPE_XBLC:
    case USBH_TYPE_XGIP_AUDIO:
    case USBH_TYPE_XINPUT_AUDIO:
    case USBH_TYPE_XINPUT_WL_AUDIO:
        periph_type = PERIPH_AUDIO;
        break;
    case USBH_TYPE_NONE:
    case USBH_TYPE_COUNT:
        ogxm_loge("Invalid device type %d for interface %d on device %d\n", 
            dev_type, itf_num, daddr);
        return false;
    default:
        periph_type = PERIPH_GAMEPAD;
        break;    
    }
    switch (dev_type) {   
    case USBH_TYPE_XINPUT_WL:
    case USBH_TYPE_XINPUT_WL_AUDIO:
        /*  Tinyusb mounts interfaces backwards, if itf_num/2 >= GAMEPADS_MAX
            and we assign it a gamepad, there will be no more room 
            left for the first gamepad. Xinput wireless interfaces are 
            itf0=gamepad1, itf1=audio1, itf3=gamepad2, itf4=audio2, etc. */
        if (dev_type == USBH_TYPE_XINPUT_WL) {
            if ((itf_num / 2) >= GAMEPADS_MAX) {
                return false;
            }
        } else if (dev_type == USBH_TYPE_XINPUT_WL_AUDIO) {
            if (((itf_num - 1) / 2) >= GAMEPADS_MAX) {
                return false;
            }
        }
    case USBH_TYPE_XGIP:
    case USBH_TYPE_XGIP_CHATPAD:
    case USBH_TYPE_XGIP_AUDIO:
    case USBH_TYPE_XINPUT:
    case USBH_TYPE_XINPUT_AUDIO:
    case USBH_TYPE_XINPUT_CHATPAD:
        /* Try to assign these to an existing device */
        for (uint8_t j = 0; j < USBH_INTERFACES_MAX; j++) {
            if (usbh_system.tuh_itf[daddr - 1][j] == nullptr) {
                continue;
            } else {
                uint8_t gp_idx = usbh_system.tuh_itf[daddr - 1][j]->gp_index;
                return assign_emu_itf(daddr, itf_num, dev_type, gp_idx, periph_type);
            }
        }
        break;
    case USBH_TYPE_XBLC:
    case USBH_TYPE_XID:
        /* Try to pair Xbox OG xblc and gamepad together */
        {
        usbh_periph_t periph_target = (periph_type == PERIPH_GAMEPAD) 
                                      ? PERIPH_AUDIO : PERIPH_GAMEPAD;
        usbh_type_t dev_target      = (dev_type == USBH_TYPE_XID) 
                                      ? USBH_TYPE_XBLC : USBH_TYPE_XID;
        for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
            if (usbh_system.gp_itf_lookup[i][periph_target] == nullptr) {
                continue;
            } else if (usbh_system.gp_itf_lookup[i][periph_target]->get()->dev_type == dev_target) {
                uint8_t gp_idx = usbh_system.gp_itf_lookup[i][periph_target]->get()->gp_index;
                return assign_emu_itf(daddr, itf_num, dev_type, gp_idx, periph_type);
            }
        }
        }
        break;
    default:
        break;
    }
    /* Just find a free slot */
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        if (usbh_system.tuh_itf[i][periph_type] != nullptr) {
            continue;
        } else {
            return assign_emu_itf(daddr, itf_num, dev_type, i, periph_type);
        }
    }
    return false;
}

/* ---- TUH HXX Callbacks ---- */

void tuh_hxx_mounted_cb(usbh_type_t type, uint8_t daddr, uint8_t itf_num, const uint8_t* desc_report, uint16_t desc_len) {
    if (!assign_emu_device(daddr, itf_num, type)) {
        ogxm_loge("Failed to assign emu device for daddr %d, itf %d\n", daddr, itf_num);
        return;
    }
    EmuInterface* emu_itf = usbh_system.tuh_itf[daddr - 1][itf_num].get();
    const usb_host_driver_t* driver = emu_itf->driver;
    if (driver == NULL) {
        ogxm_loge("No driver found for daddr %d, itf %d\n", daddr, itf_num);
        return;
    }
    ogxm_logi("Initializing host driver: %s for daddr %d, itf %d\n", 
        driver->name, daddr, itf_num);

    driver->mounted_cb(type, emu_itf->gp_index, daddr, itf_num, 
                       desc_report, desc_len, emu_itf->state_buffer);
}

void tuh_hxx_unmounted_cb(uint8_t daddr, uint8_t itf_num) {
    EmuInterface* emu_itf = usbh_system.tuh_itf[daddr - 1][itf_num].get();
    const usb_host_driver_t* driver = emu_itf->driver;

    ogxm_logi("Unmounting host driver: %s, daddr %d, itf %d\n", 
        (driver) ? driver->name : "Unknown", daddr, itf_num);

    if (driver && driver->unmounted_cb) {
        driver->unmounted_cb(emu_itf->gp_index, daddr, itf_num);
    }
    if (usbh_system.connect_cb) {
        usbh_system.connect_cb(emu_itf->gp_index, emu_itf->dev_type, false);
    }
    // if (usbh_system.use_mutex) {
    //     usbh_system.unmount_pending = true;
    //     emu_itf->unmount_pending = true;
    // } else {
        uint8_t gp_index = emu_itf->gp_index;
        usbh_periph_t periph_type = emu_itf->periph_type;
        usbh_system.tuh_itf[daddr - 1][itf_num].reset();
        usbh_system.gp_itf_lookup[gp_index][periph_type] = nullptr;
    // }
}

void tuh_hxx_report_received_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len) {
    EmuInterface* emu_itf = usbh_system.tuh_itf[daddr - 1][itf_num].get();
    if (emu_itf == nullptr) {
        ogxm_loge("Interface %d on device %d is not assigned\n", itf_num, daddr);
        return;
    }
    const usb_host_driver_t* driver = emu_itf->driver;
    if (!emu_itf->disabled && driver && driver->report_cb) {
        driver->report_cb(emu_itf->gp_index, emu_itf->periph_type, daddr, itf_num, data, len);
    }
}

const usbh_class_driver_t* usbh_app_driver_get_cb(uint8_t* driver_count) {
    static const usbh_class_driver_t TUH_DRIVER_HXX = {
        .name       = "HXX",
        .init       = hxx_init_cb,
        .deinit     = hxx_deinit_cb,
        .open       = hxx_open_cb,
        .set_config = hxx_set_config_cb,
        .xfer_cb    = hxx_ep_xfer_cb,
        .close      = hxx_close_cb
    };
    *driver_count = 1;
    return &TUH_DRIVER_HXX;
}

/* ---- Private host driver callbacks ---- */

void usb_host_driver_connect_cb(uint8_t index, usbh_type_t type, bool connected) {
    if (usbh_system.connect_cb) {
        usbh_system.connect_cb(index, type, connected);
    }
}

void usb_host_driver_pad_cb(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    if (usbh_system.gamepad_cb) {
        usbh_system.gamepad_cb(index, pad, flags);
    }
}

/* ---- Public ---- */

void usb_host_configure(const usb_host_config_t* config) {
    usbh_system.connect_cb = config->connect_cb;
    usbh_system.gamepad_cb = config->gamepad_cb;
    usbh_system.audio_cb = config->audio_cb;
    usbh_system.use_mutex = config->use_mutex;
    usbh_system.hw_type = config->hw_type;
    vcc_set_enabled(false);
}

void usb_host_enable(void) {
    vcc_set_enabled(true);
    
    if (usbh_system.hw_type == USBH_HW_PIO) {
#if USBH_PIO_ENABLED
        pio_usb_configuration_t pio_cfg = {
            USBH_PIO_DP_PIN,
            PIO_USB_TX_DEFAULT,
            PIO_SM_USB_TX_DEFAULT,
            PIO_USB_DMA_TX_DEFAULT,
            PIO_USB_RX_DEFAULT,
            PIO_SM_USB_RX_DEFAULT,
            PIO_SM_USB_EOP_DEFAULT,
            NULL,
            PIO_USB_DEBUG_PIN_NONE,
            PIO_USB_DEBUG_PIN_NONE,
            false,
            PIO_USB_PINOUT_DPDM
        };
        tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
        tuh_init(BOARD_TUH_RHPORT);
#else
        panic("PIO USB is not enabled in the configuration.\n");
#endif
    } else {
        tusb_rhport_init_t host_init = {
            .role = TUSB_ROLE_HOST,
            .speed = TUSB_SPEED_AUTO
        };
        tusb_init(BOARD_TUH_RHPORT, &host_init);
    }
}

void usb_host_set_device_enabled(uint8_t index, bool enabled) {
    if (index >= GAMEPADS_MAX) {
        return;
    }
    for (uint8_t i = 0; i < PERIPH_COUNT; i++) {
        EmuInterface* emu_itf = usbh_system.tuh_itf[index][i].get();
        if (emu_itf == nullptr) {
            continue;
        }
        emu_itf->disabled = !enabled;
        if (enabled) {
            if (i != PERIPH_AUDIO) {
                tuh_hxx_receive_report(emu_itf->daddr, emu_itf->itf_num);
            } else {
                tuh_xaudio_receive_data(emu_itf->daddr, emu_itf->itf_num);
                // tuh_xaudio_receive_data_ctrl(emu_itf->daddr, emu_itf->itf_num_ctrl);
            }
        }
    }
}

void usb_host_set_rumble(uint8_t index, const gamepad_rumble_t* rumble) {
    if ((index >= GAMEPADS_MAX) ||
        (usbh_system.gp_itf_lookup[index][PERIPH_GAMEPAD] == nullptr)) {
        return;
    }
    EmuInterface* emu_itf = usbh_system.gp_itf_lookup[index][PERIPH_GAMEPAD]->get();
    if (usbh_system.use_mutex) {
        emu_itf->set_data(rumble);
    } else {
        const usb_host_driver_t* driver = emu_itf->driver;
        if (driver && driver->send_rumble) {
            driver->send_rumble(index, emu_itf->daddr, emu_itf->itf_num, rumble);
        }
    }
}

void usb_host_set_audio(uint8_t index, const gamepad_pcm_out_t* pcm) {
    if ((index >= GAMEPADS_MAX) ||
        (usbh_system.gp_itf_lookup[index][PERIPH_AUDIO] == nullptr)) {
        return;
    }
    EmuInterface* emu_itf = usbh_system.gp_itf_lookup[index][PERIPH_AUDIO]->get();
    if (usbh_system.use_mutex) {
        emu_itf->set_data(pcm);
    } else {
        const usb_host_driver_t* driver = emu_itf->driver;
        if (driver && driver->send_audio) {
            driver->send_audio(index, emu_itf->daddr, emu_itf->itf_num, pcm);
        }
    }
}

void usb_host_task(void) {
    // if (usbh_system.unmount_pending) {
    //     usbh_system.unmount_pending = false;
    //     for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
    //         for (uint8_t j = 0; j < PERIPH_COUNT; j++) {
    //             if (usbh_system.gp_itf_lookup[i][j] != nullptr) {
    //                 EmuInterface* emu_itf = usbh_system.gp_itf_lookup[i][j]->get();
    //                 if (emu_itf->unmount_pending) {
    //                     emu_itf->unmount_pending = false;
    //                     usbh_system.tuh_itf[emu_itf->daddr - 1][emu_itf->itf_num].reset();
    //                     usbh_system.gp_itf_lookup[i][j] = nullptr;
    //                 }
    //             }
    //         }
    //     }
    // }
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        if (usbh_system.use_mutex) {
            if (usbh_system.gp_itf_lookup[i][PERIPH_GAMEPAD] != nullptr) {
                EmuInterface* emu_itf = usbh_system.gp_itf_lookup[i][PERIPH_GAMEPAD]->get();
                if (!emu_itf->disabled &&
                    (emu_itf->driver->send_rumble != nullptr) &&
                    emu_itf->new_data.load(std::memory_order_acquire)) {
                    gamepad_rumble_t rumble;
                    emu_itf->get_data(&rumble);
                    emu_itf->driver->send_rumble(i, emu_itf->daddr, emu_itf->itf_num, &rumble);
                }
            }
            if (usbh_system.gp_itf_lookup[i][PERIPH_AUDIO] != nullptr) {
                EmuInterface* emu_itf = usbh_system.gp_itf_lookup[i][PERIPH_AUDIO]->get();
                if (!emu_itf->disabled &&
                    (emu_itf->driver->send_audio != nullptr) &&
                    emu_itf->new_data.load(std::memory_order_acquire)) {
                    gamepad_pcm_out_t pcm;
                    emu_itf->get_data(&pcm);
                    emu_itf->driver->send_audio(i, emu_itf->daddr, emu_itf->itf_num, &pcm);
                }
            }
        }
        for (uint8_t j = 0; j < PERIPH_COUNT; j++) {
            if (usbh_system.gp_itf_lookup[i][j] != nullptr) {
                EmuInterface* emu_itf = usbh_system.gp_itf_lookup[i][j]->get();
                if (emu_itf->disabled) {
                    continue;
                }
                const usb_host_driver_t* driver = emu_itf->driver;
                if (driver && driver->task_cb) {
                    driver->task_cb(emu_itf->gp_index, emu_itf->daddr, emu_itf->itf_num);
                }
            }
        }
    }
    tuh_task();
}

#endif // USBH_ENABLED