#ifndef _PS5_DESCRIPTORS_H_
#define _PS5_DESCRIPTORS_H_

#include <cstdint>
#include <cstring>

namespace PS5
{
    static constexpr uint8_t DPAD_MASK = 0x0F;
    static constexpr uint8_t JOYSTICK_MID = 0x80;

    namespace OutReportID
    {
        static constexpr uint8_t CONTROL = 0x02;
        static constexpr uint8_t RUMBLE = 0x05;
    };

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
        static constexpr uint8_t MUTE = 0x04;
    };

    #pragma pack(push, 1)
    struct InReport
    {
        uint8_t report_id;

        uint8_t joystick_lx; 
        uint8_t joystick_ly; 
        uint8_t joystick_rx; 
        uint8_t joystick_ry; 
        uint8_t trigger_l;
        uint8_t trigger_r;

        uint8_t seq_number;

        uint8_t buttons[4];

        // Motion sensors
        uint16_t gyro[3]; // Gyroscope data for x, y, z axes
        uint16_t accel[3]; // Accelerometer data for x, y, z axes
        uint32_t sensor_timestamp; // Timestamp for sensor data
        uint8_t reserved0;

        struct Touchpad 
        {
            uint32_t counter : 7; // Incremented every time a finger touches the touchpad
            uint32_t touching : 1; // Indicates if a finger is currently touching the touchpad
            uint32_t x : 12; // X coordinate of the touchpoint
            uint32_t y : 12; // Y coordinate of the touchpoint
        } points[2]; // Array of touchpoints (up to 2)
        // uint32_t touchpad[2];

        uint8_t reserved1[12];

        uint8_t status; // ?

        uint8_t reserved2[10];

        InReport()
        {
            std::memset(this, 0, sizeof(InReport));
        }
    };
    static_assert(sizeof(InReport) == 60, "PS5::InReport is not correct size");

    struct OutReport
    {
        uint8_t report_id;
        uint8_t control_flag[2];

        /* For DualShock 4 compatibility mode. */
        uint8_t motor_right;
        uint8_t motor_left;

        /* Audio controls */
        uint8_t headphone_audio_volume; /* 0-0x7f */
        uint8_t speaker_audio_volume;   /* 0-255 */
        uint8_t internal_microphone_volume; /* 0-0x40 */
        uint8_t audio_flags;
        uint8_t mute_button_led;

        uint8_t power_save_control;

        /* right trigger motor */
        uint8_t right_trigger_motor_mode;
        uint8_t right_trigger_param[10];

        /* right trigger motor */
        uint8_t left_trigger_motor_mode;
        uint8_t left_trigger_param[10];

        uint8_t reserved2[4];

        uint8_t reduce_motor_power;
        uint8_t audio_flags2; /* 3 first bits: speaker pre-gain */

        /* LEDs and lightbar */
        uint8_t led_control_flag;
        uint8_t reserved3[2];
        uint8_t pulse_option;
        uint8_t led_brightness;
        uint8_t player_number;
        uint8_t lightbar_red;
        uint8_t lightbar_green;
        uint8_t lightbar_blue;

        OutReport()
        {
            std::memset(this, 0, sizeof(OutReport));
        }
    };
    static_assert(sizeof(OutReport) == 48);
    #pragma pack(pop)

}; // namespace PS5

#endif // _PS5_DESCRIPTORS_H_