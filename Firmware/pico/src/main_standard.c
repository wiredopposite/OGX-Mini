#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_STANDARD)

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/time.h>
#include <hardware/clocks.h>
#include <hardware/watchdog.h>
#include "led/led.h"
#include "bluetooth/bluetooth.h"
#include "usb/device/device.h"
#include "usb/host/host.h"
#include "gamepad/gamepad.h"
#include "settings/button_combo.h"
#include "settings/settings.h"
#include "log/log.h"

volatile bool check_pad = false;
volatile bool host_mounted = false;

static void host_connect_cb(uint8_t index, usbh_type_t type, bool connected) {
    ogxm_logi("USB host %s at index %d\n", 
        connected ? "connected" : "disconnected", index);
    host_mounted = connected;
    led_set_on(connected);
}

void core1_entry(void) {
    led_init();
    led_set_on(false);

    usb_host_config_t usbh_config = {
        .use_mutex = true,
        .hw_type = USBH_HW_PIO,
        .connect_cb = host_connect_cb,
        .gamepad_cb = usb_device_set_pad,
        .audio_cb = usb_device_set_audio,
    };
    usb_host_configure(&usbh_config);
    usb_host_enable();

    while (true) {
        usb_host_task();
        sleep_ms(1);
    }
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

static bool check_pad_timer_cb(repeating_timer_t *rt) {
    check_pad = host_mounted;
    return true;
}

static void init_device(usbd_type_t device_type) {
    usb_device_config_t usbd_config = {
        .use_mutex = true,
        .count = 1,
        .usb = {
            {
                .type = device_type,
                .addons = 0,
            }
        },
        .rumble_cb = usb_host_set_rumble,
        .audio_cb = usb_host_set_audio,
    };
    usb_device_configure(&usbd_config);
    usb_device_connect();
    ogxm_logi("USB device configured as %d\n", device_type);
}

int main(void) {
    set_sys_clock_khz(240000, true);
    ogxm_log_init(true);
    settings_init();
    
    usbd_type_t device_type = settings_get_device_type();

    launch_core1();
    init_device(device_type);
    
    ogxm_logi("Device inited\n");

    repeating_timer_t gp_check_timer;
    add_repeating_timer_ms(COMBO_CHECK_INTERVAL_MS, check_pad_timer_cb, 
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
                /*  Eventually write or modify the host stack to be able to 
                    cleanly deinit and not need a full reboot */
                full_reboot();
            }
        }
    }
}

#endif // (OGXM_BOARD == OGXM_BOARD_DEVKIT)