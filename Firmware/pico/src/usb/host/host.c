#include "board_config.h"
#if USBH_ENABLED

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
#include "ring/ring_usb.h"

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

typedef struct {
    volatile bool               alloc;
    volatile bool               disabled;
    uint8_t                     gp_index;
    usbh_type_t                 dev_type;
    usbh_periph_t               periph_type;
    const usb_host_driver_t*    driver;
    uint8_t                     daddr;
    uint8_t                     itf_num;
    uint8_t                     state_buffer[USBH_STATE_BUFFER_SIZE] __attribute__((aligned(4)));
} emu_interface_t;

typedef struct {
    volatile bool           multithread;
    usbh_hw_type_t          hw_type;
    usb_host_connect_cb_t   connect_cb;
    gamepad_pad_cb_t        gamepad_cb;
    gamepad_pcm_cb_t        audio_cb;
    emu_interface_t         gp_itf[GAMEPADS_MAX][PERIPH_COUNT];
    emu_interface_t*        tuh_itf_lookup[CFG_TUH_DEVICE_MAX][USBH_INTERFACES_MAX];
    ring_usb_t              ring;
    uint8_t                 ring_buf[RING_USB_BUF_SIZE] __attribute__((aligned(4)));
} usbh_system_t;

static usbh_system_t  usbh_system = {0};
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
    if (usbh_system.gp_itf[gp_index][periph_type].alloc) {
        ogxm_loge("Interface %d on device %d is already assigned\n", itf_num, daddr);
        return false;
    } else if ((daddr == 0) || (itf_num >= USBH_INTERFACES_MAX)) {
        ogxm_loge("Invalid device address %d or interface number %d\n", daddr, itf_num);
        return false;
    } else if (usbh_system.tuh_itf_lookup[daddr - 1][itf_num] != NULL) {
        ogxm_loge("Interface %d on device %d is already assigned\n", itf_num, daddr);
        return false;
    }
    emu_interface_t* emu_itf = &usbh_system.gp_itf[gp_index][periph_type];
    emu_itf->alloc = true;
    emu_itf->gp_index = gp_index;
    emu_itf->dev_type = dev_type;
    emu_itf->periph_type = periph_type;
    emu_itf->driver = USBH_DRIVERS[dev_type];
    emu_itf->daddr = daddr;
    emu_itf->itf_num = itf_num;
    memset(emu_itf->state_buffer, 0, USBH_STATE_BUFFER_SIZE);
    usbh_system.tuh_itf_lookup[daddr - 1][itf_num] = emu_itf;
    return true;
}

static bool assign_emu_device(uint8_t daddr, uint8_t itf_num, usbh_type_t dev_type) {
    /* Combine gamepad, headset, and chatpad into a single emulated device */
    if ((daddr == 0) || (itf_num >= USBH_INTERFACES_MAX)) {
        ogxm_loge("Invalid device address %d or interface number %d\n", daddr, itf_num);
        return false;
    } else if (usbh_system.tuh_itf_lookup[daddr - 1][itf_num] != NULL) {
        ogxm_loge("Interface %d on device %d is already assigned\n", itf_num, daddr);
        return false;
    }
    
    usbh_periph_t periph_type = PERIPH_COUNT;
    bool xinput_wl = (dev_type == USBH_TYPE_XINPUT_WL) || 
                     (dev_type == USBH_TYPE_XINPUT_WL_AUDIO);

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
        // /*  Tinyusb mounts interfaces backwards, if itf_num/2 >= GAMEPADS_MAX
        //     and we assign it a gamepad, there will be no more room 
        //     left for the first gamepad. Xinput wireless interfaces are 
        //     itf0=gamepad1, itf1=audio1, itf3=gamepad2, itf4=audio2, etc. */
        // if (dev_type == USBH_TYPE_XINPUT_WL) {
        //     if ((itf_num / 2) >= GAMEPADS_MAX) {
        //         return false;
        //     }
        // } else if (dev_type == USBH_TYPE_XINPUT_WL_AUDIO) {
        //     if (((itf_num - 1) / 2) >= GAMEPADS_MAX) {
        //         return false;
        //     }
        // }
    case USBH_TYPE_XGIP:
    case USBH_TYPE_XGIP_CHATPAD:
    case USBH_TYPE_XGIP_AUDIO:
    case USBH_TYPE_XINPUT:
    case USBH_TYPE_XINPUT_AUDIO:
    case USBH_TYPE_XINPUT_CHATPAD:
        /* Try to pair these to an existing device */
        for (uint8_t j = 0; j < USBH_INTERFACES_MAX; j++) {
            if (usbh_system.tuh_itf_lookup[daddr - 1][j] == NULL) {
                continue;
            } else {
                uint8_t gp_idx = usbh_system.tuh_itf_lookup[daddr - 1][j]->gp_index;
                if (usbh_system.gp_itf[gp_idx][periph_type].alloc) {
                    // Peripheral type is already assigned to this gamepad
                    continue;
                }
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
            if (!usbh_system.gp_itf[i][periph_target].alloc) {
                continue;
            } else if (usbh_system.gp_itf[i][periph_target].dev_type == dev_target) {
                return assign_emu_itf(daddr, itf_num, dev_type, i, periph_type);
            }
        }
        }
        break;
    default:
        break;
    }
    /* Just find a free slot */
    if (xinput_wl) {
        /* Go backwards for Xinput wireless */
        for (int i = (GAMEPADS_MAX - 1); i >= 0; i--) {
            if (usbh_system.gp_itf[i][periph_type].alloc) {
                continue;
            } else {
                return assign_emu_itf(daddr, itf_num, dev_type, i, periph_type);
            }
        }
    } else {
        for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
            if (usbh_system.gp_itf[i][periph_type].alloc) {
                continue;
            } else {
                return assign_emu_itf(daddr, itf_num, dev_type, i, periph_type);
            }
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

    emu_interface_t* emu_itf = usbh_system.tuh_itf_lookup[daddr - 1][itf_num];
    if ((emu_itf == NULL) || !emu_itf->alloc || (emu_itf->driver == NULL)) {
        panic("tuh_hxx_mounted_cb: Interface %d on device %d is not assigned\n", itf_num, daddr);
        return;
    }

    const usb_host_driver_t* driver = emu_itf->driver;
    ogxm_logi("Initializing host driver: %s for daddr %d, itf %d\n", 
        driver->name, daddr, itf_num);

    if (driver->mounted_cb != NULL) {
        driver->mounted_cb(type, emu_itf->gp_index, daddr, itf_num, 
                           desc_report, desc_len, emu_itf->state_buffer);
    }
}

void tuh_hxx_unmounted_cb(uint8_t daddr, uint8_t itf_num) {
    emu_interface_t* emu_itf = usbh_system.tuh_itf_lookup[daddr - 1][itf_num];
    if (usbh_system.tuh_itf_lookup[daddr - 1][itf_num] == NULL) {
        ogxm_loge("tuh_hxx_unmounted_cb: Interface %d on device %d is not assigned\n", itf_num, daddr);
        return;
    }
    const usb_host_driver_t* driver = emu_itf->driver;

    ogxm_logi("Unmounting host driver: %s, daddr %d, itf %d\n", 
        (driver != NULL) ? driver->name : "Unknown", daddr, itf_num);

    if ((driver != NULL) && (driver->unmounted_cb != NULL)) {
        driver->unmounted_cb(emu_itf->gp_index, daddr, itf_num);
    }

    if (usbh_system.connect_cb != NULL) {
        usbh_system.connect_cb(emu_itf->gp_index, emu_itf->dev_type, false);
    }

    uint8_t gp_index = emu_itf->gp_index;
    usbh_periph_t periph_type = emu_itf->periph_type;
    memset(emu_itf, 0, sizeof(emu_interface_t));
    usbh_system.tuh_itf_lookup[daddr - 1][itf_num] = NULL;
}

void tuh_hxx_report_received_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len) {
    if ((daddr == 0) || (itf_num >= USBH_INTERFACES_MAX)) {
        ogxm_loge("Invalid device address %d or interface number %d\n", daddr, itf_num);
        return;
    }
    emu_interface_t* emu_itf = usbh_system.tuh_itf_lookup[daddr - 1][itf_num];
    if ((emu_itf != NULL) && !emu_itf->disabled) {
        const usb_host_driver_t* driver = emu_itf->driver;
        if (driver->report_cb) {
            driver->report_cb(emu_itf->gp_index, emu_itf->periph_type, daddr, itf_num, data, len);
        }
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

void usb_host_driver_pad_cb(uint8_t index, const gamepad_pad_t* pad) {
    if (usbh_system.gamepad_cb) {
        usbh_system.gamepad_cb(index, pad);
    }
}

/* ---- Public ---- */

void usb_host_configure(const usb_host_config_t* config) {
    memset(&usbh_system, 0, sizeof(usbh_system_t));
    usbh_system.connect_cb = config->connect_cb;
    usbh_system.gamepad_cb = config->gamepad_cb;
    usbh_system.audio_cb = config->audio_cb;
    usbh_system.multithread = config->multithread;
    usbh_system.hw_type = config->hw_type;
    vcc_set_enabled(false);
    ogxm_logd("USB Host configured with multithread: %d, hw_type: %d\n", 
        usbh_system.multithread, usbh_system.hw_type);
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
        if (!tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg)) {
            panic("usb_host_enable: Failed to configure PIO USB\n");
        }
        if (!tuh_init(BOARD_TUH_RHPORT)) {
            panic("usb_host_enable: Failed to initialize PIO USB\n");
        }
        ogxm_logd("USB Host PIO initialized on rhport %d\n", BOARD_TUH_RHPORT);
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
        if (!usbh_system.gp_itf[index][i].alloc) {
            continue;
        }
        emu_interface_t* emu_itf = &usbh_system.gp_itf[index][i];
        if (emu_itf->disabled == enabled) {
            emu_itf->disabled = !enabled;
            if (i != PERIPH_AUDIO) {
                tuh_hxx_receive_report(emu_itf->daddr, emu_itf->itf_num);
            } else {
                // tuh_xaudio_receive_data(emu_itf->daddr, emu_itf->itf_num);
                // tuh_xaudio_receive_data_ctrl(emu_itf->daddr, emu_itf->itf_num_ctrl);
            }
        }
    }
}

void usb_host_set_rumble(uint8_t index, const gamepad_rumble_t* rumble) {
    if ((index >= GAMEPADS_MAX) ||
        !usbh_system.gp_itf[index][PERIPH_GAMEPAD].alloc) {
        return;
    }
    if (usbh_system.multithread) {
        ring_usb_packet_t packet = {
            .type = RING_USB_TYPE_RUMBLE,
            .index = index,
            .payload_len = sizeof(gamepad_rumble_t)
        };
        ring_usb_push(&usbh_system.ring, &packet, rumble);
    } else {
        emu_interface_t* emu_itf = &usbh_system.gp_itf[index][PERIPH_GAMEPAD];
        const usb_host_driver_t* driver = emu_itf->driver;
        if ((driver != NULL) && (driver->send_rumble != NULL)) {
            driver->send_rumble(index, emu_itf->daddr, emu_itf->itf_num, rumble);
        }
    }
}

void usb_host_set_audio(uint8_t index, const gamepad_pcm_out_t* pcm) {
    if ((index >= GAMEPADS_MAX) ||
        !usbh_system.gp_itf[index][PERIPH_GAMEPAD].alloc) {
        return;
    }
    if (usbh_system.multithread) {
        ring_usb_packet_t packet = {
            .type = RING_USB_TYPE_PCM,
            .index = index,
            .payload_len = sizeof(gamepad_pcm_out_t)
        };
        ring_usb_push(&usbh_system.ring, &packet, pcm);
    } else {
        emu_interface_t* emu_itf = &usbh_system.gp_itf[index][PERIPH_AUDIO];
        const usb_host_driver_t* driver = emu_itf->driver;
        if ((driver != NULL) && (driver->send_audio != NULL)) {
            driver->send_audio(index, emu_itf->daddr, emu_itf->itf_num, pcm);
        }
    }
}

void usb_host_task(void) {
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        for (uint8_t j = 0; j < PERIPH_COUNT; j++) {
            if (!usbh_system.gp_itf[i][j].alloc ||
                usbh_system.gp_itf[i][j].disabled) {
                continue;
            }
            emu_interface_t* emu_itf = &usbh_system.gp_itf[i][j];
            const usb_host_driver_t* driver = emu_itf->driver;
            if (driver->task_cb != NULL) {
                driver->task_cb(emu_itf->gp_index, emu_itf->daddr, emu_itf->itf_num);
                tuh_task();
            }
        }
    }
    if (usbh_system.multithread) {
        if (ring_usb_pop(&usbh_system.ring, usbh_system.ring_buf)) {
            emu_interface_t* emu_itf = NULL;
            ring_usb_packet_t* packet = (ring_usb_packet_t*)usbh_system.ring_buf;

            switch (packet->type) {
            case RING_USB_TYPE_RUMBLE:
                emu_itf = &usbh_system.gp_itf[packet->index][PERIPH_GAMEPAD];
                if (!emu_itf->alloc ||
                    emu_itf->disabled ||
                    emu_itf->driver == NULL ||
                    emu_itf->driver->send_rumble == NULL) {
                    break;
                }
                emu_itf->driver->send_rumble(packet->index, emu_itf->daddr, 
                                             emu_itf->itf_num, 
                                             (gamepad_rumble_t*)packet->payload);
                break;
            case RING_USB_TYPE_PCM:
                emu_itf = &usbh_system.gp_itf[packet->index][PERIPH_AUDIO];
                if (!emu_itf->alloc || 
                    emu_itf->disabled || 
                    emu_itf->driver == NULL ||
                    emu_itf->driver->send_audio == NULL) {
                    break;
                }
                emu_itf->driver->send_audio(packet->index, emu_itf->daddr, 
                                            emu_itf->itf_num, 
                                            (gamepad_pcm_t*)packet->payload);
                break;
            default:
                ogxm_loge("usb_host_task: Unknown ring buffer type %d, index=%d\n", 
                    packet->type, packet->index);
                break;
            }
        }
    }
    tuh_task();
}

#endif // USBH_ENABLED