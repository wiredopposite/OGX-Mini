#include <stdint.h>
#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "usb/host/drivers/hid/hid_usage.h"
#include "usb/host/drivers/hid/hid.h"

static const hid_usage_map_t HID_USAGE_MAP_N64 = {
    .buttons = {
        [HID_USAGE_BUTTON_01] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_02] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_03] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_04] = GAMEPAD_BTN_BIT_Y,
        [HID_USAGE_BUTTON_05] = GAMEPAD_BTN_BIT_LB,
        [HID_USAGE_BUTTON_06] = GAMEPAD_BTN_BIT_RB,
        [HID_USAGE_BUTTON_07] = GAMEPAD_BTN_BIT_A,
        [HID_USAGE_BUTTON_08] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_09] = GAMEPAD_BTN_BIT_B,
        [HID_USAGE_BUTTON_10] = GAMEPAD_BTN_BIT_START,
        [HID_USAGE_BUTTON_11] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_12] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_13] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_14] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_15] = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_16] = GAMEPAD_BTN_BIT_NONE,
    },
    .desktop_map = {
        { .usage = HID_DESKTOP_USAGE_X,  .type = GP_TYPE_INT16, .offset = GP_OFF_JOYSTICK_LX, .invert = false },
        { .usage = HID_DESKTOP_USAGE_Y,  .type = GP_TYPE_INT16, .offset = GP_OFF_JOYSTICK_LY, .invert = true },
        MAP_UNUSED,
        MAP_UNUSED,
        MAP_UNUSED,
        MAP_UNUSED,
    },
};

static bool n64_button_quirk_cb(uint8_t usage, uint32_t value, gamepad_pad_t* pad, 
                                const user_profile_t* profile) {
    switch (usage) {
    case HID_USAGE_BUTTON_01:
        pad->joystick_ry = (value ? R_INT16_MAX : pad->joystick_ry);
        return true;
    case HID_USAGE_BUTTON_02:
        pad->joystick_rx = (value ? R_INT16_MAX : pad->joystick_rx);
        return true;
    case HID_USAGE_BUTTON_03:
        pad->joystick_ry = (value ? R_INT16_MIN : pad->joystick_ry);
        return true;
    case HID_USAGE_BUTTON_04:
        pad->joystick_rx = (value ? R_INT16_MIN : pad->joystick_rx);
        return true;
    case HID_USAGE_BUTTON_08:
        pad->buttons |= (value ? GP_BIT16(profile->btn_rt) : 0);
        return true;
    default:
        break;
    }
    return false;
}

const hidh_driver_t HIDH_DRIVER_N64 = {
    .name = "N64",
    .init_cb = NULL,
    .set_led_cb = NULL,
    .set_rumble_cb = NULL,
    .desktop_quirk_cb = NULL,
    .button_quirk_cb = n64_button_quirk_cb,
    .vendor_quirk_cb = NULL,
    .usage_map = &HID_USAGE_MAP_N64,
};