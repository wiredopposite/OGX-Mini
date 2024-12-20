#ifndef _TUH_XINPUT_CMD_H_
#define _TUH_XINPUT_CMD_H_

#include <cstdint>

#include "tusb.h"

namespace tuh_xinput
{
    namespace XboxOne
    {
        static constexpr uint8_t GIP_CMD_ACK        = 0x01;
        static constexpr uint8_t GIP_CMD_ANNOUNCE   = 0x02;
        static constexpr uint8_t GIP_CMD_IDENTIFY   = 0x04;
        static constexpr uint8_t GIP_CMD_POWER      = 0x05;
        static constexpr uint8_t GIP_CMD_AUTHENTICATE   = 0x06;
        static constexpr uint8_t GIP_CMD_VIRTUAL_KEY    = 0x07;
        static constexpr uint8_t GIP_CMD_RUMBLE     = 0x09;
        static constexpr uint8_t GIP_CMD_LED        = 0x0a;
        static constexpr uint8_t GIP_CMD_FIRMWARE   = 0x0c;
        static constexpr uint8_t GIP_CMD_INPUT      = 0x20;
        static constexpr uint8_t GIP_SEQ0           = 0x00;
        static constexpr uint8_t GIP_OPT_ACK        = 0x10;
        static constexpr uint8_t GIP_OPT_INTERNAL   = 0x20;

        constexpr uint8_t GIP_PL_LEN(uint8_t N) { return N; }

        static constexpr uint8_t GIP_PWR_ON     = 0x00;
        static constexpr uint8_t GIP_PWR_SLEEP  = 0x01;
        static constexpr uint8_t GIP_PWR_OFF    = 0x04;
        static constexpr uint8_t GIP_PWR_RESET  = 0x07;
        static constexpr uint8_t GIP_LED_ON     = 0x01;

        constexpr uint8_t BIT(uint8_t n) { return 1U << n; }
        
        constexpr uint8_t GIP_MOTOR_R()     { return BIT(0); }
        constexpr uint8_t GIP_MOTOR_L()     { return BIT(1); }
        constexpr uint8_t GIP_MOTOR_RT()    { return BIT(2); }
        constexpr uint8_t GIP_MOTOR_LT()    { return BIT(3); }
        constexpr uint8_t GIP_MOTOR_ALL()   { return GIP_MOTOR_R() | GIP_MOTOR_L() | GIP_MOTOR_RT() | GIP_MOTOR_LT(); }

        static constexpr uint8_t POWER_ON[]     = { GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(1), GIP_PWR_ON };
        static constexpr uint8_t S_INIT[]       = { GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(15), 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        static constexpr uint8_t S_LED_INIT[]   = { GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(3), 0x00, 0x01, 0x14 };
        static constexpr uint8_t EXTRA_INPUT_PACKET_INIT[] = { 0x4d, 0x10, GIP_SEQ0, 0x02, 0x07, 0x00 };
        static constexpr uint8_t PDP_LED_ON[]   = { GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(3), 0x00, GIP_LED_ON, 0x14 };
        static constexpr uint8_t PDP_AUTH[]     = { GIP_CMD_AUTHENTICATE, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(2), 0x01, 0x00 };
        static constexpr uint8_t RUMBLE[]       = { GIP_CMD_RUMBLE, 0x00, 0x00, GIP_PL_LEN(9), 0x00, GIP_MOTOR_ALL(), 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF };
    }

    namespace Xbox360
    {
        static constexpr uint8_t RUMBLE[]   = { 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        static constexpr uint8_t LED[]      = { 0x01, 0x03, 0x00 };

        namespace Chatpad
        {
            static constexpr uint16_t CMD_KEEPALIVE_1 = 0x1F;
            static constexpr uint16_t CMD_KEEPALIVE_2 = 0x1E;
            static constexpr uint16_t CMD_LEDS_ON_KEYPRESS = 0x1B;
            static constexpr uint16_t CMD_CAPSLOCK_1 = 0x8;
            static constexpr uint16_t CMD_SQUARE_1 = 0x9;
            static constexpr uint16_t CMD_PEOPLE = 0xB;
            static constexpr uint16_t CMD_BACKLIGHT = 0xC;
            static constexpr uint16_t CMD_CAPSLOCK_2 = 0x11;
            static constexpr uint16_t CMD_SQUARE_2 = 0x12;
            static constexpr uint16_t CMD_SQUARE_CAPSLOCK = 0x13;
            static constexpr uint16_t CMD_CIRCLE = 0x14;
            static constexpr uint16_t CMD_CIRCLE_CAPSLOCK = 0x15;
            static constexpr uint16_t CMD_CIRCLE_SQUARE = 0x16;
            static constexpr uint16_t CMD_CIRCLE_SQUARE_CAPSLOCK = 0x17;

            static constexpr tusb_control_request_t INIT_1   = { .bmRequestType = 0x40, .bRequest = 0xA9, .wValue = 0xA30C, .wIndex = 0x4423, .wLength = 0 };
            static constexpr tusb_control_request_t INIT_2   = { .bmRequestType = 0x40, .bRequest = 0xA9, .wValue = 0x2344, .wIndex = 0x7F03, .wLength = 0 };
            static constexpr tusb_control_request_t INIT_3   = { .bmRequestType = 0x40, .bRequest = 0xA9, .wValue = 0x5839, .wIndex = 0x6832, .wLength = 0 };
            static constexpr tusb_control_request_t INIT_4   = { .bmRequestType = 0xC0, .bRequest = 0xA1, .wValue = 0x0000, .wIndex = 0xE416, .wLength = 2 };
            static constexpr tusb_control_request_t INIT_5   = { .bmRequestType = 0x40, .bRequest = 0xA1, .wValue = 0x0000, .wIndex = 0xE416, .wLength = 2 };
            static constexpr tusb_control_request_t INIT_6   = { .bmRequestType = 0xC0, .bRequest = 0xA1, .wValue = 0x0000, .wIndex = 0xE416, .wLength = 2 };
            //wValue in xbox360_wired_chatpad_command can be replaced with CHATPAD_CMD constants for different functions
            static constexpr tusb_control_request_t CMD         = { .bmRequestType = 0x41, .bRequest = 0x00, .wValue = 0x0000, .wIndex = 0x0002, .wLength = 0 };
            static constexpr tusb_control_request_t KEEPALIVE_1 = { .bmRequestType = 0x41, .bRequest = 0x00, .wValue = 0x001F, .wIndex = 0x0002, .wLength = 0 };
            static constexpr tusb_control_request_t KEEPALIVE_2 = { .bmRequestType = 0x41, .bRequest = 0x00, .wValue = 0x001E, .wIndex = 0x0002, .wLength = 0 };
            static constexpr tusb_control_request_t LEDS_1B     = { .bmRequestType = 0x41, .bRequest = 0x00, .wValue = 0x001B, .wIndex = 0x0002, .wLength = 0 };
        }
    }

    namespace Xbox360W
    {
        static constexpr uint8_t LED[] = { 0x00, 0x00, 0x08, 0x40 };
        //Sending 0x00, 0x00, 0x08, 0x00 will permanently disable rumble until you do this:
        static constexpr uint8_t RUMBLE_ENABLE[] = { 0x00, 0x00, 0x08, 0x01 };
        static constexpr uint8_t RUMBLE[]        = { 0x00, 0x01, 0x0F, 0xC0, 0x00, 0x00, 0x00 };
        static constexpr uint8_t INQUIRE_PRESENT[] = { 0x08, 0x00, 0x0F, 0xC0 };
        static constexpr uint8_t CONTROLLER_INFO[] = { 0x00, 0x00, 0x00, 0x40 };
        static constexpr uint8_t UNKNOWN[]      = { 0x00, 0x00, 0x02, 0x80 };
        static constexpr uint8_t POWER_OFF[]    = { 0x00, 0x00, 0x08, 0xC0 };

        namespace Chatpad 
        {
            static constexpr uint8_t INIT[] = { 0x00, 0x00, 0x0C, 0x1B };
            static constexpr uint8_t KEEPALIVE_1[] = { 0x00, 0x00, 0x0C, 0x1F };
            static constexpr uint8_t KEEPALIVE_2[] = { 0x00, 0x00, 0x0C, 0x1E };
            static constexpr uint8_t LEDS_ON_KEYPRESS[] = { 0x00, 0x00, 0x0C, 0x1B };

            static constexpr uint8_t CAPSLOCK = 0x20;
            static constexpr uint8_t GREEN = 0x08;
            static constexpr uint8_t ORANGE = 0x10;
            static constexpr uint8_t MESSENGER = 0x01;
            /*  The controller feedbacks the currently set leds. The bitmask for these are in chatpad_mod.
                xbox360w_chatpad_led_ctrl is used to turn on/off a chatpad led. Byte 3 is set to chatpad_led_on[x] 
                or chatpad_led_off[x] to turn that respective led on or off.    */
            static constexpr uint8_t LED_CTRL[] = { 0x00, 0x00, 0x0C, 0x00 };
            static constexpr uint8_t MOD[] =     { CAPSLOCK, GREEN, ORANGE, MESSENGER };
            static constexpr uint8_t LED_ON[] =  { 0x08, 0x09, 0x0A, 0x0B };
            static constexpr uint8_t LED_OFF[] = { 0x00, 0x01, 0x02, 0x03 };
        }
    }

    namespace XboxOG
    {
        static const uint8_t RUMBLE[] = { 0x00, 0x06, 0x00, 0x00, 0x00, 0x00 };
    }

} // namespace tuh_xinput

#endif // _TUH_XINPUT_CMD_H_