#pragma once

#include <stdint.h>
#include <stddef.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAP_UNUSED { .usage = 0, .type = GP_TYPE_NONE, .offset = GP_OFF_NONE, .invert = false }

typedef enum {
    HID_ITEM_MAIN   = 0x0,
    HID_ITEM_GLOBAL = 0x1,
    HID_ITEM_LOCAL  = 0x2,
} hid_item_t;

typedef enum {
    HID_MAIN_INPUT          = 0x8,
    HID_MAIN_OUTPUT         = 0x9,
    HID_MAIN_FEATURE        = 0xB,
    HID_MAIN_COLLECTION     = 0xA,
    HID_MAIN_END_COLLECTION = 0xC,
} hid_main_item_t;

typedef enum {
    HID_GLOBAL_USAGE_PAGE   = 0x0,
    HID_GLOBAL_LOG_MIN      = 0x1,
    HID_GLOBAL_LOG_MAX      = 0x2,
    HID_GLOBAL_PHY_MIN      = 0x3,
    HID_GLOBAL_PHY_MAX      = 0x4,
    HID_GLOBAL_UNIT_EXP     = 0x5,
    HID_GLOBAL_UNIT         = 0x6,
    HID_GLOBAL_REPORT_SIZE  = 0x7,
    HID_GLOBAL_REPORT_ID    = 0x8,
    HID_GLOBAL_REPORT_COUNT = 0x9,
} hid_global_item_t;

typedef enum {
    HID_LOCAL_USAGE         = 0x0,
    HID_LOCAL_USAGE_MIN     = 0x1,
    HID_LOCAL_USAGE_MAX     = 0x2,
} hid_local_item_t;

typedef enum hid_usage_page_ {
    HID__USAGE_PAGE_GENERIC_DESKTOP          = 0x01,
    HID__USAGE_PAGE_SIMULATION_CONTROLS      = 0x02,
    HID__USAGE_PAGE_GENERIC_DEVICE_CONTROLS  = 0x06,
    HID__USAGE_PAGE_KEYBOARD_KEYPAD          = 0x07,
    HID__USAGE_PAGE_BUTTON                   = 0x09,
    HID__USAGE_PAGE_CONSUMER                 = 0x0c,
    HID__USAGE_PAGE_DIGITIZER                = 0x0d,
    HID__USAGE_PAGE_VENDOR                   = 0xff,
} hid_usage_page_t;

typedef enum {
    HID_USAGE_BUTTON_NONE = 0,
    HID_USAGE_BUTTON_01,
    HID_USAGE_BUTTON_02,
    HID_USAGE_BUTTON_03,
    HID_USAGE_BUTTON_04,
    HID_USAGE_BUTTON_05,
    HID_USAGE_BUTTON_06,
    HID_USAGE_BUTTON_07,
    HID_USAGE_BUTTON_08,
    HID_USAGE_BUTTON_09,
    HID_USAGE_BUTTON_10,
    HID_USAGE_BUTTON_11,
    HID_USAGE_BUTTON_12,
    HID_USAGE_BUTTON_13,
    HID_USAGE_BUTTON_14,
    HID_USAGE_BUTTON_15,
    HID_USAGE_BUTTON_16,
    HID_USAGE_BUTTON_COUNT,
} hid_button_usage_t;

typedef enum {
    HID_DESKTOP_USAGE_X             = 0x30,
    HID_DESKTOP_USAGE_Y             = 0x31,
    HID_DESKTOP_USAGE_Z             = 0x32,
    HID_DESKTOP_USAGE_RX            = 0x33,
    HID_DESKTOP_USAGE_RY            = 0x34,
    HID_DESKTOP_USAGE_RZ            = 0x35,
    HID_DESKTOP_USAGE_HAT           = 0x39,
    HID_DESKTOP_USAGE_DPAD_UP       = 0x90,
    HID_DESKTOP_USAGE_DPAD_DOWN     = 0x91,
    HID_DESKTOP_USAGE_DPAD_RIGHT    = 0x92,
    HID_DESKTOP_USAGE_DPAD_LEFT     = 0x93,
} hid_desktop_usage_t;

typedef enum hid_hat_t {
    HID_HAT_UP = 0,
    HID_HAT_UP_RIGHT,
    HID_HAT_RIGHT,
    HID_HAT_DOWN_RIGHT,
    HID_HAT_DOWN,
    HID_HAT_DOWN_LEFT,
    HID_HAT_LEFT,
    HID_HAT_UP_LEFT,
    HID_HAT_CENTER,
} hid_hat_t;

typedef enum {
    HID_USAGE_VENDOR_0x20 = 0x20,
    HID_USAGE_VENDOR_0x21 = 0x21,
    HID_USAGE_VENDOR_0x22 = 0x22,
    HID_USAGE_VENDOR_0x23 = 0x23,
    HID_USAGE_VENDOR_0x24 = 0x24,
    HID_USAGE_VENDOR_0x25 = 0x25,
    HID_USAGE_VENDOR_0x26 = 0x26,
    HID_USAGE_VENDOR_0x27 = 0x27,
    HID_USAGE_VENDOR_0x28 = 0x28,
    HID_USAGE_VENDOR_0x29 = 0x29,
    HID_USAGE_VENDOR_0x2A = 0x2A,
    HID_USAGE_VENDOR_0x2B = 0x2B,
} hid_vendor_usage_t;

typedef enum {
    GP_TYPE_NONE = 0,
    GP_TYPE_UINT8,
    GP_TYPE_INT16,
} gp_data_type_t;

typedef enum {
    GP_OFF_NONE = 0,
    GP_OFF_TRIGGER_L    = offsetof(gamepad_pad_t, trigger_l),
    GP_OFF_TRIGGER_R    = offsetof(gamepad_pad_t, trigger_r),
    GP_OFF_JOYSTICK_LX  = offsetof(gamepad_pad_t, joystick_lx),
    GP_OFF_JOYSTICK_LY  = offsetof(gamepad_pad_t, joystick_ly),
    GP_OFF_JOYSTICK_RX  = offsetof(gamepad_pad_t, joystick_rx),
    GP_OFF_JOYSTICK_RY  = offsetof(gamepad_pad_t, joystick_ry),
    GP_OFF_ANALOG_UP    = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_UP]),
    GP_OFF_ANALOG_DOWN  = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_DOWN]),
    GP_OFF_ANALOG_LEFT  = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_LEFT]),
    GP_OFF_ANALOG_RIGHT = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_RIGHT]),
    GP_OFF_ANALOG_A     = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_A]),
    GP_OFF_ANALOG_B     = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_B]),
    GP_OFF_ANALOG_X     = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_X]),
    GP_OFF_ANALOG_Y     = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_Y]),
    GP_OFF_ANALOG_LB    = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_LB]),
    GP_OFF_ANALOG_RB    = offsetof(gamepad_pad_t, analog[GAMEPAD_ANALOG_RB]),
} gp_byte_offset_t;

typedef struct __attribute__((aligned(4))) {
    uint8_t         usage;
    gp_data_type_t  type;
    uint8_t         offset;
    bool            invert;
} hid_desktop_map_t, hid_vendor_map_t;

typedef struct __attribute__((aligned(4))) {
    uint8_t             buttons[HID_USAGE_BUTTON_COUNT];
    hid_desktop_map_t   desktop_map[6];
} hid_usage_map_t;

typedef struct __attribute__((aligned(4))) {
    uint8_t     usage_page; 
    uint8_t     usage;
    uint16_t    bit_offset;
    uint8_t     bit_size;
    int32_t     logical_min;
    int32_t     logical_max;
    uint8_t     button_index; 
} hid_field_t;

#ifdef __cplusplus
}
#endif