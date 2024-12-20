#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <cstdint>

namespace Gamepad 
{
    namespace DPad
    {
        static constexpr uint8_t UP    = 0x01;
        static constexpr uint8_t DOWN  = 0x02;
        static constexpr uint8_t LEFT  = 0x04;
        static constexpr uint8_t RIGHT = 0x08;
        static constexpr uint8_t UP_LEFT    = UP | LEFT;
        static constexpr uint8_t UP_RIGHT   = UP | RIGHT;
        static constexpr uint8_t DOWN_LEFT  = DOWN | LEFT;
        static constexpr uint8_t DOWN_RIGHT = DOWN | RIGHT;
        static constexpr uint8_t NONE = 0x00;
    }
    namespace Button 
    {
        static constexpr uint16_t A     = 0x0001;
        static constexpr uint16_t B     = 0x0002;
        static constexpr uint16_t X     = 0x0004;
        static constexpr uint16_t Y     = 0x0008;
        static constexpr uint16_t L3    = 0x0010;
        static constexpr uint16_t R3    = 0x0020;
        static constexpr uint16_t BACK  = 0x0040;
        static constexpr uint16_t START = 0x0080;
        static constexpr uint16_t LB    = 0x0100;
        static constexpr uint16_t RB    = 0x0200;
        static constexpr uint16_t SYS   = 0x0400;
        static constexpr uint16_t MISC  = 0x0800;
    }

} // namespace Gamepad

namespace Scale 
{
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

#endif // GAMEPAD_H