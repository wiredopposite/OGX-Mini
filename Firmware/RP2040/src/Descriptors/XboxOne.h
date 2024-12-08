#ifndef _XBOX_ONE_DESCRIPTORS_H_
#define _XBOX_ONE_DESCRIPTORS_H_

#include <cstdint>

namespace XboxOne
{
    namespace Buttons0
    {
        static constexpr uint8_t SYNC = (1 << 0);
        static constexpr uint8_t GUIDE = (1 << 1);
        static constexpr uint8_t START = (1 << 2);
        static constexpr uint8_t BACK = (1 << 3);
        static constexpr uint8_t A = (1 << 4);
        static constexpr uint8_t B = (1 << 5);
        static constexpr uint8_t X = (1 << 6);
        static constexpr uint8_t Y = (1 << 7);
    };
    namespace Buttons1
    {
        static constexpr uint8_t DPAD_UP = (1 << 0);
        static constexpr uint8_t DPAD_DOWN = (1 << 1);
        static constexpr uint8_t DPAD_LEFT = (1 << 2);
        static constexpr uint8_t DPAD_RIGHT = (1 << 3);
        static constexpr uint8_t LB = (1 << 4);
        static constexpr uint8_t RB = (1 << 5);
        static constexpr uint8_t L3 = (1 << 6);
        static constexpr uint8_t R3 = (1 << 7);
    };

    #pragma pack(push, 1)
    struct InReport
    {
        struct GipHeader
        {
            uint8_t command;
            uint8_t client : 4;
            uint8_t needsAck : 1;
            uint8_t internal : 1;
            uint8_t chunkStart : 1;
            uint8_t chunked : 1;
            uint8_t sequence;
            uint8_t length;
        } header;

        uint8_t buttons[2];

        uint16_t trigger_l;
        uint16_t trigger_r;

        int16_t joystick_lx;
        int16_t joystick_ly;
        int16_t joystick_rx;
        int16_t joystick_ry;

        uint8_t reserved[18]; // 18-byte padding at the end
    };
    static_assert(sizeof(InReport) == 36, "XboxOne::InReport is not the correct size");
    #pragma pack(pop)

}; // namespace XboxOne

#endif // _XBOX_ONE_DESCRIPTORS_H_