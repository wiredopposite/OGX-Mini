#include <string.h>
#include <stdatomic.h>
#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <hardware/gpio.h>
#include "log/log.h"
#include "usbd/usbd.h"
#include "usb/device/device_private.h"
#include "usb/device/drivers/generic_hub.h"
#include "usb/device/device.h"

typedef enum {
    USBD_INT_TYPE_XBOXOG_GP = 0,
    USBD_INT_TYPE_XBOXOG_SB,
    USBD_INT_TYPE_XBOXOG_XR,
    USBD_INT_TYPE_XINPUT,
    // USBD_INT_TYPE_DINPUT,
    USBD_INT_TYPE_PS3,
    USBD_INT_TYPE_PSCLASSIC,
    USBD_INT_TYPE_SWITCH,
    USBD_INT_TYPE_WEBAPP,
    USBD_INT_TYPE_UART_BRIDGE,
    /* Internal types */
    USBD_INT_TYPE_XBOXOG_HUB,
    USBD_INT_TYPE_XBOXOG_XMU,
    USBD_INT_TYPE_XBOXOG_XBLC,
    USBD_INT_TYPE_COUNT
} usbd_internal_type_t;

typedef struct {
    mutex_t                     gp_mutex;
    atomic_bool                 pad_new;
    gamepad_pad_t               pad;
    uint32_t                    pad_flags;
    atomic_bool                 pcm_new;
    gamepad_pcm_out_t           pcm_out;

    usbd_handle_t*              handle;
    const usb_device_driver_t*  driver;
} usb_gamepad_t;

typedef struct {
    bool            inited;
    bool            use_mutex;
    bool            pio;
    bool            generic_hub;
    usbd_handle_t*  generic_hub_handle;
    usb_gamepad_t   gamepads[GAMEPADS_MAX];
    uint8_t         status_buffer[USBD_DEVICES_MAX][USBD_STATUS_BUF_SIZE] __attribute__((aligned(4)));
    app_rumble_cb_t rumble_cb;
    app_audio_cb_t  audio_cb;
} usbd_system_t;

static const usb_device_driver_t* USBD_DRIVERS[USBD_INT_TYPE_COUNT] = {
    [USBD_INT_TYPE_XBOXOG_GP]   = &USBD_DRIVER_XBOXOG_GP,
    [USBD_INT_TYPE_XBOXOG_SB]   = &USBD_DRIVER_XBOXOG_SB,
    [USBD_INT_TYPE_XBOXOG_XR]   = NULL,
    [USBD_INT_TYPE_XINPUT]      = &USBD_DRIVER_XINPUT,
    // [USBD_INT_TYPE_DINPUT]      = &USBD_DRIVER_DINPUT,
    [USBD_INT_TYPE_PS3]         = &USBD_DRIVER_PS3,
    [USBD_INT_TYPE_PSCLASSIC]   = &USBD_DRIVER_PSCLASSIC,
    [USBD_INT_TYPE_SWITCH]      = &USBD_DRIVER_SWITCH,
    [USBD_INT_TYPE_WEBAPP]      = &USBD_DRIVER_WEBAPP,
    [USBD_INT_TYPE_UART_BRIDGE] = &USBD_DRIVER_UART_BRIDGE,
    [USBD_INT_TYPE_XBOXOG_HUB]  = &USBD_DRIVER_XBOXOG_HUB,
    [USBD_INT_TYPE_XBOXOG_XMU]  = NULL,
    [USBD_INT_TYPE_XBOXOG_XBLC] = NULL,
};

static usbd_system_t usbd_system = {0};
static size_t usbd_system_size = sizeof(usbd_system_t);

static void ts3usb221_init(void) {
#if TS3USB221_ENABLED
    /* Set disconnected */
    gpio_init(TS3USB221_PIN_MUX_OE);
    gpio_init(TS3USB221_PIN_MUX_SEL);
    gpio_set_dir(TS3USB221_PIN_MUX_OE, GPIO_OUT);
    gpio_set_dir(TS3USB221_PIN_MUX_SEL, GPIO_OUT);
    gpio_put(TS3USB221_PIN_MUX_OE, 1);
    gpio_put(TS3USB221_PIN_MUX_SEL, 1);
#endif
}

static void ts3usb221_set_connected(usbd_hw_type_t hw_type, bool connected) {
#if TS3USB221_ENABLED
    if (!connected) {
        gpio_put(TS3USB221_PIN_MUX_OE, 1);
        gpio_put(TS3USB221_PIN_MUX_SEL, 1);
    } else {
        if (hw_type == USBD_HW_PIO) {
            gpio_put(TS3USB221_PIN_MUX_OE, 0);
            gpio_put(TS3USB221_PIN_MUX_SEL, 0);
        } else {
            gpio_put(TS3USB221_PIN_MUX_OE, 0);
            gpio_put(TS3USB221_PIN_MUX_SEL, 1);
        }
    }
#endif
}

void usb_device_rumble_cb(usbd_handle_t* handle, const gamepad_rumble_t *rumble) {
    /* Everything is index 0 unless using a generic hub, may change */
    uint8_t index = (usbd_system.generic_hub) ? (handle->port - 1) : 0;
    if (usbd_system.rumble_cb != NULL) {
        usbd_system.rumble_cb(index, rumble);
    }
}

void usb_device_audio_cb(usbd_handle_t* handle, const gamepad_pcm_out_t *pcm_out) {
    /* Everything is index 0 unless using a generic hub, may change */
    uint8_t index = (usbd_system.generic_hub) ? (handle->port - 1) : 0;
    if (usbd_system.audio_cb != NULL) {
        usbd_system.audio_cb(index, pcm_out);
    }
}

bool usb_device_configure(const usb_device_config_t* config) {
    if (config == NULL || usbd_system.inited) {
        return false;
    }
    for (uint8_t i = 0; i < config->count; i++) {
        if ((uint8_t)config->usb[i].type >= USBD_TYPE_COUNT) {
            return false;
        }
    }
    memset(&usbd_system, 0, sizeof(usbd_system_t));
    usbd_system.audio_cb = config->audio_cb;
    usbd_system.rumble_cb = config->rumble_cb;
    usbd_system.use_mutex = config->use_mutex;

    if (usbd_system.use_mutex) {
        for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
            mutex_init(&usbd_system.gamepads[i].gp_mutex);
        }
    }

    ts3usb221_init();

    if (config->count > 1) {
#if USBD_PIO_ENABLED
        usbd_system.generic_hub = true;
        usbd_system.pio = true;
        usb_device_driver_cfg_t drv_cfg = {
            .usb = {
                .hw_type = USBD_HW_PIO,
                .addons = 0,
                .status_buffer = usbd_system.status_buffer[0],
            }
        };
        usbd_system.generic_hub_handle = generic_hub_init(&drv_cfg);
        if (usbd_system.generic_hub_handle == NULL) {
            return false;
        }

        uint8_t ds_max = MIN(config->count, MIN(USBD_DEVICES_MAX - 1, GAMEPADS_MAX));
        for (uint8_t i = 0; i < ds_max; i++) {
            drv_cfg.usb.addons = config->usb[i].addons;
            drv_cfg.usb.status_buffer = usbd_system.status_buffer[i + 1];

            usbd_system.gamepads[i].driver = USBD_DRIVERS[config->usb[i].type];
            if (usbd_system.gamepads[i].driver->init == NULL) {
                continue;
            }
            usbd_system.gamepads[i].handle = usbd_system.gamepads[i].driver->init(&drv_cfg);
            if (usbd_system.gamepads[i].handle == NULL) {
                continue;
            }
            generic_hub_register_ds_port(usbd_system.gamepads[i].handle);
        }
        usbd_system.inited = true;
        return true;
    } else if ((config->usb[0].type == USBD_TYPE_XBOXOG_GP) && (config->usb[0].addons != 0)) {
        usbd_system.pio = true;
        usb_device_driver_cfg_t drv_cfg = {
            .xboxog_hub = {
                .hw_type = USBD_HW_PIO,
                .addons = config->usb[0].addons,
                .status_buffer = {NULL},
            }
        };
        for (uint8_t i = 0; i < USBD_DEVICES_MAX; i++) {
            drv_cfg.xboxog_hub.status_buffer[i] = usbd_system.status_buffer[i];
        }
        usbd_system.gamepads[0].driver = USBD_DRIVERS[USBD_INT_TYPE_XBOXOG_HUB];
        usbd_system.gamepads[0].handle = usbd_system.gamepads[0].driver->init(&drv_cfg);
        if (usbd_system.gamepads[0].handle == NULL) {
            return false;
        }
        usbd_system.inited = true;
        return true;
#endif // USBD_PIO_ENABLED
    } else {
        usb_device_driver_cfg_t drv_cfg = {
            .usb = {
                .hw_type = USBD_HW_USB,
                .addons = config->usb[0].addons,
                .status_buffer = usbd_system.status_buffer[0],
            }
        };
        usbd_system.gamepads[0].driver = USBD_DRIVERS[config->usb[0].type];
        usbd_system.gamepads[0].handle = usbd_system.gamepads[0].driver->init(&drv_cfg);
        if (usbd_system.gamepads[0].handle == NULL) {
            return false;
        }
        usbd_system.inited = true;
        return true;
    }
    return false;
}

void usb_device_connect(void) {
    if (!usbd_system.inited) {
        return;
    }
    ts3usb221_set_connected(usbd_system.pio ? USBD_HW_PIO : USBD_HW_USB, true);
    for (uint8_t i = 0; i < USBD_DEVICES_MAX; i++) {
        usbd_connect(usbd_system.pio ? USBD_HW_PIO : USBD_HW_USB);
    }
}

void usb_device_deinit(void) {
    ts3usb221_set_connected(usbd_system.pio ? USBD_HW_PIO : USBD_HW_USB, false);
    usbd_deinit(usbd_system.pio ? USBD_HW_PIO : USBD_HW_USB);
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        if (usbd_system.use_mutex) {
            mutex_enter_blocking(&usbd_system.gamepads[i].gp_mutex);
            usbd_system.gamepads[i].handle = NULL;
            mutex_exit(&usbd_system.gamepads[i].gp_mutex);
        }
    }
    usbd_system.inited = false;
    memset(&usbd_system, 0, sizeof(usbd_system_t));
}

bool usb_device_get_pad_unsafe(uint8_t index, gamepad_pad_t* pad) {
    if ((index >= GAMEPADS_MAX) || (pad == NULL) || 
        (usbd_system.gamepads[index].handle == NULL) ||
        (usbd_system.gamepads[index].driver->set_pad == NULL)) {
        return false;
    }
    memcpy(pad, &usbd_system.gamepads[index].pad, sizeof(gamepad_pad_t));
    return true;
}

bool usb_device_get_pad_safe(uint8_t index, gamepad_pad_t* pad) {
    if ((index >= GAMEPADS_MAX) || (pad == NULL) || 
        (usbd_system.gamepads[index].handle == NULL) ||
        (usbd_system.gamepads[index].driver->set_pad == NULL)) {
        return false;
    }
    if (usbd_system.use_mutex) {
        mutex_enter_blocking(&usbd_system.gamepads[index].gp_mutex);
        memcpy(pad, &usbd_system.gamepads[index].pad, sizeof(gamepad_pad_t));
        mutex_exit(&usbd_system.gamepads[index].gp_mutex);
    } else {
        memcpy(pad, &usbd_system.gamepads[index].pad, sizeof(gamepad_pad_t));
    }
    return true;
}

void usb_device_set_pad(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    if ((usbd_system.gamepads[index].handle == NULL) ||
        (usbd_system.gamepads[index].driver->set_pad == NULL)) {
        return;
    }
    if (usbd_system.use_mutex) {
        mutex_enter_blocking(&usbd_system.gamepads[index].gp_mutex);

        if ((flags & GAMEPAD_FLAG_IN_CHATPAD) == 0) {
            memcpy(&usbd_system.gamepads[index].pad, 
                   pad, offsetof(gamepad_pad_t, chatpad));
            usbd_system.gamepads[index].pad_flags |= flags;
        } else if ((flags & GAMEPAD_FLAG_IN_CHATPAD) && 
                   !(flags & GAMEPAD_FLAG_IN_PAD)) {
            memcpy(&usbd_system.gamepads[index].pad.chatpad, 
                   pad->chatpad, sizeof(pad->chatpad));
            usbd_system.gamepads[index].pad_flags |= flags;
        } else {
            memcpy(&usbd_system.gamepads[index].pad, 
                   pad, sizeof(gamepad_pad_t));
            usbd_system.gamepads[index].pad_flags = flags;
        }
        atomic_store_explicit(&usbd_system.gamepads[index].pad_new, true, memory_order_release);
        mutex_exit(&usbd_system.gamepads[index].gp_mutex);
    } else {
        if (usbd_system.gamepads[index].driver->set_pad != NULL) {
            usbd_system.gamepads[index].driver->set_pad(
                usbd_system.gamepads[index].handle, 
                pad, 
                flags
            );
        }
        if ((flags & GAMEPAD_FLAG_IN_CHATPAD) == 0) {
            memcpy(&usbd_system.gamepads[index].pad, 
                   pad, offsetof(gamepad_pad_t, chatpad));
        } else if ((flags & GAMEPAD_FLAG_IN_CHATPAD) && 
                   !(flags & GAMEPAD_FLAG_IN_PAD)) {
            memcpy(&usbd_system.gamepads[index].pad.chatpad, 
                   pad->chatpad, sizeof(pad->chatpad));
        } else {
            memcpy(&usbd_system.gamepads[index].pad, 
                   pad, sizeof(gamepad_pad_t));
        }
    }
}

void usb_device_set_audio(uint8_t index, const gamepad_pcm_out_t* pcm_out) {
    if ((usbd_system.gamepads[index].handle == NULL) ||
        (usbd_system.gamepads[index].driver->set_audio == NULL)) {
        return;
    }
    if (usbd_system.use_mutex) {
        mutex_enter_blocking(&usbd_system.gamepads[index].gp_mutex);
        memcpy(&usbd_system.gamepads[index].pcm_out, pcm_out, sizeof(gamepad_pcm_out_t));
        atomic_store_explicit(&usbd_system.gamepads[index].pcm_new, true, memory_order_release);
        mutex_exit(&usbd_system.gamepads[index].gp_mutex);
    } else {
        if (usbd_system.gamepads[index].driver->set_audio == NULL) {
            return;
        }
        usbd_system.gamepads[index].driver->set_audio(
            usbd_system.gamepads[index].handle, 
            pcm_out
        );
    }
}

void usb_device_task(void) {
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        if (usbd_system.gamepads[i].handle == NULL) {
            continue;
        }
        if (usbd_system.use_mutex) {
            if ((usbd_system.gamepads[i].driver->set_pad != NULL) && 
                atomic_load_explicit(&usbd_system.gamepads[i].pad_new, memory_order_acquire)) {
                mutex_enter_blocking(&usbd_system.gamepads[i].gp_mutex);
                atomic_store_explicit(&usbd_system.gamepads[i].pad_new, false, memory_order_relaxed);
                gamepad_pad_t pad = usbd_system.gamepads[i].pad;
                uint32_t flags = usbd_system.gamepads[i].pad_flags;
                usbd_system.gamepads[i].pad_flags = 0; // Reset flags after reading
                mutex_exit(&usbd_system.gamepads[i].gp_mutex);
                usbd_system.gamepads[i].driver->set_pad(
                    usbd_system.gamepads[i].handle, 
                    &pad, 
                    flags
                );
            }
            if ((usbd_system.gamepads[i].driver->set_audio != NULL) &&
                atomic_load_explicit(&usbd_system.gamepads[i].pcm_new, memory_order_acquire)) {
                mutex_enter_blocking(&usbd_system.gamepads[i].gp_mutex);
                atomic_store_explicit(&usbd_system.gamepads[i].pcm_new, false, memory_order_relaxed);
                gamepad_pcm_out_t pcm = usbd_system.gamepads[i].pcm_out;
                mutex_exit(&usbd_system.gamepads[i].gp_mutex);
                usbd_system.gamepads[i].driver->set_audio(
                    usbd_system.gamepads[i].handle, 
                    &pcm
                );
            }
        }
        if (usbd_system.gamepads[i].driver->task != NULL) {
            usbd_system.gamepads[i].driver->task(usbd_system.gamepads[i].handle);
        }
    }
    usbd_task();
}