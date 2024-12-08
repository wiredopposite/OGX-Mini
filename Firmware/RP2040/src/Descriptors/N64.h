#ifndef _N64_DESCRIPTORS_H_
#define _N64_DESCRIPTORS_H_

#include <cstdint>

namespace N64 
{
    static constexpr uint16_t DPAD_MASK = 0x0F;
    static constexpr uint8_t JOY_MIN = 0x00;
    static constexpr uint8_t JOY_MID = 0x80;
    static constexpr uint8_t JOY_MAX = 0xFF;

    namespace Buttons 
    {
        static constexpr uint16_t DPAD_UP         = 0x00;
        static constexpr uint16_t DPAD_UP_RIGHT   = 0x01;
        static constexpr uint16_t DPAD_RIGHT      = 0x02;
        static constexpr uint16_t DPAD_RIGHT_DOWN = 0x03;
        static constexpr uint16_t DPAD_DOWN       = 0x04;
        static constexpr uint16_t DPAD_DOWN_LEFT  = 0x05;
        static constexpr uint16_t DPAD_LEFT       = 0x06;
        static constexpr uint16_t DPAD_LEFT_UP    = 0x07;
        static constexpr uint16_t DPAD_NONE       = 0x08;

        static constexpr uint16_t C_UP    = (1 << 4);
        static constexpr uint16_t C_RIGHT = (1 << 5);
        static constexpr uint16_t C_DOWN  = (1 << 6);
        static constexpr uint16_t C_LEFT  = (1 << 7);
        static constexpr uint16_t L       = (1 << 8);
        static constexpr uint16_t R       = (1 << 9);
        static constexpr uint16_t A       = (1 << 10);
        static constexpr uint16_t Z       = (1 << 11);
        static constexpr uint16_t B       = (1 << 12);
        static constexpr uint16_t START   = (1 << 13);
    };

    #pragma pack(push, 1)
    struct InReport
    {
        uint8_t joystick_x;
        uint8_t joystick_y;
        uint8_t padding[3];
        uint16_t buttons;
    };
    static_assert(sizeof(InReport) == 7, "N64 InReport size is not correct");
    #pragma pack(pop)

}; // namespace N64

#endif // _N64_DESCRIPTORS_H_