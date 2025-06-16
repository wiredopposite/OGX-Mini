#include <pico/time.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include "log/log.h"
#include "settings/button_combo.h"
#include "settings/settings.h"
#include "shared.h"

static volatile bool check_pad_time_up = false;

static bool check_pad_timer_cb(repeating_timer_t *rt) {
    (void)rt;
    check_pad_time_up = true;
    return true;
}

void check_pad_timer_set_enabled(bool enabled) {
    static repeating_timer_t timer;
    static bool timer_running = false;

    if (enabled && !timer_running) {
        timer_running = true;
        add_repeating_timer_ms(COMBO_CHECK_INTERVAL_MS, 
                               check_pad_timer_cb, 
                               NULL, &timer);
    } else if (!enabled && timer_running) {
        timer_running = false;
        cancel_repeating_timer(&timer);
    }
}

bool check_pad_time(void) {
    if (check_pad_time_up) {
        check_pad_time_up = false;
        return true;
    }
    return false;
}

void check_pad_for_driver_change(uint8_t index, usbd_type_t device_type) {
    gamepad_pad_t pad = {0};
    if (!usb_device_get_pad(index, &pad)) {
        return;
    }
    usbd_type_t type = check_button_combo(0, &pad);
    if ((type != USBD_TYPE_COUNT) && (type != device_type)) {
        check_pad_timer_set_enabled(false);
        multicore_reset_core1();
        usb_device_deinit();
        settings_store_device_type(type);
        /*  Eventually rewrite or modify the host stack to be able to 
            cleanly deinit and not need a full reboot */
        pico_full_reboot();
    }
}

void pico_full_reboot(void) {
    ogxm_logi("Full reboot requested\n");
    multicore_reset_core1();
    watchdog_reboot(0, 0, 0);
    while (1) { tight_loop_contents(); }
}