#pragma once

#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL)

#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"
#include "gamepad/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

void four_ch_master_init(gamepad_set_rumble_cb_t rumble_cb);
void four_ch_master_set_connected(uint8_t index, bool connected);
void four_ch_master_set_pad(uint8_t index, const gamepad_pad_t* pad, uint32_t flags);
void four_ch_master_task(void);

#ifdef __cplusplus
}
#endif

#endif // OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL