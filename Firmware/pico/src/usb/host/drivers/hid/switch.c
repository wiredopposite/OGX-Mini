#include <stdint.h>
#include "gamepad/gamepad.h"
#include "usb/host/drivers/hid/hid_usage.h"
#include "usb/host/drivers/hid/hid.h"

static const hid_usage_map_t HID_USAGE_MAP_SWITCH = {
    .buttons = {
        [HID_USAGE_BUTTON_01] = GAMEPAD_BIT_X,
        [HID_USAGE_BUTTON_02] = GAMEPAD_BIT_A,
        [HID_USAGE_BUTTON_03] = GAMEPAD_BIT_B,
        [HID_USAGE_BUTTON_04] = GAMEPAD_BIT_Y,
        [HID_USAGE_BUTTON_05] = GAMEPAD_BIT_LB,
        [HID_USAGE_BUTTON_06] = GAMEPAD_BIT_RB,
        [HID_USAGE_BUTTON_07] = GAMEPAD_BIT_LT,
        [HID_USAGE_BUTTON_08] = GAMEPAD_BIT_RT,
        [HID_USAGE_BUTTON_09] = GAMEPAD_BIT_BACK,
        [HID_USAGE_BUTTON_10] = GAMEPAD_BIT_START,
        [HID_USAGE_BUTTON_11] = GAMEPAD_BIT_L3,
        [HID_USAGE_BUTTON_12] = GAMEPAD_BIT_R3,
        [HID_USAGE_BUTTON_13] = GAMEPAD_BIT_SYS,
        [HID_USAGE_BUTTON_14] = GAMEPAD_BIT_MISC,
        [HID_USAGE_BUTTON_15] = GAMEPAD_BIT_NONE,
        [HID_USAGE_BUTTON_16] = GAMEPAD_BIT_NONE,
    },
    .desktop_map = {
        { .usage = HID_DESKTOP_USAGE_X,  .type = GP_TYPE_INT16, .offset = GP_OFF_JOYSTICK_LX, .invert = false },
        { .usage = HID_DESKTOP_USAGE_Y,  .type = GP_TYPE_INT16, .offset = GP_OFF_JOYSTICK_LY, .invert = false },
        { .usage = HID_DESKTOP_USAGE_Z,  .type = GP_TYPE_INT16, .offset = GP_OFF_JOYSTICK_RX, .invert = false },
        { .usage = HID_DESKTOP_USAGE_RZ, .type = GP_TYPE_INT16, .offset = GP_OFF_JOYSTICK_RY, .invert = false },
        MAP_UNUSED,
        MAP_UNUSED,
    }
};

const hidh_driver_t HIDH_DRIVER_SWITCH = {
    .name = "Switch",
    .init_cb = NULL,
    .set_led_cb = NULL,
    .set_rumble_cb = NULL,
    .desktop_quirk_cb = NULL,
    .button_quirk_cb = NULL,
    .vendor_quirk_cb = NULL,
    .usage_map = &HID_USAGE_MAP_SWITCH,
};