#pragma once

#include <stdint.h>
#include "gamepad/gamepad.h"
#include "settings/device_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COMBO_CHECK_INTERVAL_MS     ((uint32_t)600U)
#define COMBO_CHECK_INTERVAL_US     (COMBO_CHECK_INTERVAL_MS * 1000U)
#define COMBO_HOLD_TIME_US          ((uint32_t)(3U * 1000U * 1000U))

usbd_type_t check_button_combo(uint8_t index, const gamepad_pad_t* pad);

#ifdef __cplusplus
}
#endif