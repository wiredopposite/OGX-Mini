#include <stdint.h>

#include "usbh/tusb_hid/shared.h"

int16_t scale_uint8_to_int16(uint8_t value, bool invert) 
{
    const uint32_t scaling_factor = UINT16_MAX;
    const int32_t bias = INT16_MIN;

    int32_t scaled_value = ((uint32_t)value * scaling_factor) >> 8;
    scaled_value += bias;

    if (invert)
    {
        scaled_value = -scaled_value - 1;
    }

    if (scaled_value < INT16_MIN)
    {
        scaled_value = INT16_MIN;
    } 
    else if (scaled_value > INT16_MAX) 
    {
        scaled_value = INT16_MAX;
    }

    return (int16_t)scaled_value;
}