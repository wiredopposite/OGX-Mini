#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#include <cstdint>
#include <atomic>
#include <limits>
#include <cstring>
#include <array>
#include <pico/mutex.h>

#include "Scale.h"
#include "UserSettings/UserProfile.h"

class Gamepad 
{
public:
    //Defaults used by device to get buttons

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

    static constexpr uint8_t ANALOG_OFF_UP    = 0;
    static constexpr uint8_t ANALOG_OFF_DOWN  = 1;
    static constexpr uint8_t ANALOG_OFF_LEFT  = 2;
    static constexpr uint8_t ANALOG_OFF_RIGHT = 3;
    static constexpr uint8_t ANALOG_OFF_A     = 4;
    static constexpr uint8_t ANALOG_OFF_B     = 5;
    static constexpr uint8_t ANALOG_OFF_X     = 6;
    static constexpr uint8_t ANALOG_OFF_Y     = 7;
    static constexpr uint8_t ANALOG_OFF_LB    = 8;
    static constexpr uint8_t ANALOG_OFF_RB    = 9;

    //Mappings used by host to set buttons

    uint8_t MAP_DPAD_UP         = DPAD_UP        ;
    uint8_t MAP_DPAD_DOWN       = DPAD_DOWN      ;
    uint8_t MAP_DPAD_LEFT       = DPAD_LEFT      ;
    uint8_t MAP_DPAD_RIGHT      = DPAD_RIGHT     ;
    uint8_t MAP_DPAD_UP_LEFT    = DPAD_UP_LEFT   ;
    uint8_t MAP_DPAD_UP_RIGHT   = DPAD_UP_RIGHT  ;
    uint8_t MAP_DPAD_DOWN_LEFT  = DPAD_DOWN_LEFT ;
    uint8_t MAP_DPAD_DOWN_RIGHT = DPAD_DOWN_RIGHT;
    uint8_t MAP_DPAD_NONE       = DPAD_NONE      ;

    uint16_t MAP_BUTTON_A     = BUTTON_A    ;
    uint16_t MAP_BUTTON_B     = BUTTON_B    ;
    uint16_t MAP_BUTTON_X     = BUTTON_X    ;
    uint16_t MAP_BUTTON_Y     = BUTTON_Y    ;
    uint16_t MAP_BUTTON_L3    = BUTTON_L3   ;
    uint16_t MAP_BUTTON_R3    = BUTTON_R3   ;
    uint16_t MAP_BUTTON_BACK  = BUTTON_BACK ;
    uint16_t MAP_BUTTON_START = BUTTON_START;
    uint16_t MAP_BUTTON_LB    = BUTTON_LB   ;
    uint16_t MAP_BUTTON_RB    = BUTTON_RB   ;
    uint16_t MAP_BUTTON_SYS   = BUTTON_SYS  ;
    uint16_t MAP_BUTTON_MISC  = BUTTON_MISC ;

    uint8_t MAP_ANALOG_OFF_UP    = ANALOG_OFF_UP   ;
    uint8_t MAP_ANALOG_OFF_DOWN  = ANALOG_OFF_DOWN ;
    uint8_t MAP_ANALOG_OFF_LEFT  = ANALOG_OFF_LEFT ;
    uint8_t MAP_ANALOG_OFF_RIGHT = ANALOG_OFF_RIGHT;
    uint8_t MAP_ANALOG_OFF_A     = ANALOG_OFF_A    ;
    uint8_t MAP_ANALOG_OFF_B     = ANALOG_OFF_B    ;
    uint8_t MAP_ANALOG_OFF_X     = ANALOG_OFF_X    ;
    uint8_t MAP_ANALOG_OFF_Y     = ANALOG_OFF_Y    ;
    uint8_t MAP_ANALOG_OFF_LB    = ANALOG_OFF_LB   ;
    uint8_t MAP_ANALOG_OFF_RB    = ANALOG_OFF_RB   ;

#pragma pack(push, 1)
    struct PadIn
    {
        uint8_t dpad;
        uint16_t buttons;
        uint8_t trigger_l;
        uint8_t trigger_r;
        int16_t joystick_lx;
        int16_t joystick_ly;
        int16_t joystick_rx;
        int16_t joystick_ry;
        // uint8_t chatpad[3];
        uint8_t analog[10];
    
        PadIn()
        {
            std::memset(this, 0, sizeof(PadIn));
        }
    };

    struct PadOut
    {
        uint8_t rumble_l;
        uint8_t rumble_r;

        PadOut()
        {
            std::memset(this, 0, sizeof(PadOut));
        }
    };

    using ChatpadIn = std::array<uint8_t, 3>;

#pragma pack(pop)

    Gamepad()
    {
        mutex_init(&pad_in_mutex_);
        mutex_init(&pad_out_mutex_);
        mutex_init(&chatpad_in_mutex_);
        reset_pad_in();
        reset_pad_out();
        reset_chatpad_in();
    };

    ~Gamepad() = default;

    //Get
    inline bool new_pad_in() const { return new_pad_in_.load(); }
    inline bool new_pad_out() const { return new_pad_out_.load(); }

    //True if both host and device have enabled analog
    inline bool analog_enabled() const { return analog_enabled_.load(std::memory_order_relaxed); }

    inline PadIn get_pad_in()
    {
        mutex_enter_blocking(&pad_in_mutex_);
        PadIn pad_in = pad_in_;
        new_pad_in_.store(false);
        mutex_exit(&pad_in_mutex_);

        return pad_in;
    }

    inline PadOut get_pad_out()
    {
        mutex_enter_blocking(&pad_out_mutex_);
        PadOut pad_out = pad_out_;
        new_pad_out_.store(false);
        mutex_exit(&pad_out_mutex_);

        return pad_out;
    }

    inline ChatpadIn get_chatpad_in()
    {
        mutex_enter_blocking(&chatpad_in_mutex_);
        ChatpadIn chatpad_in = chatpad_in_;
        mutex_exit(&chatpad_in_mutex_);

        return chatpad_in;
    }

    //Set

    void set_analog_device(bool value) 
    { 
        analog_device_.store(value); 
        if (analog_host_.load() && analog_device_.load() && profile_analog_enabled_)
        {
            analog_enabled_.store(true);
        }
    }

    void set_analog_host(bool value) 
    { 
        analog_host_.store(value); 
        if (analog_host_.load() && analog_device_.load() && profile_analog_enabled_)
        {
            analog_enabled_.store(true);
        }
    }

    void set_profile(const UserProfile& user_profile) 
    { 
        set_profile_options(user_profile);
        set_profile_mappings(user_profile);
        set_profile_deadzones(user_profile);
    }

    inline void set_pad_in(PadIn pad_in)
    {
        pad_in.trigger_l = (pad_in.trigger_l > dz_.trigger_l) ? pad_in.trigger_l : UINT_8::MIN;
        pad_in.trigger_r = (pad_in.trigger_r > dz_.trigger_r) ? pad_in.trigger_r : UINT_8::MIN;
        pad_in.joystick_lx = (pad_in.joystick_lx < dz_.joystick_l_neg || pad_in.joystick_lx > dz_.joystick_l_pos) ? pad_in.joystick_lx : INT_16::MID;
        pad_in.joystick_ly = (pad_in.joystick_ly < dz_.joystick_l_neg || pad_in.joystick_ly > dz_.joystick_l_pos) ? pad_in.joystick_ly : INT_16::MID;
        pad_in.joystick_rx = (pad_in.joystick_rx < dz_.joystick_r_neg || pad_in.joystick_rx > dz_.joystick_r_pos) ? pad_in.joystick_rx : INT_16::MID;
        pad_in.joystick_ry = (pad_in.joystick_ry < dz_.joystick_r_neg || pad_in.joystick_ry > dz_.joystick_r_pos) ? pad_in.joystick_ry : INT_16::MID;
        pad_in.joystick_ly = profile_invert_ly_ ? Scale::invert_joy(pad_in.joystick_ly) : pad_in.joystick_ly;
        pad_in.joystick_ry = profile_invert_ry_ ? Scale::invert_joy(pad_in.joystick_ry) : pad_in.joystick_ry;

        mutex_enter_blocking(&pad_in_mutex_);
        pad_in_ = pad_in;
        new_pad_in_.store(true);
        mutex_exit(&pad_in_mutex_);
    }

    inline void set_pad_out(const PadOut& pad_out)
    {
        mutex_enter_blocking(&pad_out_mutex_);
        pad_out_ = pad_out;
        new_pad_out_.store(true);
        mutex_exit(&pad_out_mutex_);
    }

    inline void set_chatpad_in(const ChatpadIn& chatpad_in)
    {
        mutex_enter_blocking(&chatpad_in_mutex_);
        chatpad_in_ = chatpad_in;
        mutex_exit(&chatpad_in_mutex_);
    }

    inline void reset_pad_in() 
	{ 
        mutex_enter_blocking(&pad_in_mutex_);
        pad_in_ = PadIn();
        mutex_exit(&pad_in_mutex_);
        new_pad_in_.store(true);
    }
    
    inline void reset_pad_out()
    {
        mutex_enter_blocking(&pad_out_mutex_);
        pad_out_ = PadOut();
        new_pad_out_.store(true);
        mutex_exit(&pad_out_mutex_);
    }

    inline void reset_chatpad_in()
    {
        mutex_enter_blocking(&chatpad_in_mutex_);
        chatpad_in_.fill(0);
        mutex_exit(&chatpad_in_mutex_);
    }

private:
    mutex_t pad_in_mutex_;
    mutex_t pad_out_mutex_;
    mutex_t chatpad_in_mutex_;

    PadOut pad_out_;
    PadIn pad_in_;
    ChatpadIn chatpad_in_;

    std::atomic<bool> new_pad_in_{false};
    std::atomic<bool> new_pad_out_{false};

    std::atomic<bool> analog_enabled_{false};
    std::atomic<bool> analog_host_{false};
    std::atomic<bool> analog_device_{false};

    bool profile_invert_ly_{false};
    bool profile_invert_ry_{false};
    bool profile_analog_enabled_{false};

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
        profile_analog_enabled_ = profile.analog_enabled ? true : false;
    }

    void set_profile_mappings(const UserProfile& profile)
    {
        MAP_DPAD_UP         = profile.dpad_up;
        MAP_DPAD_DOWN       = profile.dpad_down;
        MAP_DPAD_LEFT       = profile.dpad_left;
        MAP_DPAD_RIGHT      = profile.dpad_right;
        MAP_DPAD_UP_LEFT    = profile.dpad_up | profile.dpad_left;
        MAP_DPAD_UP_RIGHT   = profile.dpad_up | profile.dpad_right;
        MAP_DPAD_DOWN_LEFT  = profile.dpad_down | profile.dpad_left;
        MAP_DPAD_DOWN_RIGHT = profile.dpad_down | profile.dpad_right;
        MAP_DPAD_NONE       = 0;

        MAP_BUTTON_A     = profile.button_a;
        MAP_BUTTON_B     = profile.button_b;
        MAP_BUTTON_X     = profile.button_x;
        MAP_BUTTON_Y     = profile.button_y;
        MAP_BUTTON_L3    = profile.button_l3;
        MAP_BUTTON_R3    = profile.button_r3;
        MAP_BUTTON_BACK  = profile.button_back;
        MAP_BUTTON_START = profile.button_start;
        MAP_BUTTON_LB    = profile.button_lb;
        MAP_BUTTON_RB    = profile.button_rb;
        MAP_BUTTON_SYS   = profile.button_sys;
        MAP_BUTTON_MISC  = profile.button_misc;

        MAP_ANALOG_OFF_UP    = profile.analog_off_up;
        MAP_ANALOG_OFF_DOWN  = profile.analog_off_down;
        MAP_ANALOG_OFF_LEFT  = profile.analog_off_left;
        MAP_ANALOG_OFF_RIGHT = profile.analog_off_right;
        MAP_ANALOG_OFF_A     = profile.analog_off_a;
        MAP_ANALOG_OFF_B     = profile.analog_off_b;
        MAP_ANALOG_OFF_X     = profile.analog_off_x;
        MAP_ANALOG_OFF_Y     = profile.analog_off_y;
        MAP_ANALOG_OFF_LB    = profile.analog_off_lb;
        MAP_ANALOG_OFF_RB    = profile.analog_off_rb;
    }

    void set_profile_deadzones(const UserProfile& profile) //Deadzones in the profile are 0-255 (0-100%)
    {
        dz_.trigger_l = profile.dz_trigger_l;
        dz_.trigger_r = profile.dz_trigger_r;

        dz_.joystick_l_pos = Scale::uint8_to_int16(profile.dz_joystick_l / 2);
        dz_.joystick_l_neg = Scale::invert_joy(dz_.joystick_l_pos);
        dz_.joystick_r_pos = Scale::uint8_to_int16(profile.dz_joystick_r / 2);
        dz_.joystick_r_neg = Scale::invert_joy(dz_.joystick_r_pos);
    }
};

#endif // _GAMEPAD_H_