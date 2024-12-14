#ifndef _SCALE_H_
#define _SCALE_H_

#include <cstdint>

namespace INT_16
{
    static constexpr int16_t MIN = INT16_MIN;
    static constexpr int16_t MID = 0;
    static constexpr int16_t MAX = INT16_MAX;
}
namespace UINT_16
{
    static constexpr uint16_t MIN = 0;
    static constexpr uint16_t MID = 0x8000;
    static constexpr uint16_t MAX = 0xFFFF;
}
namespace UINT_8
{
    static constexpr uint8_t  MAX  = 0xFF;
    static constexpr uint8_t  MID  = 0x80;
    static constexpr uint8_t  MIN  = 0x00;
}
namespace INT_10
{
    static constexpr int32_t  MIN  = -512;
    static constexpr int32_t  MAX  = 511;
}
namespace UINT_10
{
    static constexpr int32_t MAX = 1023;
}

namespace Scale //Scale and invert values
{
    static inline uint8_t invert_joy(uint8_t value) 
    {    
        return static_cast<uint8_t>(UINT_8::MAX - value); 
    }
    static inline int8_t invert_joy(int8_t value) 
    {    
        return (value == std::numeric_limits<int8_t>::min()) ? std::numeric_limits<int8_t>::max() : -value; 
    }
    static inline uint16_t invert_joy(uint16_t value) 
    {    
        return static_cast<uint16_t>(std::numeric_limits<uint16_t>::max() - value); 
    }
    static inline int16_t invert_joy(int16_t value) 
    {    
        return (value == std::numeric_limits<int16_t>::min()) ? std::numeric_limits<int16_t>::max() : -value; 
    }

    static inline uint8_t int16_to_uint8(int16_t value)
    {
        uint16_t shifted_value = static_cast<uint16_t>(value + UINT_16::MID);
        return static_cast<uint8_t>(shifted_value >> 8);
    }
    static inline uint16_t int16_to_uint16(int16_t value)
    {
        return static_cast<uint16_t>(value + UINT_16::MID);
    }
    static inline int8_t int16_to_int8(int16_t value)
    {
        return static_cast<int8_t>((value + UINT_16::MID) >> 8);
    }

    static inline uint8_t uint16_to_uint8(uint16_t value)
    {
        return static_cast<uint8_t>(value >> 8);
    }
    static inline int16_t uint16_to_int16(uint16_t value)
    {
        return static_cast<int16_t>(value - UINT_16::MID);
    }
    static inline int8_t uint16_to_int8(uint16_t value)
    {
        return static_cast<int8_t>((value >> 8) - UINT_8::MID);
    }

    static inline int16_t uint8_to_int16(uint8_t value)
    {
        return static_cast<int16_t>((static_cast<int32_t>(value) << 8) - UINT_16::MID);
    }
    static inline uint16_t uint8_to_uint16(uint8_t value)
    {
        return static_cast<uint16_t>(value) << 8;
    }
    static inline int8_t uint8_to_int8(uint8_t value)
    {
        return static_cast<int8_t>(value - UINT_8::MID);
    }

    static inline int16_t int8_to_int16(int8_t value)
    {
        return static_cast<int16_t>(value) << 8;
    }
    static inline uint16_t int8_to_uint16(int8_t value)
    {
        return static_cast<uint16_t>((value + UINT_8::MID) << 8);
    }
    static inline uint8_t int8_to_uint8(int8_t value)
    {
        return static_cast<uint8_t>(value + UINT_8::MID);
    }

    static inline uint8_t int10_to_uint8(int32_t value)
    {
        value = value - INT_10::MIN;

        if (value >= UINT_10::MAX) 
        {
            return UINT_8::MAX;
        }
        else if (value <= 0) 
        {
            return 0;
        }
        return static_cast<uint8_t>(value >> 2);
    }
    static inline int16_t int10_to_int16(int32_t value)
    {
        constexpr int32_t scale_factor = INT_16::MAX - INT_16::MIN;
        constexpr int32_t range = INT_10::MAX - INT_10::MIN;

        if (value >= INT_10::MAX)
        {
            return INT_16::MAX;
        }
        else if (value <= INT_10::MIN)
        {
            return INT_16::MIN;
        }

        int32_t scaled_value = (value - INT_10::MIN) * scale_factor;
        return static_cast<int16_t>(scaled_value / range + INT_16::MIN);
    }
    static inline uint8_t uint10_to_uint8(int32_t value)
    {
        if (value > UINT_10::MAX) 
        {
            value = UINT_10::MAX;
        }
        else if (value < 0) 
        {
            value = 0;
        }
        return static_cast<uint8_t>(value >> 2);
    }
} // namespace Scale

#endif // _SCALE_H_