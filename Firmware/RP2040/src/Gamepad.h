#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#include <cstdint>
#include <atomic>
#include <limits>
#include <array>
#include <cstring>

#include "UserSettings/UserProfile.h"

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

class Gamepad 
{
public:
    struct DPad
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
    };

    struct Button 
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
    };

    using Chatpad = std::array<uint8_t, 3>;

    Gamepad()
    {
        reset_pad();
        reset_rumble();
        setup_deadzones();
    };

    ~Gamepad() = default;

    class Values //Store and scale values
    {
    public:
        Values() : type_(Type::UINT8), uint8_(0) {};
        ~Values() = default;

        inline int16_t int16(const bool invert = false) const
        {
            int16_t result = 0;
            switch (type_.load())
            {
                case Type::INT16:
                    result = int16_.load();
                    break;
                case Type::UINT16:
                    result = uint16_to_int16(uint16_.load());
                    break;
                case Type::UINT8:
                    result = uint8_to_int16(uint8_.load());
                    break;
                case Type::INT8:
                    result = int8_to_int16(int8_.load());
                    break;
                default:
                    break;
            }
            return invert ? invert_joy(result) : result;
        }

        inline uint16_t uint16(const bool invert = false) const
        {
            uint16_t result = 0;
            switch (type_.load())
            {
                case Type::UINT16:
                    result = uint16_.load();
                    break;
                case Type::INT16:
                    result = int16_to_uint16(int16_.load());
                    break;
                case Type::UINT8:
                    result = uint8_to_uint16(uint8_.load());
                    break;
                case Type::INT8:
                    result = int8_to_uint16(int8_.load());
                    break;
            }

            return invert ? invert_joy(result) : result;
        }

        inline uint8_t uint8(const bool invert = false) const
        {
            uint8_t result = 0;
            switch (type_.load())
            {
                case Type::UINT8:
                    result = uint8_.load();
                    break;
                case Type::INT16:
                    result = int16_to_uint8(int16_.load());
                    break;
                case Type::UINT16:
                    result = uint16_to_uint8(uint16_.load());
                    break;
                case Type::INT8:
                    result = int8_to_uint8(int8_.load());
                    break;
            }
            return invert ? invert_joy(result) : result;
        }

        inline int8_t int8(bool invert = false) const
        {
            int8_t result = 0;
            switch (type_.load())
            {
                case Type::INT8:
                    result = int8_.load();
                    break;
                case Type::INT16:
                    result = int16_to_int8(int16_.load());
                    break;
                case Type::UINT16:
                    result = uint16_to_int8(uint16_.load());
                    break;
                case Type::UINT8:
                    result = uint8_to_int8(uint8_.load());
                    break;
            }
            return invert ? invert_joy(result) : result;
        }

        inline void set_value(int16_t  value) { type_.store(Type::INT16);  int16_.store(value); }
        inline void set_value(uint16_t value) { type_.store(Type::UINT16); uint16_.store(value); }
        inline void set_value(int8_t   value) { type_.store(Type::INT8);   int8_.store(value); }
        inline void set_value(uint8_t  value) { type_.store(Type::UINT8);  uint8_.store(value); }

        inline void reset_value() { type_ = Type::INT16; uint16_ = 0; };

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

    private:
        enum class Type { INT16, UINT16, INT8, UINT8 };
        std::atomic<Type> type_;
        union {
            std::atomic<int16_t>  int16_;
            std::atomic<uint16_t> uint16_;
            std::atomic<int8_t>   int8_;
            std::atomic<uint8_t>  uint8_;
        };
    }; //Values

    //Get

    inline bool analog_enabled() const { return analog_enabled_; }
    
    inline uint8_t get_dpad_buttons() const { return pad_state_.dpad; }
    inline uint16_t get_buttons() const { return pad_state_.buttons; }

    inline bool get_dpad_up()    const { return (pad_state_.dpad & DPad::UP); }
    inline bool get_dpad_down()  const { return (pad_state_.dpad & DPad::DOWN); }
    inline bool get_dpad_left()  const { return (pad_state_.dpad & DPad::LEFT); }
    inline bool get_dpad_right() const { return (pad_state_.dpad & DPad::RIGHT); }

    inline bool get_button_a() const { return (pad_state_.buttons & Button::A); }
    inline bool get_button_b() const { return (pad_state_.buttons & Button::B); }
    inline bool get_button_x() const { return (pad_state_.buttons & Button::X); }
    inline bool get_button_y() const { return (pad_state_.buttons & Button::Y); }
    inline bool get_button_lb() const { return (pad_state_.buttons & Button::LB); }
    inline bool get_button_rb() const { return (pad_state_.buttons & Button::RB); }
    inline bool get_button_l3() const { return (pad_state_.buttons & Button::L3); }
    inline bool get_button_r3() const { return (pad_state_.buttons & Button::R3); }
    inline bool get_button_start() const { return (pad_state_.buttons & Button::START); }
    inline bool get_button_back()  const { return (pad_state_.buttons & Button::BACK); }
    inline bool get_button_sys()   const { return (pad_state_.buttons & Button::SYS); }
    inline bool get_button_misc()  const { return (pad_state_.buttons & Button::MISC); }

    inline const Values& get_trigger_l() const { return pad_state_.triggers.l; }
    inline const Values& get_trigger_r() const { return pad_state_.triggers.r; }

    inline const Values& get_joystick_lx() const { return pad_state_.joysticks.lx; }
    inline const Values& get_joystick_ly() const { return pad_state_.joysticks.ly; }
    inline const Values& get_joystick_rx() const { return pad_state_.joysticks.rx; }
    inline const Values& get_joystick_ry() const { return pad_state_.joysticks.ry; }

    inline uint8_t get_analog_up() const { return pad_state_.analog_buttons.up; }
    inline uint8_t get_analog_down() const { return pad_state_.analog_buttons.down; }
    inline uint8_t get_analog_left() const { return pad_state_.analog_buttons.left; }
    inline uint8_t get_analog_right() const { return pad_state_.analog_buttons.right; }
    inline uint8_t get_analog_a() const { return pad_state_.analog_buttons.a; }
    inline uint8_t get_analog_b() const { return pad_state_.analog_buttons.b; }
    inline uint8_t get_analog_x() const { return pad_state_.analog_buttons.x; }
    inline uint8_t get_analog_y() const { return pad_state_.analog_buttons.y; }
    inline uint8_t get_analog_lb() const { return pad_state_.analog_buttons.lb; }
    inline uint8_t get_analog_rb() const { return pad_state_.analog_buttons.rb; }

    inline const Values& get_rumble_l() const { return rumble_state_.l; }
    inline const Values& get_rumble_r() const { return rumble_state_.r; }

    inline Chatpad get_chatpad() const 
    { 
        return 
        {  
            pad_state_.chatpad[0], 
            pad_state_.chatpad[1], 
            pad_state_.chatpad[2] 
        }; 
    }

    //Set

    void set_analog_enabled(bool value) { analog_enabled_ = value; }
    void set_profile(const UserProfile& user_profile) 
    { 
        profile_ = user_profile; 
        setup_deadzones();
    }

    inline void set_dpad(uint8_t value) { pad_state_.dpad = value; }

    inline void set_dpad_up()         { pad_state_.dpad |= profile_.dpad_up; }
    inline void set_dpad_down()       { pad_state_.dpad |= profile_.dpad_down; }
    inline void set_dpad_left()       { pad_state_.dpad |= profile_.dpad_left; }
    inline void set_dpad_right()      { pad_state_.dpad |= profile_.dpad_right; }
    inline void set_dpad_up_left()    { pad_state_.dpad |= (profile_.dpad_up | profile_.dpad_left); }
    inline void set_dpad_up_right()   { pad_state_.dpad |= (profile_.dpad_up | profile_.dpad_right); }
    inline void set_dpad_down_left()  { pad_state_.dpad |= (profile_.dpad_down | profile_.dpad_left); }
    inline void set_dpad_down_right() { pad_state_.dpad |= (profile_.dpad_down | profile_.dpad_right); }

    inline void set_buttons(uint16_t value) { pad_state_.buttons = value; }

    inline void set_button_a()     { pad_state_.buttons |= profile_.button_a; }
    inline void set_button_b()     { pad_state_.buttons |= profile_.button_b; }
    inline void set_button_x()     { pad_state_.buttons |= profile_.button_x; }
    inline void set_button_y()     { pad_state_.buttons |= profile_.button_y; }
    inline void set_button_lb()    { pad_state_.buttons |= profile_.button_lb; }
    inline void set_button_rb()    { pad_state_.buttons |= profile_.button_rb; }
    inline void set_button_l3()    { pad_state_.buttons |= profile_.button_l3; }
    inline void set_button_r3()    { pad_state_.buttons |= profile_.button_r3; }
    inline void set_button_start() { pad_state_.buttons |= profile_.button_start; }
    inline void set_button_back()  { pad_state_.buttons |= profile_.button_back;  }
    inline void set_button_sys()   { pad_state_.buttons |= profile_.button_sys;   }
    inline void set_button_misc()  { pad_state_.buttons |= profile_.button_misc;  }

    inline void set_analog_up(uint8_t value)    { *analog_map_[profile_.analog_off_up] = value; }
    inline void set_analog_down(uint8_t value)  { *analog_map_[profile_.analog_off_down] = value; }
    inline void set_analog_left(uint8_t value)  { *analog_map_[profile_.analog_off_left] = value; }
    inline void set_analog_right(uint8_t value) { *analog_map_[profile_.analog_off_right] = value; }
    inline void set_analog_a(uint8_t value)     { *analog_map_[profile_.analog_off_a] = value; }
    inline void set_analog_b(uint8_t value)     { *analog_map_[profile_.analog_off_b] = value; }
    inline void set_analog_x(uint8_t value)     { *analog_map_[profile_.analog_off_x] = value; }
    inline void set_analog_y(uint8_t value)     { *analog_map_[profile_.analog_off_y] = value; }
    inline void set_analog_lb(uint8_t value)    { *analog_map_[profile_.analog_off_lb] = value; }
    inline void set_analog_rb(uint8_t value)    { *analog_map_[profile_.analog_off_rb] = value; }

    inline void set_trigger_l(const uint8_t value)  { pad_state_.triggers.l.set_value((value > dz_trig_uint8_.l) ? value : UINT_8::MIN); }
    inline void set_trigger_l(const uint16_t value) { pad_state_.triggers.l.set_value((value > dz_trig_uint16_.l) ? value : UINT_16::MIN); }

    inline void set_trigger_r(const uint8_t value)  { pad_state_.triggers.r.set_value((value > dz_trig_uint8_.r) ? value : UINT_8::MIN); }
    inline void set_trigger_r(const uint16_t value) { pad_state_.triggers.r.set_value((value > dz_trig_uint16_.r) ? value : UINT_16::MIN); }

    inline void set_joystick_lx(const int16_t value, const bool invert = false) { pad_state_.joysticks.lx.set_value((value < dz_joy_int16_.l_neg || value > dz_joy_int16_.l_pos) ? (invert ? Values::invert_joy(value) : value) : INT_16::MID); }
    inline void set_joystick_lx(const uint8_t value, const bool invert = false) { pad_state_.joysticks.lx.set_value((value < dz_joy_uint8_.l_neg || value > dz_joy_uint8_.l_pos) ? (invert ? Values::invert_joy(value) : value) : UINT_8::MID); }

    inline void set_joystick_ly(const int16_t value, const bool invert = false) { pad_state_.joysticks.ly.set_value((value < dz_joy_int16_.l_neg || value > dz_joy_int16_.l_pos) ? ((invert ^ profile_.invert_ly) ? Values::invert_joy(value) : value) : INT_16::MID); }
    inline void set_joystick_ly(const uint8_t value, const bool invert = false) { pad_state_.joysticks.ly.set_value((value < dz_joy_uint8_.l_neg || value > dz_joy_uint8_.l_pos) ? ((invert ^ profile_.invert_ly) ? Values::invert_joy(value) : value) : UINT_8::MID); }

    inline void set_joystick_rx(const int16_t value, const bool invert = false) { pad_state_.joysticks.rx.set_value((value < dz_joy_int16_.r_neg || value > dz_joy_int16_.r_pos) ? (invert ? Values::invert_joy(value) : value) : INT_16::MID); }
    inline void set_joystick_rx(const uint8_t value, const bool invert = false) { pad_state_.joysticks.rx.set_value((value < dz_joy_uint8_.r_neg || value > dz_joy_uint8_.r_pos) ? (invert ? Values::invert_joy(value) : value) : UINT_8::MID); }

    inline void set_joystick_ry(const int16_t value, const bool invert = false) { pad_state_.joysticks.ry.set_value((value < dz_joy_int16_.r_neg || value > dz_joy_int16_.r_pos) ? ((invert ^ profile_.invert_ry) ? Values::invert_joy(value) : value) : INT_16::MID); }
    inline void set_joystick_ry(const uint8_t value, const bool invert = false) { pad_state_.joysticks.ry.set_value((value < dz_joy_uint8_.r_neg || value > dz_joy_uint8_.r_pos) ? ((invert ^ profile_.invert_ry) ? Values::invert_joy(value) : value) : UINT_8::MID); }

    // 10 bit methods for weird stuff
    inline void set_trigger_l_uint10(const int32_t value)  
    { 
        uint8_t new_value = Values::uint10_to_uint8(value);
        pad_state_.triggers.l.set_value((new_value > dz_trig_uint8_.l) ? new_value : UINT_8::MIN); 
    }
    inline void set_trigger_r_uint10(const int32_t value)  
    { 
        uint8_t new_value = Values::uint10_to_uint8(value);
        pad_state_.triggers.r.set_value((new_value > dz_trig_uint8_.r) ? new_value : UINT_8::MIN); 
    }
    inline void set_joystick_lx_int10(const int32_t value, const bool invert = false) 
    { 
        uint8_t new_value = Values::int10_to_uint8(value);
        pad_state_.joysticks.lx.set_value((new_value < dz_joy_uint8_.l_neg || new_value > dz_joy_uint8_.l_pos) ? (invert ? Values::invert_joy(new_value) : new_value) : UINT_8::MID);
    }
    inline void set_joystick_ly_int10(const int32_t value, const bool invert = false) 
    { 
        uint8_t new_value = Values::int10_to_uint8(value);
        pad_state_.joysticks.ly.set_value((new_value < dz_joy_uint8_.l_neg || new_value > dz_joy_uint8_.l_pos) ? ((invert ^ profile_.invert_ly) ? Values::invert_joy(new_value) : new_value) : UINT_8::MID); 
    }
    inline void set_joystick_rx_int10(const int32_t value, const bool invert = false) 
    { 
        uint8_t new_value = Values::int10_to_uint8(value);
        pad_state_.joysticks.rx.set_value((new_value < dz_joy_uint8_.r_neg || new_value > dz_joy_uint8_.r_pos) ? (invert ? Values::invert_joy(new_value) : new_value) : UINT_8::MID);
    }
    inline void set_joystick_ry_int10(const int32_t value, const bool invert = false) 
    { 
        uint8_t new_value = Values::int10_to_uint8(value);
        pad_state_.joysticks.ry.set_value((new_value < dz_joy_uint8_.r_neg || new_value > dz_joy_uint8_.r_pos) ? ((invert ^ profile_.invert_ry) ? Values::invert_joy(new_value) : new_value) : UINT_8::MID); 
    }
    // inline void set_joystick_lx_int10(const int32_t value, const bool invert = false) 
    // { 
    //     int16_t new_value = Values::int10_to_int16(value);
    //     pad_state_.joysticks.lx.set_value((new_value < dz_joy_int16_.l_neg || new_value > dz_joy_int16_.l_pos) ? (invert ? Values::invert_joy(new_value) : new_value) : INT_16::MID);
    // }
    // inline void set_joystick_ly_int10(const int32_t value, const bool invert = false) 
    // { 
    //     int16_t new_value = Values::int10_to_int16(value);
    //     pad_state_.joysticks.ly.set_value((new_value < dz_joy_int16_.l_neg || new_value > dz_joy_int16_.l_pos) ? ((invert ^ profile_.invert_ly) ? Values::invert_joy(new_value) : new_value) : INT_16::MID); 
    // }
    // inline void set_joystick_rx_int10(const int32_t value, const bool invert = false) 
    // { 
    //     int16_t new_value = Values::int10_to_int16(value);
    //     pad_state_.joysticks.rx.set_value((new_value < dz_joy_int16_.r_neg || new_value > dz_joy_int16_.r_pos) ? (invert ? Values::invert_joy(new_value) : new_value) : INT_16::MID);
    // }
    // inline void set_joystick_ry_int10(const int32_t value, const bool invert = false) 
    // { 
    //     int16_t new_value = Values::int10_to_int16(value);
    //     pad_state_.joysticks.ry.set_value((new_value < dz_joy_int16_.r_neg || new_value > dz_joy_int16_.r_pos) ? ((invert ^ profile_.invert_ry) ? Values::invert_joy(new_value) : new_value) : INT_16::MID); 
    // }

    inline void set_rumble_l(uint8_t value)  { rumble_state_.l.set_value(value); }
    inline void set_rumble_l(uint16_t value) { rumble_state_.l.set_value(value); }
    inline void set_rumble_r(uint8_t value)  { rumble_state_.r.set_value(value); }
    inline void set_rumble_r(uint16_t value) { rumble_state_.r.set_value(value); }

    inline void set_chatpad(const Chatpad& chatpad_array) 
    {
        pad_state_.chatpad[0] = chatpad_array[0];
        pad_state_.chatpad[1] = chatpad_array[1];
        pad_state_.chatpad[2] = chatpad_array[2];
    }

    inline void reset_buttons() { pad_state_.dpad = 0; pad_state_.buttons = 0; }
    inline void reset_pad() 
	{ 
        pad_state_.dpad = 0;
        pad_state_.buttons = 0;
        pad_state_.triggers.l.set_value(UINT_8::MIN);
        pad_state_.triggers.r.set_value(UINT_8::MIN);
        pad_state_.joysticks.lx.set_value(UINT_8::MID);
        pad_state_.joysticks.ly.set_value(UINT_8::MID);
        pad_state_.joysticks.rx.set_value(UINT_8::MID);
        pad_state_.joysticks.ry.set_value(UINT_8::MID);

        std::memset(&pad_state_.analog_buttons, 0, sizeof(PadState::AnalogButtons));
        std::memset(&pad_state_.chatpad, 0, sizeof(Chatpad));
    }
    inline void reset_rumble()
    {
        rumble_state_.l.set_value(UINT_8::MIN);
        rumble_state_.l.set_value(UINT_8::MIN);
    }

private:
    struct PadState
    {
        uint8_t dpad{0};
        uint16_t buttons{0};

        struct Triggers
        {
            Values l;
            Values r;
        } triggers;

        struct Joysticks
        {
            Values lx;
            Values ly;
            Values rx;
            Values ry;
        } joysticks;

        struct AnalogButtons
        {
            uint8_t up{0};
            uint8_t down{0};
            uint8_t left{0};
            uint8_t right{0};
            uint8_t a{0};
            uint8_t b{0};
            uint8_t x{0};
            uint8_t y{0};
            uint8_t lb{0};
            uint8_t rb{0};
        } analog_buttons;

        std::array<uint8_t, 3> chatpad = { 0, 0, 0 };
    };

    struct Rumble
    {
        Values l;
        Values r;
    };

    bool analog_enabled_{false};
    UserProfile profile_;

    template<typename type>
    struct DZTrigger
    {
        type l{0};
        type r{0};
    };

    template<typename type>
    struct DZJoystick
    {
        type l_neg;
        type l_pos;
        type r_neg;
        type r_pos;

        DZJoystick()
        {
            if constexpr (std::is_same_v<type, uint8_t>) { l_neg = l_pos = r_neg = r_pos = UINT_8::MID; }
            else if constexpr (std::is_same_v<type, int16_t>) { l_neg = l_pos = r_neg = r_pos = 0; }
        }
    };

    DZTrigger<uint8_t>  dz_trig_uint8_;
    DZTrigger<uint16_t> dz_trig_uint16_;
    DZJoystick<int16_t> dz_joy_int16_;
    DZJoystick<uint8_t> dz_joy_uint8_;

    PadState pad_state_;
    Rumble rumble_state_;

    std::array<uint8_t*, sizeof(PadState::AnalogButtons)> analog_map_ =
    {
        &pad_state_.analog_buttons.up,
        &pad_state_.analog_buttons.down,
        &pad_state_.analog_buttons.left,
        &pad_state_.analog_buttons.right,
        &pad_state_.analog_buttons.a,
        &pad_state_.analog_buttons.b,
        &pad_state_.analog_buttons.x,
        &pad_state_.analog_buttons.y,
        &pad_state_.analog_buttons.lb,
        &pad_state_.analog_buttons.rb
    };

    void setup_deadzones() //Deadzones in the profile are 0-255 (0-100%)
    {
        dz_trig_uint8_.l = profile_.dz_trigger_l;
        dz_trig_uint8_.r = profile_.dz_trigger_r;
        dz_trig_uint16_.l = Values::uint8_to_uint16(profile_.dz_trigger_l);
        dz_trig_uint16_.r = Values::uint8_to_uint16(profile_.dz_trigger_r);

        dz_joy_uint8_.l_neg = UINT_8::MID - (profile_.dz_joystick_l / 2);
        dz_joy_uint8_.l_pos = UINT_8::MID + (profile_.dz_joystick_l / 2);
        dz_joy_uint8_.r_neg = UINT_8::MID - (profile_.dz_joystick_r / 2);
        dz_joy_uint8_.r_pos = UINT_8::MID + (profile_.dz_joystick_r / 2);

        dz_joy_int16_.l_neg = Values::uint8_to_int16(dz_joy_uint8_.l_neg);
        dz_joy_int16_.l_pos = Values::uint8_to_int16(dz_joy_uint8_.l_pos);
        dz_joy_int16_.r_neg = Values::uint8_to_int16(dz_joy_uint8_.r_neg);
        dz_joy_int16_.r_pos = Values::uint8_to_int16(dz_joy_uint8_.r_pos);
    }
};

#endif // _GAMEPAD_H_