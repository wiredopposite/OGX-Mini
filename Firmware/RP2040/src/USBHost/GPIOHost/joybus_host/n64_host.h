#ifndef USBHOST_GPIOHOST_JOYBUS_N64_HOST_H_
#define USBHOST_GPIOHOST_JOYBUS_N64_HOST_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize N64 controller host (1-wire, same protocol as PicoGamepadConverter). Uses same controller.pio as JoyBus. */
void n64_host_init(uint pio_index, uint data_pin);

/** Poll N64 controller (sends 0x01, reads 4 bytes). Call from main loop when input source is N64_GPIO. */
void n64_host_poll(void);

/** Get pointer to last poll response (4 bytes: buttons1, buttons2, joy_x, joy_y). Valid until next n64_host_poll. */
const uint8_t* n64_host_get_response(void);

#ifdef __cplusplus
}
#endif

#endif /* USBHOST_GPIOHOST_JOYBUS_N64_HOST_H_ */
