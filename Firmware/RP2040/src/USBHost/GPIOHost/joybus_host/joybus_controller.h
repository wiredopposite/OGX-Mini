#ifndef USBHOST_GPIOHOST_JOYBUS_JOYBUS_CONTROLLER_H_
#define USBHOST_GPIOHOST_JOYBUS_JOYBUS_CONTROLLER_H_

#include "hardware/pio.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize JoyBus host (GameCube controller). pio_index 0 or 1, pin = data line. */
void joybus_host_init(uint pio_index, uint pin);

/** Send request and read response. request/response lengths in bytes. Response written to internal buffer. */
void joybus_send_data(const uint8_t *request, uint8_t data_length, uint8_t response_length);

/** Get pointer to last response buffer (8 bytes for GameCube). Valid until next joybus_send_data. */
const uint8_t* joybus_get_response(void);

#ifdef __cplusplus
}
#endif

#endif /* USBHOST_GPIOHOST_JOYBUS_JOYBUS_CONTROLLER_H_ */
