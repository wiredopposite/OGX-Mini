#pragma once

#include "board_config.h"
#if BLUETOOTH_ENABLED

#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"
#include "gamepad/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Wont't return on Pico W if bluetooth is started successfully */
bool bluetooth_init(gamepad_connect_cb_t connect_cb, gamepad_pad_cb_t gamepad_cb, gamepad_pcm_cb_t pcm_cb);

void bluetooth_task(void);

void bluetooth_set_rumble(uint8_t index, const gamepad_rumble_t* rumble); /* Thread safe */

void bluetooth_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm); /* Thread safe */

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_ENABLED