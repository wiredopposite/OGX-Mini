/* PS1/PS2 controller protocol (device side). Ported from PicoGamepadConverter by Loc15.
 * https://github.com/Loc15/PicoGamepadConverter
 * License: GPL-3.0. OGX-Mini uses this for GPIO output to PS1/PS2 console. */

#ifndef PS1PS2_PSX_SIMULATOR_H_
#define PS1PS2_PSX_SIMULATOR_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t buttons1;
    uint8_t buttons2;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
    uint8_t l2;
    uint8_t r2;
} PSXInputState;

/* Initialize the PS1/PS2 controller simulator. Uses PIO and GPIO; call once before launching core1.
 * pio: 0 = PIO0, 1 = PIO1.
 * data: pointer to shared state (written by main loop, read by simulator on core1).
 * reset_pio: callback to restart core1 (e.g. multicore_reset_core1 + multicore_launch_core1(psx_device_main)). */
void psx_device_init(unsigned pio, PSXInputState* data, void (*reset_pio)(void));

/* Entry for core1: run this in an infinite loop. Blocks. */
void psx_device_main(void);

/* When enabled, SEL IRQ only restarts PIO (no Core1 reset). Use with psx_device_poll() on Core0 (e.g. Pico W: BT on Core1, PS2 on Core0). */
void psx_device_set_poll_mode(bool enable);

/* SEL rising-edge check and PIO restart only; does not read CMD FIFO. Use from Core0 when Core1 runs psx_device_main() (Pico W PS2 + OPL). */
void psx_device_sel_restart_check(void);

/* One poll iteration; call from Core0 main loop when in poll mode. Returns 1 if a command was processed. */
int psx_device_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* PS1PS2_PSX_SIMULATOR_H_ */
