#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void led_init(void);
void led_set(uint8_t index, bool state);

#ifdef __cplusplus
}
#endif