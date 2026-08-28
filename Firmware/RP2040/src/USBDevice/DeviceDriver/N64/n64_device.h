/* N64 controller device (output to N64 console). Ported from pdaxrom/usb2n64-adapter. */

#ifndef N64_DEVICE_H_
#define N64_DEVICE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 4-byte report: buttons1, buttons2, joy_x, joy_y (same format as N64 host input). */
typedef struct __attribute__((packed)) {
    uint8_t buttons1;
    uint8_t buttons2;
    int8_t joy_x;
    int8_t joy_y;
} N64Report;

/* Run on core1; blocks forever. data_pin = GPIO for N64 data line (e.g. 19). */
void n64_device_main(int data_pin);

/* Set report from Core0 (called by N64Device::process). */
void n64_device_set_report(const N64Report* report);

#ifdef __cplusplus
}
#endif

#endif /* N64_DEVICE_H_ */
