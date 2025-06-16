#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "usb/device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

void pico_full_reboot(void);

void check_pad_timer_set_enabled(bool enabled);
bool check_pad_time(void);
void check_pad_for_driver_change(uint8_t index, usbd_type_t device_type);

#ifdef __cplusplus
}
#endif