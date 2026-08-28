/* GameCube controller protocol (device side). Ported from PicoGamepadConverter by Loc15.
 * https://github.com/Loc15/PicoGamepadConverter
 * Single-wire JoyBus; output to GameCube/Wii console. */

#ifndef GAMECUBE_GC_SIMULATOR_H_
#define GAMECUBE_GC_SIMULATOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t buttons1;  /* A, B, X, Y, Start */
    uint8_t buttons2;  /* D-pad, Z, R, L, 0x80 */
    uint8_t lx;        /* left stick X, 0x80 center */
    uint8_t ly;        /* left stick Y */
    uint8_t rx;        /* right stick X (C-stick) */
    uint8_t ry;        /* right stick Y */
    uint8_t l;         /* L analog 0–255 */
    uint8_t r;         /* R analog 0–255 */
} GCReport;

/* Run on core1; blocks forever. pio: 0 or 1, data_pin: GPIO for DATA line (e.g. 19). */
void gc_device_main(unsigned pio, GCReport* data, int data_pin);

/* Entry for core1 when using OGX-Mini GameCube driver (uses report set in initialize). */
void gamecube_core1_entry(void);

#ifdef __cplusplus
}
#endif

#endif /* GAMECUBE_GC_SIMULATOR_H_ */
