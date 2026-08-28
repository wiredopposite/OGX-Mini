/* Dreamcast controller structures. Ported from DreamPicoPort (OrangeFox86).
 * https://github.com/OrangeFox86/DreamPicoPort */

#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

/* 8-byte controller condition (little endian). 0 = pressed for buttons. */
typedef struct controller_condition_s {
    uint8_t l;   /* 0 released .. 255 pressed */
    uint8_t r;
    unsigned z : 1;
    unsigned y : 1;
    unsigned x : 1;
    unsigned d : 1;
    unsigned upb : 1;
    unsigned downb : 1;
    unsigned leftb : 1;
    unsigned rightb : 1;
    unsigned c : 1;
    unsigned b : 1;
    unsigned a : 1;
    unsigned start : 1;
    unsigned up : 1;
    unsigned down : 1;
    unsigned left : 1;
    unsigned right : 1;
    uint8_t rAnalogUD;  /* 0 up, 128 neutral, 255 down */
    uint8_t rAnalogLR;
    uint8_t lAnalogUD;
    uint8_t lAnalogLR;
} __attribute__((packed)) controller_condition_t;

#ifdef __cplusplus
}
#endif
