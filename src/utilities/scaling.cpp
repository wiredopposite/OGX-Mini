#include "utilities/scaling.h"

// uint8_t scale_int16_to_uint8(int16_t value, bool invert)
// {
//     if (value == INT16_MIN && invert) 
//     {
//         return UINT8_MAX;
//     }
// 
//     if (invert) 
//     {
//         value = -value;
//     }
// 
//     uint32_t scaled_value = static_cast<uint32_t>(value - INT16_MIN) * UINT8_MAX / (INT16_MAX - INT16_MIN);
// 
//     if (scaled_value > UINT8_MAX) 
//     {
//         scaled_value = UINT8_MAX;
//     }
// 
//     return static_cast<uint8_t>(scaled_value);
// }

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