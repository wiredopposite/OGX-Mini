#include <stdint.h>
#include "gamepad/gamepad.h"
#include "usb/host/drivers/hid/hid_usage.h"
#include "usb/host/drivers/hid/hid.h"

static const hid_usage_map_t HID_USAGE_MAP_PSCLASSIC = {
    .buttons = {
        [HID_USAGE_BUTTON_01] = GAMEPAD_BTN_BIT_Y,
        [HID_USAGE_BUTTON_02] = GAMEPAD_BTN_BIT_B,
        [HID_USAGE_BUTTON_03] = GAMEPAD_BTN_BIT_A,
        [HID_USAGE_BUTTON_04] = GAMEPAD_BTN_BIT_X,
        [HID_USAGE_BUTTON_05] = GAMEPAD_BTN_BIT_LT,
        [HID_USAGE_BUTTON_06] = GAMEPAD_BTN_BIT_RT,
        [HID_USAGE_BUTTON_07] = GAMEPAD_BTN_BIT_LB,
        [HID_USAGE_BUTTON_08] = GAMEPAD_BTN_BIT_RB,
        [HID_USAGE_BUTTON_09] = GAMEPAD_BTN_BIT_BACK,
        [HID_USAGE_BUTTON_10] = GAMEPAD_BTN_BIT_START,
        [HID_USAGE_BUTTON_11] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_12] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_13] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_14] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_15] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_16] = GAMEPAD_BTN_BIT_NONE,
    }
};

static bool psc_desktop_quirk_cb(uint8_t usage, uint32_t value, gamepad_pad_t* pad, 
                                 const user_profile_t* profile) {
    switch (usage) {
    case HID_DESKTOP_USAGE_X:
        switch (value) {
        case 0:
            pad->dpad |= GP_BIT8(profile->d_left);
            break;
        case 2:
            pad->dpad |= GP_BIT8(profile->d_right);
            break;
        }
        break;
    case HID_DESKTOP_USAGE_Y:
        switch (value) {
        case 0:
            pad->dpad |= GP_BIT8(profile->d_up);
            break;
        case 2:
            pad->dpad |= GP_BIT8(profile->d_down);
            break;
        }
        break;
    default:
        break;
    }
    return true;
}

const hidh_driver_t HIDH_DRIVER_PSCLASSIC = {
    .name = "PSClassic",
    .init_cb = NULL,
    .set_led_cb = NULL,
    .set_rumble_cb = NULL,
    .desktop_quirk_cb = psc_desktop_quirk_cb,
    .button_quirk_cb = NULL,
    .vendor_quirk_cb = NULL,
    .usage_map = &HID_USAGE_MAP_PSCLASSIC,
};