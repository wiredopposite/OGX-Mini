#ifndef USBHOST_GPIOHOST_PSX_HOST_PSX_HOST_H_
#define USBHOST_GPIOHOST_PSX_HOST_PSX_HOST_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize PSX controller host. gpio_output = CMD pin (CLK=gpio_output+1, ATT=gpio_output+2); gpio_input = DATA. */
void psx_host_init(uint pio_index, uint gpio_output, uint gpio_input);

/** Copy latest 9-byte report if available and clear ready. Returns true if data was copied. */
bool psx_host_take_data(uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* USBHOST_GPIOHOST_PSX_HOST_PSX_HOST_H_ */
