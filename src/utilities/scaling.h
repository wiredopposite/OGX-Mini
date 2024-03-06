#pragma once

#ifndef _SCALING_H_
#define _SCALING_H_

#include <stdint.h>

uint8_t scale_int16_to_uint8(int16_t value, bool invert);
int16_t scale_uint8_to_int16(uint8_t value, bool invert);

#endif // _SCALING_H_