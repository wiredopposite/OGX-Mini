#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gamepad_connect_cb_t)(uint8_t index, bool connected);
typedef void (*gamepad_pad_cb_t)(uint8_t index, const gamepad_pad_t* pad);
typedef void (*gamepad_pcm_cb_t)(uint8_t index, const gamepad_pcm_t* pcm);
typedef void (*gamepad_rumble_cb_t)(uint8_t index, const gamepad_rumble_t* rumble);

#ifdef __cplusplus
}
#endif