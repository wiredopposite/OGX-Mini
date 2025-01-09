#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <cstdint>

#include "sdkconfig.h"
#include "UserSettings/UserProfile.h"
#include "UserSettings/UserSettings.h"
#include "Board/ogxm_log.h"

#define MAX_GAMEPADS CONFIG_BLUEPAD32_MAX_DEVICES
static_assert(  MAX_GAMEPADS > 0 && 
                MAX_GAMEPADS <= 4, 
                "MAX_GAMEPADS must be between 1 and 4");

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

namespace Gamepad
{
    static constexpr uint8_t DPAD_UP         = 0x01;
    static constexpr uint8_t DPAD_DOWN       = 0x02;
    static constexpr uint8_t DPAD_LEFT       = 0x04;
    static constexpr uint8_t DPAD_RIGHT      = 0x08;
    static constexpr uint8_t DPAD_UP_LEFT    = DPAD_UP | DPAD_LEFT;
    static constexpr uint8_t DPAD_UP_RIGHT   = DPAD_UP | DPAD_RIGHT;
    static constexpr uint8_t DPAD_DOWN_LEFT  = DPAD_DOWN | DPAD_LEFT;
    static constexpr uint8_t DPAD_DOWN_RIGHT = DPAD_DOWN | DPAD_RIGHT;
    static constexpr uint8_t DPAD_NONE       = 0x00;

    static constexpr uint16_t BUTTON_A     = 0x0001;
    static constexpr uint16_t BUTTON_B     = 0x0002;
    static constexpr uint16_t BUTTON_X     = 0x0004;
    static constexpr uint16_t BUTTON_Y     = 0x0008;
    static constexpr uint16_t BUTTON_L3    = 0x0010;
    static constexpr uint16_t BUTTON_R3    = 0x0020;
    static constexpr uint16_t BUTTON_BACK  = 0x0040;
    static constexpr uint16_t BUTTON_START = 0x0080;
    static constexpr uint16_t BUTTON_LB    = 0x0100;
    static constexpr uint16_t BUTTON_RB    = 0x0200;
    static constexpr uint16_t BUTTON_SYS   = 0x0400;
    static constexpr uint16_t BUTTON_MISC  = 0x0800;
}

class GamepadMapper
{
public:
    uint8_t DPAD_UP         = Gamepad::DPAD_UP        ;
    uint8_t DPAD_DOWN       = Gamepad::DPAD_DOWN      ;
    uint8_t DPAD_LEFT       = Gamepad::DPAD_LEFT      ;
    uint8_t DPAD_RIGHT      = Gamepad::DPAD_RIGHT     ;
    uint8_t DPAD_UP_LEFT    = Gamepad::DPAD_UP_LEFT   ;
    uint8_t DPAD_UP_RIGHT   = Gamepad::DPAD_UP_RIGHT  ;
    uint8_t DPAD_DOWN_LEFT  = Gamepad::DPAD_DOWN_LEFT ;
    uint8_t DPAD_DOWN_RIGHT = Gamepad::DPAD_DOWN_RIGHT;
    uint8_t DPAD_NONE       = Gamepad::DPAD_NONE      ;

    uint16_t BUTTON_A     = Gamepad::BUTTON_A    ;
    uint16_t BUTTON_B     = Gamepad::BUTTON_B    ;
    uint16_t BUTTON_X     = Gamepad::BUTTON_X    ;
    uint16_t BUTTON_Y     = Gamepad::BUTTON_Y    ;
    uint16_t BUTTON_L3    = Gamepad::BUTTON_L3   ;
    uint16_t BUTTON_R3    = Gamepad::BUTTON_R3   ;
    uint16_t BUTTON_BACK  = Gamepad::BUTTON_BACK ;
    uint16_t BUTTON_START = Gamepad::BUTTON_START;
    uint16_t BUTTON_LB    = Gamepad::BUTTON_LB   ;
    uint16_t BUTTON_RB    = Gamepad::BUTTON_RB   ;
    uint16_t BUTTON_SYS   = Gamepad::BUTTON_SYS  ;
    uint16_t BUTTON_MISC  = Gamepad::BUTTON_MISC ;

    GamepadMapper() = default;
    ~GamepadMapper() = default;

    void set_profile(const UserProfile& profile)
    {
        set_profile_options(profile);
        set_profile_mappings(profile);
        set_profile_deadzones(profile);
    }

    inline int16_t joystick_ly(int32_t value_10bit) 
    {
        int16_t value = int10_to_int16(value_10bit);
        value = profile_invert_ly_ ? invert_joy(value) : value;
        return (value <= dz_.joystick_l_neg || value >= dz_.joystick_l_pos) ? value : INT_16::MID;
    };

    inline int16_t joystick_lx(int32_t value_10bit) 
    {
        int16_t value = int10_to_int16(value_10bit);
        return (value <= dz_.joystick_l_neg || value >= dz_.joystick_l_pos) ? value : INT_16::MID;
    };

    inline int16_t joystick_ry(int32_t value_10bit) 
    {
        int16_t value = int10_to_int16(value_10bit);
        value = profile_invert_ry_ ? invert_joy(value) : value;
        return (value <= dz_.joystick_r_neg || value >= dz_.joystick_r_pos) ? value : INT_16::MID;
    };

    inline int16_t joystick_rx(int32_t value_10bit) 
    {
        int16_t value = int10_to_int16(value_10bit);
        return (value <= dz_.joystick_r_neg || value >= dz_.joystick_r_pos) ? value : INT_16::MID;
    };

    inline uint8_t trigger_l(int32_t value_10bit) 
    {
        uint8_t value = uint10_to_uint8(value_10bit);
        return (value >= dz_.trigger_l) ? value : UINT_8::MIN;
    };

    inline uint8_t trigger_r(int32_t value_10bit) 
    {
        uint8_t value = uint10_to_uint8(value_10bit);
        return (value >= dz_.trigger_r) ? value : UINT_8::MIN;
    };

private:
    bool profile_invert_ly_{false};
    bool profile_invert_ry_{false};

    struct Deadzones
    {
        uint8_t trigger_l{0};
        uint8_t trigger_r{0};
        int16_t joystick_l_neg{0};
        int16_t joystick_l_pos{0};
        int16_t joystick_r_neg{0};
        int16_t joystick_r_pos{0};
    } dz_;

    void set_profile_options(const UserProfile& profile)
    {
        profile_invert_ly_ = profile.invert_ly ? true : false;
        profile_invert_ry_ = profile.invert_ry ? true : false;
    }

    void set_profile_mappings(const UserProfile& profile)
    {
        DPAD_UP         = profile.dpad_up;
        DPAD_DOWN       = profile.dpad_down;
        DPAD_LEFT       = profile.dpad_left;
        DPAD_RIGHT      = profile.dpad_right;
        DPAD_UP_LEFT    = profile.dpad_up | profile.dpad_left;
        DPAD_UP_RIGHT   = profile.dpad_up | profile.dpad_right;
        DPAD_DOWN_LEFT  = profile.dpad_down | profile.dpad_left;
        DPAD_DOWN_RIGHT = profile.dpad_down | profile.dpad_right;
        DPAD_NONE       = 0;

        BUTTON_A     = profile.button_a;
        BUTTON_B     = profile.button_b;
        BUTTON_X     = profile.button_x;
        BUTTON_Y     = profile.button_y;
        BUTTON_L3    = profile.button_l3;
        BUTTON_R3    = profile.button_r3;
        BUTTON_BACK  = profile.button_back;
        BUTTON_START = profile.button_start;
        BUTTON_LB    = profile.button_lb;
        BUTTON_RB    = profile.button_rb;
        BUTTON_SYS   = profile.button_sys;
        BUTTON_MISC  = profile.button_misc;

        OGXM_LOG("Mappings: A: %i B: %i X: %i Y: %i L3: %i R3: %i BACK: %i START: %i LB: %i RB: %i SYS: %i MISC: %i\n", 
            BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y, BUTTON_L3, BUTTON_R3, BUTTON_BACK, BUTTON_START, BUTTON_LB, BUTTON_RB, BUTTON_SYS, BUTTON_MISC);
    }

    void set_profile_deadzones(const UserProfile& profile) //Deadzones in the profile are 0-255 (0-100%)
    {
        dz_.trigger_l = profile.dz_trigger_l;
        dz_.trigger_r = profile.dz_trigger_r;

        dz_.joystick_l_pos = uint8_to_int16(profile.dz_joystick_l / 2);
        dz_.joystick_l_neg = invert_joy(dz_.joystick_l_pos);
        dz_.joystick_r_pos = uint8_to_int16(profile.dz_joystick_r / 2);
        dz_.joystick_r_neg = invert_joy(dz_.joystick_r_pos);

        OGXM_LOG("Deadzones: TL: %i TR: %i JL: %i JR: %i\n", dz_.trigger_l, dz_.trigger_r, dz_.joystick_l_pos, dz_.joystick_r_pos);
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
    static inline int16_t uint8_to_int16(uint8_t value)
    {
        return static_cast<int16_t>((static_cast<int32_t>(value) << 8) - UINT_16::MID);
    }
    static inline int16_t invert_joy(int16_t value) 
    {    
        return (value == INT_16::MIN) ? INT_16::MAX : -value; 
    }

}; // class GamepadMapper

#endif // GAMEPAD_H