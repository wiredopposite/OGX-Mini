#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_BLUETOOTH)

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/time.h>
#include <hardware/clocks.h>
#include <hardware/watchdog.h>
#include "log/log.h"
#include "bluetooth/bluetooth.h"
#include "usb/device/device.h"
#include "gamepad/gamepad.h"
#include "settings/settings.h"
#include "settings/button_combo.h"

volatile bool check_pad = false;

void core1_entry(void) {
    if (!bluetooth_init(NULL, usb_device_set_pad, usb_device_set_audio)) {
        panic("Bluetooth initialization failed on core 1\n");
        while (true) {
            sleep_ms(1000);
        }
    }
    while (true) {
        bluetooth_task();
    }
}

static bool check_pad_cb(repeating_timer_t *rt) {
    check_pad = true;
    return true;
}

static void launch_core1(void) {
    multicore_reset_core1();
    multicore_launch_core1(core1_entry);
}

static void full_reboot(void) {
    ogxm_logi("Full reboot requested\n");
    multicore_reset_core1();
    watchdog_reboot(0, 0, 0);
    while (1) { tight_loop_contents(); }
}

static void uart_bridge_task(void) {
    usb_device_config_t usbd_config = {
        .use_mutex = false,
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

    // usbd_type_t device_type = settings_get_device_type();
    usbd_type_t device_type = USBD_TYPE_XINPUT;
    usb_device_config_t usbd_config = {
        .use_mutex = true,
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

    launch_core1();

    repeating_timer_t gp_check_timer;
    add_repeating_timer_ms(COMBO_CHECK_INTERVAL_MS, check_pad_cb, 
                           NULL, &gp_check_timer);

    while (true) {
        usb_device_task();
        sleep_ms(1);

        if (check_pad) {
            check_pad = false;
            gamepad_pad_t pad = {0};
            if (!usb_device_get_pad_unsafe(0, &pad)) {
                continue;
            }
            usbd_type_t type = check_button_combo(0, &pad);
            if ((type != USBD_TYPE_COUNT) && (type != device_type)) {
                cancel_repeating_timer(&gp_check_timer);
                multicore_reset_core1();
                usb_device_deinit();
                settings_store_device_type(type);
                full_reboot();
            }
        }
    }
}

#endif // (OGXM_BOARD == OGXM_BOARD_BLUETOOTH)