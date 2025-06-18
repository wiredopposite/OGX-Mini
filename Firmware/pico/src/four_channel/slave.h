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

typedef void (*four_ch_slave_en_cb_t)(bool enabled);

void four_ch_slave_init(uint8_t index, four_ch_slave_en_cb_t enabled_cb, gamepad_set_pad_cb_t gamepad_cb);
void four_ch_slave_set_rumble(const gamepad_rumble_t* rumble);
void four_ch_slave_task(void);

#ifdef __cplusplus
}
#endif

#endif // OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL