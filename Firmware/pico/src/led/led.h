#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These are not thread safe, just call from a single core */

void led_init(void);
void led_rgb_set_color(uint8_t r, uint8_t g, uint8_t b);
void led_set_on(bool on);

#ifdef __cplusplus
}
#endif