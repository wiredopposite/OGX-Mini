#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <cstdint>

#include "sdkconfig.h"
#include "Gamepad/Range.h"
#include "Gamepad/fix16ext.h"
#include "UserSettings/UserProfile.h"
#include "UserSettings/UserSettings.h"
#include "Board/ogxm_log.h"

#define MAX_GAMEPADS CONFIG_BLUEPAD32_MAX_DEVICES
static_assert(  MAX_GAMEPADS > 0 && 
                MAX_GAMEPADS <= 4, 
                "MAX_GAMEPADS must be between 1 and 4");

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
        set_profile_settings(profile);
        set_profile_mappings(profile);
    }

    template <uint8_t bits = 0, typename T>
    inline std::pair<int16_t, int16_t> scale_joystick_r(T x, T y, bool invert_y = false) const
    {
        int16_t joy_x = 0;
        int16_t joy_y = 0;
        if constexpr (bits > 0)
        {
            joy_x = Range::scale_from_bits<int16_t, bits>(x);
            joy_y = Range::scale_from_bits<int16_t, bits>(y);
        }
        else if constexpr (!std::is_same_v<T, int16_t>)
        {
            joy_x = Range::scale<int16_t>(x);
            joy_y = Range::scale<int16_t>(y);
        }
        else
        {
            joy_x = x;
            joy_y = y;
        }

        return  joy_settings_r_en_ 
                    ? apply_joystick_settings(joy_x, joy_y, joy_settings_r_, invert_y) 
                    : std::make_pair(joy_x, (invert_y ? Range::invert(joy_y) : joy_y));
    }

    template <uint8_t bits = 0, typename T>
    inline std::pair<int16_t, int16_t> scale_joystick_l(T x, T y, bool invert_y = false) const
    {
        int16_t joy_x = 0;
        int16_t joy_y = 0;
        if constexpr (bits > 0)
        {
            joy_x = Range::scale_from_bits<int16_t, bits>(x);
            joy_y = Range::scale_from_bits<int16_t, bits>(y);
        }
        else if constexpr (!std::is_same_v<T, int16_t>)
        {
            joy_x = Range::scale<int16_t>(x);
            joy_y = Range::scale<int16_t>(y);
        }
        else
        {
            joy_x = x;
            joy_y = y;
        }

        return  joy_settings_l_en_ 
                    ? apply_joystick_settings(joy_x, joy_y, joy_settings_l_, invert_y) 
                    : std::make_pair(joy_x, (invert_y ? Range::invert(joy_y) : joy_y));
    }

    template <uint8_t bits = 0, typename T>
    inline uint8_t scale_trigger_l(T value) const
    {
        uint8_t trigger_value = 0;
        if constexpr (bits > 0)
        {
            trigger_value = Range::scale_from_bits<uint8_t, bits>(value);
        }
        else if constexpr (!std::is_same_v<T, uint8_t>)
        {
            trigger_value = Range::scale<uint8_t>(value);
        }
        else
        {
            trigger_value = value;
        }
        return  trig_settings_l_en_ 
                    ? apply_trigger_settings(trigger_value, trig_settings_l_) 
                    : trigger_value;
    }

    template <uint8_t bits = 0, typename T>
    inline uint8_t scale_trigger_r(T value) const
    {
        uint8_t trigger_value = 0;
        if constexpr (bits > 0)
        {
            trigger_value = Range::scale_from_bits<uint8_t, bits>(value);
        }
        else if constexpr (!std::is_same_v<T, uint8_t>)
        {
            trigger_value = Range::scale<uint8_t>(value);
        }
        else
        {
            trigger_value = value;
        }
        return  trig_settings_r_en_ 
                    ? apply_trigger_settings(trigger_value, trig_settings_r_) 
                    : trigger_value;
    }

private:
    JoystickSettings joy_settings_l_;
    JoystickSettings joy_settings_r_;
    TriggerSettings trig_settings_l_;
    TriggerSettings trig_settings_r_;

    bool joy_settings_l_en_{false};
    bool joy_settings_r_en_{false};
    bool trig_settings_l_en_{false};
    bool trig_settings_r_en_{false};

    void set_profile_settings(const UserProfile& profile)
    {
        if ((joy_settings_l_en_ = !joy_settings_l_.is_same(profile.joystick_settings_l)))
        {
            joy_settings_l_.set_from_raw(profile.joystick_settings_l);
            //This needs to be addressed in the webapp, just multiply here for now
            joy_settings_l_.axis_restrict *= static_cast<int16_t>(100);
            joy_settings_l_.angle_restrict *= static_cast<int16_t>(100);
            joy_settings_l_.anti_dz_angular *= static_cast<int16_t>(100);
        }
        if ((joy_settings_r_en_ = !joy_settings_r_.is_same(profile.joystick_settings_r)))
        {
            joy_settings_r_.set_from_raw(profile.joystick_settings_r);
            //This needs to be addressed in the webapp, just multiply here for now
            joy_settings_r_.axis_restrict *= static_cast<int16_t>(100);
            joy_settings_r_.angle_restrict *= static_cast<int16_t>(100);
            joy_settings_r_.anti_dz_angular *= static_cast<int16_t>(100);
        }
        if ((trig_settings_l_en_ = !trig_settings_l_.is_same(profile.trigger_settings_l)))
        {
            trig_settings_l_.set_from_raw(profile.trigger_settings_l);
        }
        if ((trig_settings_r_en_ = !trig_settings_r_.is_same(profile.trigger_settings_r)))
        {
            trig_settings_r_.set_from_raw(profile.trigger_settings_r);
        }

        OGXM_LOG("GamepadMapper: JoyL: %s, JoyR: %s, TrigL: %s, TrigR: %s\n",
            joy_settings_l_en_ ? "Enabled" : "Disabled",
            joy_settings_r_en_ ? "Enabled" : "Disabled",
            trig_settings_l_en_ ? "Enabled" : "Disabled",
            trig_settings_r_en_ ? "Enabled" : "Disabled");
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
    }

    static inline std::pair<int16_t, int16_t> apply_joystick_settings(
        int16_t gp_joy_x, 
        int16_t gp_joy_y, 
        const JoystickSettings& set,
        bool invert_y)
    {
        static const Fix16 
            FIX_0(0.0f),
            FIX_1(1.0f),
            FIX_2(2.0f),
            FIX_45(45.0f),
            FIX_90(90.0f),
            FIX_100(100.0f),
            FIX_180(180.0f),
            FIX_EPSILON(0.0001f),
            FIX_EPSILON2(0.001f),
            FIX_ELLIPSE_DEF(1.570796f),
            FIX_DIAG_DIVISOR(0.29289f);

        Fix16 x = (set.invert_x ? Fix16(Range::invert(gp_joy_x)) : Fix16(gp_joy_x)) / Range::MAX<int16_t>;
        Fix16 y = ((set.invert_y ^ invert_y) ? Fix16(Range::invert(gp_joy_y)) : Fix16(gp_joy_y)) / Range::MAX<int16_t>;

        const Fix16 abs_x = fix16::abs(x);
        const Fix16 abs_y = fix16::abs(y);
        const Fix16 inv_axis_restrict = FIX_1 / (FIX_1 - set.axis_restrict);

        Fix16 rAngle = (abs_x < FIX_EPSILON) 
            ? FIX_90 
            : fix16::rad2deg(fix16::abs(fix16::atan(y / x)));

        Fix16 axial_x = (abs_x <= set.axis_restrict && rAngle > FIX_45) 
            ? FIX_0 
            : ((abs_x - set.axis_restrict) * inv_axis_restrict);
                
        Fix16 axial_y = (abs_y <= set.axis_restrict && rAngle <= FIX_45) 
            ? FIX_0 
            : ((abs_y - set.axis_restrict) * inv_axis_restrict);

        Fix16 in_magnitude = fix16::sqrt(fix16::sq(axial_x) + fix16::sq(axial_y));

        if (in_magnitude < set.dz_inner)
        {
            return { 0, 0 };
        }

        Fix16 angle = 
            fix16::abs(axial_x) < FIX_EPSILON 
                ? FIX_90 
                : fix16::rad2deg(fix16::abs(fix16::atan(axial_y / axial_x)));

        Fix16 anti_r_scale = (set.anti_dz_square_y_scale == FIX_0) ? set.anti_dz_square : set.anti_dz_square_y_scale;
        Fix16 anti_dz_c = set.anti_dz_circle;

        if (anti_r_scale > FIX_0 && anti_dz_c > FIX_0)
        {
            Fix16 anti_ellip_scale = anti_ellip_scale / anti_dz_c;
            Fix16 ellipse_angle = fix16::atan((FIX_1 / anti_ellip_scale) * fix16::tan(fix16::rad2deg(rAngle)));
            ellipse_angle = (ellipse_angle < FIX_0) ? FIX_ELLIPSE_DEF : ellipse_angle;

            Fix16 ellipse_x = fix16::cos(ellipse_angle);
            Fix16 ellipse_y = fix16::sqrt(fix16::sq(anti_ellip_scale) * (FIX_1 - fix16::sq(ellipse_x)));
            anti_dz_c *= fix16::sqrt(fix16::sq(ellipse_x) + fix16::sq(ellipse_y));
        }

        if (anti_dz_c > FIX_0)
        {
            anti_dz_c = anti_dz_c / ((anti_dz_c * (FIX_1 - set.anti_dz_circle / set.dz_outer)) / (anti_dz_c * (FIX_1 - set.anti_dz_square)));
        }

        if (abs_x > set.axis_restrict && abs_y > set.axis_restrict)
        {
            const Fix16 FIX_ANGLE_MAX = set.angle_restrict / 2.0f;

            if (angle > FIX_0 && angle < FIX_ANGLE_MAX)
            {
                angle = FIX_0;
            }
            if (angle > (FIX_90 - FIX_ANGLE_MAX))
            {
                angle = FIX_90;
            }
            if (angle > FIX_ANGLE_MAX && angle < (FIX_90 - FIX_ANGLE_MAX))
            {
                angle = ((angle - FIX_ANGLE_MAX) * FIX_90) / ((FIX_90 - FIX_ANGLE_MAX) - FIX_ANGLE_MAX);
            }
        }

        Fix16 ref_angle = (angle < FIX_EPSILON2) ? FIX_0 : angle;
        Fix16 diagonal = (angle > FIX_45) ? (((angle - FIX_45) * (-FIX_45)) / FIX_45) + FIX_45 : angle;

        const Fix16 angle_comp = set.angle_restrict / FIX_2;

        if (angle < FIX_90 && angle > FIX_0)
        {
            angle = ((angle * ((FIX_90 - angle_comp) - angle_comp)) / FIX_90) + angle_comp;
        }

        if (axial_x < FIX_0 && axial_y > FIX_0)
        {
            angle = -angle;
        }
        if (axial_x > FIX_0 && axial_y < FIX_0)
        {
            angle = angle - FIX_180;
        }
        if (axial_x < FIX_0 && axial_y < FIX_0)
        {
            angle = angle + FIX_180;
        }

        //Deadzone Warp
        Fix16 out_magnitude = (in_magnitude - set.dz_inner) / (set.anti_dz_outer - set.dz_inner);
        out_magnitude = fix16::pow(out_magnitude, (FIX_1 / set.curve)) * (set.dz_outer - anti_dz_c) + anti_dz_c;
        out_magnitude = (out_magnitude > set.dz_outer && !set.uncap_radius) ? set.dz_outer : out_magnitude;

		Fix16 d_scale = (((out_magnitude - anti_dz_c) * (set.diag_scale_max - set.diag_scale_min)) / (set.dz_outer - anti_dz_c)) + set.diag_scale_min;		
		Fix16 c_scale = (diagonal * (FIX_1 / fix16::sqrt(FIX_2))) / FIX_45; //Both these lines scale the intensity of the warping
		c_scale       = FIX_1 - fix16::sqrt(FIX_1 - c_scale * c_scale);     //based on a circular curve to the perfect diagonal
		d_scale       = (c_scale * (d_scale - FIX_1)) / FIX_DIAG_DIVISOR + FIX_1;

		out_magnitude = out_magnitude * d_scale;

		//Scaling values for square antideadzone
		Fix16 new_x = fix16::cos(fix16::deg2rad(angle)) * out_magnitude;
		Fix16 new_y = fix16::sin(fix16::deg2rad(angle)) * out_magnitude;
		
		//Magic angle wobble fix by user ME.
		// if (angle > 45.0 && angle < 225.0) {
		// 	newX = inv(Math.sin(deg2rad(angle - 90.0)))*outputMagnitude;
		// 	newY = inv(Math.cos(deg2rad(angle - 270.0)))*outputMagnitude;
		// }
		
		//Square antideadzone scaling
		Fix16 output_x = fix16::abs(new_x) * (FIX_1 - set.anti_dz_square / set.dz_outer) + set.anti_dz_square;
		if (x < FIX_0)
        {
            output_x = -output_x;
        }
		if (ref_angle == FIX_90)
        {
            output_x = FIX_0;
        }
		
		Fix16 output_y = fix16::abs(new_y) * (FIX_1 - anti_r_scale / set.dz_outer) + anti_r_scale;
		if (y < FIX_0)
        {
            output_y = -output_y;
        }
		if (ref_angle == FIX_0)
        {
            output_y = FIX_0;
        }

        output_x = fix16::clamp(output_x, -FIX_1, FIX_1) * Range::MAX<int16_t>;
        output_y = fix16::clamp(output_y, -FIX_1, FIX_1) * Range::MAX<int16_t>;

        return { static_cast<int16_t>(fix16_to_int(output_x)), static_cast<int16_t>(fix16_to_int(output_y)) };
    }

    static inline uint8_t apply_trigger_settings(uint8_t value, const TriggerSettings& set)
    {
        Fix16 abs_value = fix16::abs(Fix16(static_cast<int16_t>(value)) / static_cast<int16_t>(Range::MAX<uint8_t>));

        if (abs_value < set.dz_inner)
        {
            return 0;
        }

        static const Fix16 
            FIX_0(0.0f),
            FIX_1(1.0f),
            FIX_2(2.0f);

        Fix16 value_out = (abs_value - set.dz_inner) / (set.anti_dz_outer - set.dz_inner);
        value_out = fix16::clamp(value_out, FIX_0, FIX_1);

        if (set.anti_dz_inner > FIX_0)
        {
            value_out = set.anti_dz_inner + (FIX_1 - set.anti_dz_inner) * value_out;
        }
        if (set.curve != FIX_1)
        {
            value_out = fix16::pow(value_out, FIX_1 / set.curve);
        }
        if (set.anti_dz_outer < FIX_1)
        {
            value_out = fix16::clamp(value_out * (FIX_1 / (FIX_1 - set.anti_dz_outer)), FIX_0, FIX_1);
        }

        value_out *= set.dz_outer;
        return static_cast<uint8_t>(fix16_to_int(value_out * static_cast<int16_t>(Range::MAX<uint8_t>)));
    }

}; // class GamepadMapper

#endif // GAMEPAD_H