#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GAMEPADS_MAX
#define GAMEPADS_MAX CONFIG_BLUEPAD32_MAX_DEVICES
#endif

#define GAMPAD_PCM_FRAME_SIZE       16U

typedef enum {
    GAMEPAD_DPAD_BIT_UP = 0,
    GAMEPAD_DPAD_BIT_DOWN,
    GAMEPAD_DPAD_BIT_LEFT,
    GAMEPAD_DPAD_BIT_RIGHT,
    GAMEPAD_DPAD_BIT_COUNT,
    GAMEPAD_DPAD_BIT_NONE = GAMEPAD_DPAD_BIT_COUNT,
} gamepad_dpad_bit_t;

typedef enum {
    GAMEPAD_BTN_BIT_A = 0,
    GAMEPAD_BTN_BIT_B,
    GAMEPAD_BTN_BIT_X,
    GAMEPAD_BTN_BIT_Y,
    GAMEPAD_BTN_BIT_L3,
    GAMEPAD_BTN_BIT_R3,
    GAMEPAD_BTN_BIT_LB,
    GAMEPAD_BTN_BIT_RB,
    GAMEPAD_BTN_BIT_LT,
    GAMEPAD_BTN_BIT_RT,
    GAMEPAD_BTN_BIT_BACK,
    GAMEPAD_BTN_BIT_START,
    GAMEPAD_BTN_BIT_SYS,
    GAMEPAD_BTN_BIT_MISC,
    GAMEPAD_BTN_BIT_COUNT,
    GAMEPAD_BTN_BIT_NONE = GAMEPAD_BTN_BIT_COUNT,
} gamepad_btn_bit_t;

typedef enum {
    GAMEPAD_ANALOG_UP = 0,
    GAMEPAD_ANALOG_DOWN,
    GAMEPAD_ANALOG_LEFT,
    GAMEPAD_ANALOG_RIGHT,
    GAMEPAD_ANALOG_A,
    GAMEPAD_ANALOG_B,
    GAMEPAD_ANALOG_X,
    GAMEPAD_ANALOG_Y,
    GAMEPAD_ANALOG_LB,
    GAMEPAD_ANALOG_RB,
    GAMEPAD_ANALOG_COUNT
} gampead_analog_t;

#define GP_BIT8(bit)           ((uint8_t)1U << (bit))
#define GP_BIT16(bit)          ((uint16_t)1U << (bit))

#define GAMEPAD_D_UP        GP_BIT8(GAMEPAD_DPAD_BIT_UP)
#define GAMEPAD_D_DOWN      GP_BIT8(GAMEPAD_DPAD_BIT_DOWN)
#define GAMEPAD_D_LEFT      GP_BIT8(GAMEPAD_DPAD_BIT_LEFT)
#define GAMEPAD_D_RIGHT     GP_BIT8(GAMEPAD_DPAD_BIT_RIGHT)

#define GAMEPAD_BTN_A       GP_BIT16(GAMEPAD_BTN_BIT_A)
#define GAMEPAD_BTN_B       GP_BIT16(GAMEPAD_BTN_BIT_B)
#define GAMEPAD_BTN_X       GP_BIT16(GAMEPAD_BTN_BIT_X)
#define GAMEPAD_BTN_Y       GP_BIT16(GAMEPAD_BTN_BIT_Y)
#define GAMEPAD_BTN_L3      GP_BIT16(GAMEPAD_BTN_BIT_L3)
#define GAMEPAD_BTN_R3      GP_BIT16(GAMEPAD_BTN_BIT_R3)
#define GAMEPAD_BTN_LB      GP_BIT16(GAMEPAD_BTN_BIT_LB)
#define GAMEPAD_BTN_RB      GP_BIT16(GAMEPAD_BTN_BIT_RB)
#define GAMEPAD_BTN_LT      GP_BIT16(GAMEPAD_BTN_BIT_LT)
#define GAMEPAD_BTN_RT      GP_BIT16(GAMEPAD_BTN_BIT_RT)
#define GAMEPAD_BTN_BACK    GP_BIT16(GAMEPAD_BTN_BIT_BACK)
#define GAMEPAD_BTN_START   GP_BIT16(GAMEPAD_BTN_BIT_START)
#define GAMEPAD_BTN_SYS     GP_BIT16(GAMEPAD_BTN_BIT_SYS)
#define GAMEPAD_BTN_MISC    GP_BIT16(GAMEPAD_BTN_BIT_MISC)

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t  dpad;
    uint8_t  reserved0;
    uint16_t buttons;
    uint8_t  trigger_l;
    uint8_t  trigger_r;
    int16_t  joystick_lx;
    int16_t  joystick_ly;
    int16_t  joystick_rx;
    int16_t  joystick_ry;
    uint8_t  analog[GAMEPAD_ANALOG_COUNT];
    // uint8_t  chatpad[3];
    // uint8_t  reserved1;
} gamepad_pad_t;
_Static_assert(sizeof(gamepad_pad_t) == 24, "Gamepad pad size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t l;
    uint8_t r;
    uint8_t l_duration;
    uint8_t r_duration;
} gamepad_rumble_t;
_Static_assert(sizeof(gamepad_rumble_t) == 4, "Gamepad rumble size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    int16_t  data[GAMPAD_PCM_FRAME_SIZE];
    uint16_t samples;
    uint16_t reserved;
} gamepad_pcm_t, gamepad_pcm_in_t, gamepad_pcm_out_t;
_Static_assert(sizeof(gamepad_pcm_t) == 36, "Gamepad PCM size mismatch");

/* ---- IN/OUT Data Flags ---- */

#define GAMEPAD_FLAG_IN_PAD            ((uint32_t)1 << 0) /* New Device->Host pad data */
#define GAMEPAD_FLAG_IN_PAD_ANALOG     ((uint32_t)1 << 1) /* New Device->Host pad data has analog data */
#define GAMEPAD_FLAG_IN_CHATPAD        ((uint32_t)1 << 2) /* New Device->Host chatpad data */
#define GAMEPAD_FLAG_IN_PCM            ((uint32_t)1 << 3) /* New Device->Host PCM data */
#define GAMEPAD_FLAG_OUT_PCM           ((uint32_t)1 << 4) /* New Host->Device PCM data */
#define GAMEPAD_FLAG_OUT_RUMBLE        ((uint32_t)1 << 5) /* New Host->Device rumble data */
#define GAMEPAD_FLAG_OUT_RUMBLE_DUR    ((uint32_t)1 << 6) /* New Host->Device rumble has duration info */

#ifdef __cplusplus
}
#endif