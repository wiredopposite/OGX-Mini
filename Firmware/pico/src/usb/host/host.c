#include "usb/host/host.h"
#if USBH_ENABLED

#include <stdatomic.h>
#include <hardware/gpio.h>
#include <pico/mutex.h>
#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "pio_usb.h"

#include "log/log.h"
#include "board_config.h"
#include "gamepad/gamepad.h"
#include "usb/host/host_private.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/host/tusb_host/tuh_xaudio.h"
#include "usb/host/tusb_host/tusb_config.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define USBH_INTERFACES_MAX 8U

#define PIO_USB_CONFIG { \
    USBH_PIO_DP_PIN, \
    PIO_USB_TX_DEFAULT, \
    PIO_SM_USB_TX_DEFAULT, \
    PIO_USB_DMA_TX_DEFAULT, \
    PIO_USB_RX_DEFAULT, \
    PIO_SM_USB_RX_DEFAULT, \
    PIO_SM_USB_EOP_DEFAULT, \
    NULL, \
    PIO_USB_DEBUG_PIN_NONE, \
    PIO_USB_DEBUG_PIN_NONE, \
    false, \
    PIO_USB_PINOUT_DPDM \
}

typedef enum {
    GP_DATA_SAFE = 0,
    GP_DATA_UNSAFE,
    GP_DATA_COUNT
} gp_data_type_t;

typedef struct {
    volatile bool   alloc;
    atomic_bool     new_data;
    uint8_t         daddr;
    uint8_t         itf_num;
    uint8_t         itf_num_ctrl;
    usbh_type_t     dev_type;
    uint8_t         buffer[GP_DATA_COUNT][32U] __attribute__((aligned(4)));
    uint8_t         state_buffer[USBH_STATE_BUFFER_SIZE] __attribute__((aligned(4)));
} emu_itf_t;

typedef struct {
    mutex_t             mutex;
    // gamepad_rumble_t    rumble[GP_DATA_COUNT];
    // gamepad_pcm_out_t   pcm[GP_DATA_COUNT];
    emu_itf_t           emu_itf[PERIPH_COUNT];
} emu_dev_t;

typedef struct {
    bool                        mounted;
    uint8_t                     gp_index;
    usbh_type_t                 dev_type;
    usbh_periph_t               periph_type;
    emu_itf_t*                  emu_itf;
    const usb_host_driver_t*    driver;
} true_itf_t;

typedef struct {
    volatile bool   disabled;
    true_itf_t      interfaces[USBH_INTERFACES_MAX];
} true_dev_t;

typedef struct {
    bool                    use_mutex;
    usbh_hw_type_t          hw_type;
    usb_host_connect_cb_t   connect_cb;
    usb_host_gamepad_cb_t   gamepad_cb;
    usb_host_audio_cb_t     audio_cb;
    true_dev_t              tru_devices[CFG_TUH_DEVICE_MAX];
    emu_dev_t               emu_devices[GAMEPADS_MAX];
} usbh_system_t;

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

static usbh_system_t  usbh_system;
static const uint16_t usbh_sys_size = sizeof(usbh_system);

static inline true_itf_t* get_true_itf(uint8_t daddr, uint8_t itf_num) {
    if (daddr == 0 || daddr > CFG_TUH_DEVICE_MAX) {
        return NULL;
    }
    return &usbh_system.tru_devices[daddr - 1].interfaces[itf_num];
}

static inline emu_itf_t* get_emu_itf(uint8_t gp_index, usbh_periph_t periph) {
    if (gp_index >= GAMEPADS_MAX || periph >= PERIPH_COUNT) {
        return NULL;
    }
    return &usbh_system.emu_devices[gp_index].emu_itf[periph];
}

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

static void assign_emu_itf(uint8_t daddr, uint8_t itf_num, usbh_type_t dtype, usbh_periph_t ptype, uint8_t gp_index) {
    emu_dev_t* emu_dev = &usbh_system.emu_devices[gp_index];
    true_itf_t* itf = &usbh_system.tru_devices[daddr - 1].interfaces[itf_num];

    itf->dev_type = dtype;
    itf->periph_type = ptype;
    itf->emu_itf = &emu_dev->emu_itf[ptype];
    itf->gp_index = gp_index;
    itf->driver = USBH_DRIVERS[dtype];

    emu_dev->emu_itf[ptype].dev_type = dtype;
    emu_dev->emu_itf[ptype].alloc = true;
    emu_dev->emu_itf[ptype].daddr = daddr;
    emu_dev->emu_itf[ptype].itf_num = itf_num;
}

static bool assign_emu_device(uint8_t daddr, uint8_t itf_num, usbh_type_t type) {
    /* Combine gamepad, headset, and chatpad into a single emulated device */
    true_dev_t* dev = &usbh_system.tru_devices[daddr - 1];
    true_itf_t* itf = &dev->interfaces[itf_num];
    if (itf->mounted) {
        ogxm_logi("Interface %d on device %d is already mounted\n", itf_num, daddr);
        return false;
    }
    
    usbh_periph_t periph_type = PERIPH_COUNT;

    switch (type) {
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
            type, itf_num, daddr);
        return false;
    default:
        periph_type = PERIPH_GAMEPAD;
        break;
    }

    switch (type) {
    case USBH_TYPE_XINPUT_WL:
    case USBH_TYPE_XINPUT_WL_AUDIO:
        /*  Tinyusb mounts interfaces backwards, if itf_num/2 >= GAMEPADS_MAX
            and we assign it a gamepad, there will be no more room 
            left for the first gamepad. Xinput wireless interfaces are 
            itf0=gamepad1, itf1=audio1, itf3=gamepad2, itf4=audio2, etc. */
        if (type == USBH_TYPE_XINPUT_WL) {
            if ((itf_num / 2) >= GAMEPADS_MAX) {
                return false;
            }
        } else if (type == USBH_TYPE_XINPUT_WL_AUDIO) {
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
        for (uint8_t i = 0; i < ARRAY_SIZE(usbh_system.emu_devices); i++) {
            for (uint8_t j = 0; j < PERIPH_COUNT; j++) {
                emu_dev_t* emu_dev = &usbh_system.emu_devices[i];
                if ((emu_dev->emu_itf[j].daddr == daddr) &&
                    (!emu_dev->emu_itf[periph_type].alloc)) {
                    assign_emu_itf(daddr, itf_num, type, periph_type, i);
                    return true;
                }
            }
        }
        break;
    case USBH_TYPE_XBLC:
    case USBH_TYPE_XID:
        /* Try to pair Xbox OG xblc and gamepad together */
        {
        usbh_periph_t periph_target = (periph_type == PERIPH_GAMEPAD) 
                                      ? PERIPH_AUDIO : PERIPH_GAMEPAD;
        usbh_type_t type_target     = (type == USBH_TYPE_XID) 
                                      ? USBH_TYPE_XBLC : USBH_TYPE_XID;
        for (uint8_t i = 0; i < ARRAY_SIZE(usbh_system.emu_devices); i++) {
            emu_dev_t* emu_dev = &usbh_system.emu_devices[i];
            if (emu_dev->emu_itf[periph_target].alloc &&
                (emu_dev->emu_itf[periph_target].dev_type == type_target) &&
                !emu_dev->emu_itf[periph_type].alloc) {
                assign_emu_itf(daddr, itf_num, type, periph_type, i);
                return true;
            }
        }
        }
        break;
    default:
        break;
    }
    /* Just find a free slot */
    for (uint8_t i = 0; i < ARRAY_SIZE(usbh_system.emu_devices); i++) {
        emu_dev_t* emu_dev = &usbh_system.emu_devices[i];
        if (!emu_dev->emu_itf[periph_type].alloc) {
            assign_emu_itf(daddr, itf_num, type, periph_type, i);
            return true;
        }
    }
    return false;
}

/* ---- TUH HXX Callbacks ---- */

void tuh_hxx_mounted_cb(usbh_type_t type, uint8_t daddr, uint8_t itf_num, const uint8_t* desc_report, uint16_t desc_len) {
    true_dev_t* dev = &usbh_system.tru_devices[daddr - 1];
    true_itf_t* itf = &dev->interfaces[itf_num];
    if (!assign_emu_device(daddr, itf_num, type)) {
        ogxm_loge("Failed to assign emu device for daddr %d, itf %d\n", daddr, itf_num);
        return;
    }
    itf->mounted = true;

    const usb_host_driver_t* driver = itf->driver;
    if (driver == NULL) {
        ogxm_loge("No driver found for daddr %d, itf %d\n", daddr, itf_num);
        return;
    }
    ogxm_logi("Initializing host driver: %s for daddr %d, itf %d\n", 
        driver->name, daddr, itf_num);

    driver->mounted_cb(type, itf->gp_index, daddr, itf_num, desc_report, desc_len, itf->emu_itf->state_buffer);
}

void tuh_hxx_unmounted_cb(uint8_t daddr, uint8_t itf_num) {
    true_itf_t* itf = &usbh_system.tru_devices[daddr - 1].interfaces[itf_num];
    const usb_host_driver_t* driver = itf->driver;

    ogxm_logi("Unmounting host driver: %s, daddr %d, itf %d\n", 
        (driver) ? driver->name : "Unknown", daddr, itf_num);

    if (driver && driver->unmounted_cb) {
        driver->unmounted_cb(itf->gp_index, daddr, itf_num);
    }
    if (usbh_system.connect_cb) {
        usbh_system.connect_cb(itf->gp_index, itf->emu_itf->dev_type, false);
    }
    memset(itf->emu_itf, 0, sizeof(emu_itf_t));
    memset(itf, 0, sizeof(true_itf_t));
}

void tuh_hxx_report_received_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len) {
    if (usbh_system.tru_devices[daddr - 1].disabled) {
        return;
    }
    true_itf_t* itf = &usbh_system.tru_devices[daddr - 1].interfaces[itf_num];
    const usb_host_driver_t* driver = itf->driver;
    if (driver && driver->report_cb) {
        driver->report_cb(itf->gp_index, itf->periph_type, daddr, itf_num, data, len);
    }
}

void tuh_hxx_report_ctrl_received_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len) {
    if (usbh_system.tru_devices[daddr - 1].disabled) {
        return;
    }
    true_itf_t* itf = &usbh_system.tru_devices[daddr - 1].interfaces[itf_num];
    const usb_host_driver_t* driver = itf->driver;
    if (driver && driver->report_ctrl_cb) {
        driver->report_ctrl_cb(itf->gp_index, daddr, itf_num, data, len);
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
    if (usbh_system.use_mutex) {
        for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
            emu_dev_t* emu_dev = &usbh_system.emu_devices[i];
            mutex_init(&emu_dev->mutex);
        }
    }
}

void usb_host_enable(void) {
    vcc_set_enabled(true);
    
    if ((usbh_system.hw_type == USBH_HW_PIO )&& USBH_PIO_ENABLED) {
        pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
        tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
        tuh_init(BOARD_TUH_RHPORT);
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
        emu_itf_t* emu_itf = &usbh_system.emu_devices[index].emu_itf[i];
        if (!emu_itf->alloc) {
            continue;
        }
        usbh_system.tru_devices[emu_itf->daddr - 1].disabled = !enabled;
        if (enabled) {
            if (i != PERIPH_AUDIO) {
                tuh_hxx_receive_report(emu_itf->daddr, emu_itf->itf_num);
            } else {
                tuh_xaudio_receive_data(emu_itf->daddr, emu_itf->itf_num);
                tuh_xaudio_receive_data_ctrl(emu_itf->daddr, emu_itf->itf_num_ctrl);
            }
        }
    }
}

void usb_host_set_rumble(uint8_t index, const gamepad_rumble_t* rumble) {
    if ((index >= GAMEPADS_MAX) ||
        !usbh_system.emu_devices[index].emu_itf[PERIPH_GAMEPAD].alloc) {
        return;
    }
    if (usbh_system.use_mutex) {
        emu_dev_t* emu_dev = &usbh_system.emu_devices[index];
        mutex_enter_blocking(&emu_dev->mutex);
        memcpy(emu_dev->emu_itf[PERIPH_GAMEPAD].buffer[GP_DATA_UNSAFE], rumble, sizeof(gamepad_rumble_t));
        atomic_store_explicit(&emu_dev->emu_itf[PERIPH_GAMEPAD].new_data, true, memory_order_release);
        mutex_exit(&emu_dev->mutex);
    } else {
        const uint8_t daddr   = usbh_system.emu_devices[index].emu_itf[PERIPH_GAMEPAD].daddr;
        const uint8_t itf_num = usbh_system.emu_devices[index].emu_itf[PERIPH_GAMEPAD].itf_num;
        const usb_host_driver_t* driver = usbh_system.tru_devices[daddr - 1].interfaces[itf_num].driver;
        if (driver && driver->send_rumble) {
            driver->send_rumble(index, daddr, itf_num, rumble);
        }
    }
}

void usb_host_set_audio(uint8_t index, const gamepad_pcm_out_t* pcm) {
    if ((index >= GAMEPADS_MAX) ||
        !usbh_system.emu_devices[index].emu_itf[PERIPH_AUDIO].alloc) {
        return;
    }
    if (usbh_system.use_mutex) {
        emu_dev_t* emu_dev = &usbh_system.emu_devices[index];
        mutex_enter_blocking(&emu_dev->mutex);
        memcpy(emu_dev->emu_itf[PERIPH_AUDIO].buffer[GP_DATA_UNSAFE], pcm, sizeof(gamepad_pcm_out_t));
        atomic_store_explicit(&emu_dev->emu_itf[PERIPH_AUDIO].new_data, true, memory_order_release);
        mutex_exit(&emu_dev->mutex);
    } else {
        const uint8_t daddr   = usbh_system.emu_devices[index].emu_itf[PERIPH_AUDIO].daddr;
        const uint8_t itf_num = usbh_system.emu_devices[index].emu_itf[PERIPH_AUDIO].itf_num;
        const usb_host_driver_t* driver = usbh_system.tru_devices[daddr - 1].interfaces[itf_num].driver;
        if (driver && driver->send_audio) {
            driver->send_audio(index, daddr, itf_num, pcm);
        }
    }
}

void usb_host_task(void) {
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        emu_dev_t* emu_dev = &usbh_system.emu_devices[i];

        if (usbh_system.use_mutex) {
            bool new_rumble = atomic_load_explicit(&emu_dev->emu_itf[PERIPH_GAMEPAD].new_data, 
                                                   memory_order_acquire);
            bool new_pcm = atomic_load_explicit(&emu_dev->emu_itf[PERIPH_AUDIO].new_data, 
                                                memory_order_acquire);
            if (new_rumble || new_pcm) {
                mutex_enter_blocking(&emu_dev->mutex);
                if (new_rumble) {
                    memcpy(emu_dev->emu_itf[PERIPH_GAMEPAD].buffer[GP_DATA_SAFE], 
                           emu_dev->emu_itf[PERIPH_GAMEPAD].buffer[GP_DATA_UNSAFE], 
                           sizeof(gamepad_rumble_t));
                    atomic_store_explicit(&emu_dev->emu_itf[PERIPH_GAMEPAD].new_data, 
                                          false, memory_order_relaxed);
                }
                if (new_pcm) {
                    memcpy(emu_dev->emu_itf[PERIPH_AUDIO].buffer[GP_DATA_SAFE], 
                           emu_dev->emu_itf[PERIPH_AUDIO].buffer[GP_DATA_UNSAFE], 
                           sizeof(gamepad_pcm_out_t));
                    atomic_store_explicit(&emu_dev->emu_itf[PERIPH_AUDIO].new_data, 
                                          false, memory_order_relaxed);
                }
                mutex_exit(&emu_dev->mutex);
            }
            if (new_rumble) {
                const uint8_t daddr   = emu_dev->emu_itf[PERIPH_GAMEPAD].daddr;
                const uint8_t itf_num = emu_dev->emu_itf[PERIPH_GAMEPAD].itf_num;
                const gamepad_rumble_t* rumble = 
                    (const gamepad_rumble_t*)emu_dev->emu_itf[PERIPH_GAMEPAD].buffer[GP_DATA_SAFE];
                const usb_host_driver_t* driver = 
                    usbh_system.tru_devices[daddr - 1].interfaces[itf_num].driver;

                if (driver && driver->send_rumble) {
                    driver->send_rumble(i, daddr, itf_num, rumble);
                }
            }
            if (new_pcm) {
                const uint8_t daddr   = emu_dev->emu_itf[PERIPH_AUDIO].daddr;
                const uint8_t itf_num = emu_dev->emu_itf[PERIPH_AUDIO].itf_num;
                const gamepad_pcm_out_t* pcm = 
                    (const gamepad_pcm_out_t*)emu_dev->emu_itf[PERIPH_AUDIO].buffer[GP_DATA_SAFE];
                const usb_host_driver_t* driver = 
                    usbh_system.tru_devices[daddr - 1].interfaces[itf_num].driver;

                if (driver && driver->send_audio) {
                    driver->send_audio(i, daddr, itf_num, pcm);
                }
            }
        }
    }
    for (uint8_t i = 0; i < CFG_TUH_DEVICE_MAX; i++) {
        true_dev_t* dev = &usbh_system.tru_devices[i];
        if (dev->disabled) {
            continue;
        }
        for (uint8_t j = 0; j < USBH_INTERFACES_MAX; j++) {
            true_itf_t* itf = &dev->interfaces[j];
            if (itf->driver && itf->driver->task_cb) {
                itf->driver->task_cb(itf->gp_index, itf->emu_itf->daddr, itf->emu_itf->itf_num);
            }
        }
    }
    tuh_task();
}

#endif // USBH_ENABLED