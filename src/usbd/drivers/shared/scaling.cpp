#include <stdint.h>

#include "usbd/drivers/shared/scaling.h"

uint8_t scale_int16_to_uint8(int16_t value, bool invert)
{
    if (value == INT16_MIN && invert) 
    {
        return UINT8_MAX;
    }

    if (invert) 
    {
        value = -value;
    }

    uint32_t scaled_value = static_cast<uint32_t>(value - INT16_MIN) * UINT8_MAX / (INT16_MAX - INT16_MIN);

    if (scaled_value > UINT8_MAX) 
    {
        scaled_value = UINT8_MAX;
    }

    return static_cast<uint8_t>(scaled_value);
}