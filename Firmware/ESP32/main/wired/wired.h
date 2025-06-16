#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wired_begin_cb_t)(void);
typedef void (*wired_rumble_cb_t)(uint8_t index, const gamepad_rumble_t* rumble);
typedef void (*wired_audio_cb_t)(uint8_t index, const gamepad_pcm_out_t* pcm);

void wired_config(wired_rumble_cb_t rumble_cb, wired_audio_cb_t audio_cb);
void wired_task(void* args);
void wired_set_connected(uint8_t index, bool connected);
void wired_set_pad(uint8_t index, const gamepad_pad_t* pad, uint32_t flags);
void wired_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm) __attribute__((weak));

#ifdef __cplusplus
}
#endif