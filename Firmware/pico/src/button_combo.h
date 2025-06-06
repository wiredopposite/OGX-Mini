#pragma once

#include <stdint.h>
#include "gamepad/gamepad.h"
#include "usb/device/device.h"

#define COMBO_CHECK_INTERVAL_MS     600U
#define COMBO_CHECK_COUNT           ((3U * 1000) / COMBO_CHECK_INTERVAL_MS)

#define BUTTON_COMBO(dpad, buttons) (((uint32_t)(dpad) << 16) | (buttons))
#define BUTTON_COMBO_NONE           ((uint32_t)0xFFFFFFFFU)

static const uint32_t BUTTON_COMBOS[USBD_TYPE_COUNT] = {
    [USBD_TYPE_XBOXOG_GP]   = BUTTON_COMBO(GAMEPAD_D_RIGHT, GAMEPAD_BTN_START),
    [USBD_TYPE_XBOXOG_SB]   = BUTTON_COMBO(GAMEPAD_D_RIGHT, GAMEPAD_BTN_START | GAMEPAD_BTN_RB),
    [USBD_TYPE_XBOXOG_XR]   = BUTTON_COMBO(GAMEPAD_D_RIGHT, GAMEPAD_BTN_START | GAMEPAD_BTN_LB),
    [USBD_TYPE_XINPUT]      = BUTTON_COMBO(GAMEPAD_D_UP,    GAMEPAD_BTN_START),
    [USBD_TYPE_PS3]         = BUTTON_COMBO(GAMEPAD_D_LEFT,  GAMEPAD_BTN_START),
    [USBD_TYPE_PSCLASSIC]   = BUTTON_COMBO(0,               GAMEPAD_BTN_START | GAMEPAD_BTN_A),
    [USBD_TYPE_SWITCH]      = BUTTON_COMBO(GAMEPAD_D_DOWN,  GAMEPAD_BTN_START),
    [USBD_TYPE_WEBAPP]      = BUTTON_COMBO(0,               GAMEPAD_BTN_START | GAMEPAD_BTN_LB | GAMEPAD_BTN_RB),
    [USBD_TYPE_UART_BRIDGE] = BUTTON_COMBO_NONE,
};

static inline usbd_type_t check_button_combo(uint8_t index, const gamepad_pad_t* pad) {
    if ((index >= GAMEPADS_MAX) || (pad == NULL)) {
        return USBD_TYPE_COUNT;
    }
    static uint8_t check_count[GAMEPADS_MAX] = { 0 };
    static uint32_t last_combo[GAMEPADS_MAX] = { BUTTON_COMBO_NONE };

    if (BUTTON_COMBO(pad->dpad, pad->buttons) == last_combo[index]) {
        check_count[index]++;
    } else {
        check_count[index] = 0;
        last_combo[index] = BUTTON_COMBO(pad->dpad, pad->buttons);
        return USBD_TYPE_COUNT;
    }
    if (check_count[index] >= COMBO_CHECK_COUNT) {
        check_count[index] = 0;
        for (uint8_t type = 0; type < USBD_TYPE_COUNT; type++) {
            if ((BUTTON_COMBOS[type] == last_combo[index])) {
                return (usbd_type_t)type;
            }
        }
    }
    return USBD_TYPE_COUNT;
}