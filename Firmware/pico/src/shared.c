#include <stddef.h>
#include <pico/time.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>
#include "log/log.h"
#include "settings/settings.h"
#include "shared.h"

#define COMBO_CHECK_INTERVAL_MS     ((uint32_t)600U)
#define COMBO_CHECK_INTERVAL_US     (COMBO_CHECK_INTERVAL_MS * 1000U)
#define COMBO_HOLD_TIME_US          ((uint32_t)(3U * 1000U * 1000U))
#define BUTTON_COMBO_NONE           ((uint32_t)0xFFFFFFFFU)

static const uint32_t BUTTON_COMBOS[USBD_TYPE_COUNT] = {
    [USBD_TYPE_XBOXOG_GP]   = (GAMEPAD_BUTTON_START | GAMEPAD_DPAD_RIGHT),
    [USBD_TYPE_XBOXOG_SB]   = (GAMEPAD_BUTTON_START | GAMEPAD_DPAD_RIGHT | GAMEPAD_BUTTON_RB),
    [USBD_TYPE_XBOXOG_XR]   = (GAMEPAD_BUTTON_START | GAMEPAD_DPAD_RIGHT | GAMEPAD_BUTTON_LB),
    [USBD_TYPE_XINPUT]      = (GAMEPAD_BUTTON_START | GAMEPAD_DPAD_UP),
    [USBD_TYPE_PS3]         = (GAMEPAD_BUTTON_START | GAMEPAD_DPAD_LEFT),
    [USBD_TYPE_PSCLASSIC]   = (GAMEPAD_BUTTON_START | GAMEPAD_BUTTON_A),
    [USBD_TYPE_SWITCH]      = (GAMEPAD_BUTTON_START | GAMEPAD_DPAD_DOWN),
    [USBD_TYPE_WEBAPP]      = (GAMEPAD_BUTTON_START | GAMEPAD_BUTTON_LB | GAMEPAD_BUTTON_RB),
    [USBD_TYPE_UART_BRIDGE] = (BUTTON_COMBO_NONE),
};

static volatile bool check_pad_time_up = false;
static uint32_t check_time[GAMEPADS_MAX] = { 0 };
static uint32_t last_buttons[GAMEPADS_MAX] = { BUTTON_COMBO_NONE };

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

static usbd_type_t check_button_combo(uint8_t index, const gamepad_pad_t* pad) {
    if ((index >= GAMEPADS_MAX) || (pad == NULL)) {
        return USBD_TYPE_COUNT;
    }
    const uint32_t now = time_us_32();
    if (pad->buttons != last_buttons[index]) {
        check_time[index] = now;
        last_buttons[index] = pad->buttons;
        return USBD_TYPE_COUNT;
    }
    if ((now - check_time[index]) >= COMBO_HOLD_TIME_US) {
        check_time[index] = now;
        for (uint8_t type = 0; type < USBD_TYPE_COUNT; type++) {
            if ((BUTTON_COMBOS[type] == last_buttons[index])) {
                return (usbd_type_t)type;
            }
        }
    }
    return USBD_TYPE_COUNT;
}

void check_pad_for_driver_change(uint8_t index, usbd_type_t device_type) {
    gamepad_pad_t pad = {0};
    if (!usb_device_get_pad(index, &pad)) {
        return;
    }
    usbd_type_t change_type = check_button_combo(0, &pad);
    if ((change_type != USBD_TYPE_COUNT) && (change_type != device_type)) {
        check_pad_timer_set_enabled(false);
        multicore_reset_core1();
        usb_device_deinit();
        settings_store_device_type(change_type);
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