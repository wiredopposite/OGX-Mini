#include <stdint.h>
#include "gamepad/gamepad.h"
#include "usb/host/drivers/hid/hid_usage.h"
#include "usb/host/drivers/hid/hid.h"

static const hid_usage_map_t HID_USAGE_MAP_DINPUT = {
    .buttons = {
        [HID_USAGE_BUTTON_01]   = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_02]   = GAMEPAD_BTN_BIT_B,
        [HID_USAGE_BUTTON_03]   = GAMEPAD_BTN_BIT_A,
        [HID_USAGE_BUTTON_04]   = GAMEPAD_BTN_BIT_X,
        [HID_USAGE_BUTTON_05]   = GAMEPAD_BTN_BIT_Y,
        [HID_USAGE_BUTTON_06]   = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_07]   = GAMEPAD_BTN_BIT_LB,
        [HID_USAGE_BUTTON_08]   = GAMEPAD_BTN_BIT_RB,
        [HID_USAGE_BUTTON_09]   = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_10]   = GAMEPAD_BTN_BIT_NONE,
        [HID_USAGE_BUTTON_11]   = GAMEPAD_BTN_BIT_BACK,
        [HID_USAGE_BUTTON_12]   = GAMEPAD_BTN_BIT_START,
        [HID_USAGE_BUTTON_13]   = GAMEPAD_BTN_BIT_SYS,
        [HID_USAGE_BUTTON_14]   = GAMEPAD_BTN_BIT_L3,
        [HID_USAGE_BUTTON_15]   = GAMEPAD_BTN_BIT_R3,
        [HID_USAGE_BUTTON_16]   = GAMEPAD_BTN_BIT_NONE,
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

    // .vendor_map = {
    //     { .usage = HID_USAGE_VENDOR_0x20, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_RIGHT, .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x21, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_LEFT , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x22, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_UP   , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x23, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_DOWN , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x24, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_Y    , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x25, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_B    , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x26, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_A    , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x27, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_X    , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x28, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_LB   , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x29, .type = GP_TYPE_UINT8, .offset = GP_OFF_ANALOG_RB   , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x2A, .type = GP_TYPE_UINT8, .offset = GP_OFF_TRIGGER_L   , .invert = false },
    //     { .usage = HID_USAGE_VENDOR_0x2B, .type = GP_TYPE_UINT8, .offset = GP_OFF_TRIGGER_R   , .invert = false },
    // }

const hidh_driver_t HIDH_DRIVER_DINPUT = {
    .name = "DInput",
    .init_cb = NULL,
    .set_led_cb = NULL,
    .set_rumble_cb = NULL,
    .desktop_quirk_cb = NULL,
    .button_quirk_cb = NULL,
    .vendor_quirk_cb = NULL,
    .usage_map = &HID_USAGE_MAP_DINPUT,
};