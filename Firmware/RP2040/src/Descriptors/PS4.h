#ifndef _PS4_DESCRIPTORS_H_
#define _PS4_DESCRIPTORS_H_

#include <cstdint>

namespace PS4
{
    static constexpr uint8_t DPAD_MASK = 0x0F;
    static constexpr uint8_t COUNTER_MASK = 0xFC;
    static constexpr uint8_t AXIS_MAX = 0xFF;
    static constexpr uint8_t AXIS_MIN = 0x00;
    static constexpr uint8_t JOYSTICK_MID = 0x80;

    namespace Buttons0
    {
        static constexpr uint8_t DPAD_UP         = 0x00;
        static constexpr uint8_t DPAD_UP_RIGHT   = 0x01;
        static constexpr uint8_t DPAD_RIGHT      = 0x02;
        static constexpr uint8_t DPAD_RIGHT_DOWN = 0x03;
        static constexpr uint8_t DPAD_DOWN       = 0x04;
        static constexpr uint8_t DPAD_DOWN_LEFT  = 0x05;
        static constexpr uint8_t DPAD_LEFT       = 0x06;
        static constexpr uint8_t DPAD_LEFT_UP    = 0x07;
        static constexpr uint8_t DPAD_CENTER     = 0x08;

        static constexpr uint8_t SQUARE   = 0x10;
        static constexpr uint8_t CROSS    = 0x20;
        static constexpr uint8_t CIRCLE   = 0x40;
        static constexpr uint8_t TRIANGLE = 0x80;
    };

    namespace Buttons1
    {
        static constexpr uint8_t L1 = 0x01;
        static constexpr uint8_t R1 = 0x02;
        static constexpr uint8_t L2 = 0x04;
        static constexpr uint8_t R2 = 0x08;
        static constexpr uint8_t SHARE = 0x10;
        static constexpr uint8_t OPTIONS = 0x20;
        static constexpr uint8_t L3 = 0x40;
        static constexpr uint8_t R3 = 0x80;
    };

    namespace Buttons2
    {
        static constexpr uint8_t PS = 0x01;
        static constexpr uint8_t TP = 0x02;
    };

    #pragma pack(push, 1)
    struct InReport
    {
        uint8_t report_id;

        uint8_t joystick_lx;
        uint8_t joystick_ly;
        uint8_t joystick_rx;
        uint8_t joystick_ry;

        uint8_t buttons[3];

        uint8_t trigger_l;
        uint8_t trigger_r;
    };
    static_assert(sizeof(InReport) == 10);

    struct OutReport
    {
        uint8_t report_id;

        uint8_t set_rumble : 1;
        uint8_t set_led : 1;
        uint8_t set_led_blink : 1;
        uint8_t set_ext_write : 1;
        uint8_t set_left_volume : 1;
        uint8_t set_right_volume : 1;
        uint8_t set_mic_volume : 1;
        uint8_t set_speaker_volume : 1;
        uint8_t set_flags2;

        uint8_t reserved;

        uint8_t motor_right;
        uint8_t motor_left;

        uint8_t lightbar_red;
        uint8_t lightbar_green;
        uint8_t lightbar_blue;
        uint8_t lightbar_blink_on;
        uint8_t lightbar_blink_off;

        uint8_t ext_data[8];

        uint8_t volume_left;
        uint8_t volume_right;
        uint8_t volume_mic;
        uint8_t volume_speaker;

        uint8_t other[9];
    };
    static_assert(sizeof(OutReport) == 32);
    #pragma pack(pop)

    static const uint8_t LED_COLORS[][3] =
    {
        { 0x00, 0x00, 0x40 }, // Blue
        { 0x40, 0x00, 0x00 }, // Red
        { 0x00, 0x40, 0x00 }, // Green
        { 0x20, 0x00, 0x20 }, // Pink
    };

}; // namespace PS4

#endif // _PS4_DESCRIPTORS_H_