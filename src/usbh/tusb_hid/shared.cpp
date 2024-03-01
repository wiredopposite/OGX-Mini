#include <stdint.h>

#include "usbh/tusb_hid/shared.h"

int16_t scale_uint8_to_int16(uint8_t value, bool invert) 
{
    const uint32_t scaling_factor = 65535;
    const int32_t bias = -32768;

    int32_t scaled_value = ((uint32_t)value * scaling_factor) >> 8;
    scaled_value += bias;

    if (invert)
    {
        scaled_value = -scaled_value - 1;
    }

    if (scaled_value < -32768) 
    {
        scaled_value = -32768;
    } 
    else if (scaled_value > 32767) 
    {
        scaled_value = 32767;
    }

    return (int16_t)scaled_value;
}