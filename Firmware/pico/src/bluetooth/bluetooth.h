#pragma once

#include "board_config.h"
#if BLUETOOTH_ENABLED

#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bt_connect_cb_t)(uint8_t index, bool connected);
typedef void (*bt_gamepad_cb_t)(uint8_t index, const gamepad_pad_t* pad, uint32_t flags);
typedef void (*bt_audio_cb_t)(uint8_t index, const gamepad_pcm_out_t* pcm);

bool bluetooth_init(bt_connect_cb_t connect_cb, bt_gamepad_cb_t gamepad_cb, bt_audio_cb_t audio_cb);
void bluetooth_task(void);

/* Thread safe methods */

void bluetooth_set_rumble(uint8_t index, const gamepad_rumble_t* rumble);
void bluetooth_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_ENABLED