#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAMEPAD_BIT_UP = 0,
    GAMEPAD_BIT_DOWN,
    GAMEPAD_BIT_LEFT,
    GAMEPAD_BIT_RIGHT,
    GAMEPAD_BIT_A,
    GAMEPAD_BIT_B,
    GAMEPAD_BIT_X,
    GAMEPAD_BIT_Y,
    GAMEPAD_BIT_L3,
    GAMEPAD_BIT_R3,
    GAMEPAD_BIT_LB,
    GAMEPAD_BIT_RB,
    GAMEPAD_BIT_LT,
    GAMEPAD_BIT_RT,
    GAMEPAD_BIT_BACK,
    GAMEPAD_BIT_START,
    GAMEPAD_BIT_SYS,
    GAMEPAD_BIT_MISC,
    GAMEPAD_BIT_COUNT,
    GAMEPAD_BIT_NONE = GAMEPAD_BIT_COUNT,
} gamepad_bit_t;

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

#define GP_BIT(bit)             ((uint32_t)1U << (bit))

#define GAMEPAD_DPAD_UP         GP_BIT(GAMEPAD_BIT_UP)
#define GAMEPAD_DPAD_DOWN       GP_BIT(GAMEPAD_BIT_DOWN)
#define GAMEPAD_DPAD_LEFT       GP_BIT(GAMEPAD_BIT_LEFT)
#define GAMEPAD_DPAD_RIGHT      GP_BIT(GAMEPAD_BIT_RIGHT)

#define GAMEPAD_BUTTON_UP       GP_BIT(GAMEPAD_BIT_UP)
#define GAMEPAD_BUTTON_DOWN     GP_BIT(GAMEPAD_BIT_DOWN)
#define GAMEPAD_BUTTON_LEFT     GP_BIT(GAMEPAD_BIT_LEFT)
#define GAMEPAD_BUTTON_RIGHT    GP_BIT(GAMEPAD_BIT_RIGHT)
#define GAMEPAD_BUTTON_A        GP_BIT(GAMEPAD_BIT_A)
#define GAMEPAD_BUTTON_B        GP_BIT(GAMEPAD_BIT_B)
#define GAMEPAD_BUTTON_X        GP_BIT(GAMEPAD_BIT_X)
#define GAMEPAD_BUTTON_Y        GP_BIT(GAMEPAD_BIT_Y)
#define GAMEPAD_BUTTON_L3       GP_BIT(GAMEPAD_BIT_L3)
#define GAMEPAD_BUTTON_R3       GP_BIT(GAMEPAD_BIT_R3)
#define GAMEPAD_BUTTON_LB       GP_BIT(GAMEPAD_BIT_LB)
#define GAMEPAD_BUTTON_RB       GP_BIT(GAMEPAD_BIT_RB)
#define GAMEPAD_BUTTON_LT       GP_BIT(GAMEPAD_BIT_LT)
#define GAMEPAD_BUTTON_RT       GP_BIT(GAMEPAD_BIT_RT)
#define GAMEPAD_BUTTON_BACK     GP_BIT(GAMEPAD_BIT_BACK)
#define GAMEPAD_BUTTON_START    GP_BIT(GAMEPAD_BIT_START)
#define GAMEPAD_BUTTON_SYS      GP_BIT(GAMEPAD_BIT_SYS)
#define GAMEPAD_BUTTON_MISC     GP_BIT(GAMEPAD_BIT_MISC)

#define GAMEPAD_FLAG_PAD        ((uint8_t)1 << 0)
#define GAMEPAD_FLAG_ANALOG     ((uint8_t)1 << 1)
#define GAMEPAD_FLAG_CHATPAD    ((uint8_t)1 << 2)
// #define GAMEPAD_FLAG_GYRO       ((uint8_t)1 << 3)
// #define GAMEPAD_FLAG_ACCEL      ((uint8_t)1 << 4)

typedef struct __attribute__((packed, aligned(4))) {
    union {
        struct {
            uint32_t dpad : 4;
            uint32_t reserved0 : 28;
        };
        uint32_t buttons;
    };
    int16_t  joystick_lx;
    int16_t  joystick_ly;
    int16_t  joystick_rx;
    int16_t  joystick_ry;
    uint8_t  trigger_l;
    uint8_t  trigger_r;
    uint8_t  flags;
    uint8_t  analog[GAMEPAD_ANALOG_COUNT];
    uint8_t  chatpad[3];
    // int16_t  gyro_x; 
    // int16_t  gyro_y; 
    // int16_t  gyro_z; 
    // int16_t  accel_x;
    // int16_t  accel_y;
    // int16_t  accel_z;
} gamepad_pad_t;
_Static_assert(sizeof(gamepad_pad_t) == 28, "Gamepad pad size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    union {
        struct {
            uint32_t dpad : 4;
            uint32_t reserved0 : 28;
        };
        uint32_t buttons;
    };
    int16_t  joystick_lx;
    int16_t  joystick_ly;
    int16_t  joystick_rx;
    int16_t  joystick_ry;
    uint8_t  trigger_l;
    uint8_t  trigger_r;
    uint8_t  flags;
    uint8_t  reserved;
} gamepad_pad_short_t;
_Static_assert(sizeof(gamepad_pad_short_t) == 16, "Gamepad pad size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t l; // 0-255
    uint8_t r; // 0-255
    uint8_t l_duration;
    uint8_t r_duration;
} gamepad_rumble_t;
_Static_assert(sizeof(gamepad_rumble_t) == 4, "Gamepad rumble size mismatch");

#define GAMPAD_PCM_FRAME_SIZE 16U

typedef struct __attribute__((packed, aligned(4))) {
    int16_t  data[GAMPAD_PCM_FRAME_SIZE];
    uint16_t samples;
    uint16_t reserved;
} gamepad_pcm_out_t, gamepad_pcm_in_t, gamepad_pcm_t;
_Static_assert(sizeof(gamepad_pcm_out_t) == 36, "Gamepad PCM out size mismatch");

#ifdef __cplusplus
}
#endif