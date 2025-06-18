#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_BLUETOOTH)

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/clocks.h>
#include "log/log.h"
#include "bluetooth/bluetooth.h"
#include "usb/device/device.h"
#include "gamepad/gamepad.h"
#include "settings/settings.h"
#include "shared.h"

static void bt_connect_cb(uint8_t index, bool connected) {
    ogxm_logi("Bluetooth device %d %s\n", 
        index, connected ? "connected" : "disconnected");
}

void core1_entry(void) {
    if (!bluetooth_init(bt_connect_cb, usb_device_set_pad, usb_device_set_audio)) {
        panic("Bluetooth initialization failed on core 1!\n");
    }
    while (true) {
        bluetooth_task();
    }
}

static void uart_bridge_task(void) {
    usb_device_config_t usbd_config = {
        .multithread = false,
        .count = 1,
        .usb = {
            {
                .type = USBD_TYPE_UART_BRIDGE,
                .addons = 0,
            }
        },
        .rumble_cb = NULL,
        .audio_cb = NULL,
    };

    usb_device_configure(&usbd_config);
    usb_device_connect();

    ogxm_logi("UART Bridge started\n");

    while (true) {
        usb_device_task();
        sleep_ms(1);
    }
}

int main(void) {
    set_sys_clock_khz(240000, true);
    ogxm_log_init(true);
    settings_init();

#if VERIFY_BUILD_VERSION
    ogxm_logd("Verifying datetime...\n");
    if (!settings_valid_datetime()) {
        ogxm_logd("Invalid datetime, starting UART bridge\n");
        uart_bridge_task();
    }
    ogxm_logd("Datetime is valid, continuing...\n");
#endif

    usbd_type_t device_type = settings_get_device_type();
    usb_device_config_t usbd_config = {
        .multithread = true,
        .count = 1,
        .usb = {
            {
                .type = device_type,
                .addons = 0,
            }
        },
        .rumble_cb = bluetooth_set_rumble,
        .audio_cb = bluetooth_set_audio,
    };

    usb_device_configure(&usbd_config);
    usb_device_connect();

    multicore_reset_core1();
    multicore_launch_core1(core1_entry);

    check_pad_timer_set_enabled(true);

    while (true) {
        usb_device_task();

        if (check_pad_time()) {
            check_pad_for_driver_change(0, device_type);
        }
    }
}

#endif // (OGXM_BOARD == OGXM_BOARD_BLUETOOTH)