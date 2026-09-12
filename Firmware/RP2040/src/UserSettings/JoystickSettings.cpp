#include "UserSettings/JoystickSettings.h"

bool JoystickSettings::is_same(const JoystickSettingsRaw& raw) const
{
    return  dz_inner == Fix16(raw.dz_inner) &&
            dz_outer == Fix16(raw.dz_outer) &&
            anti_dz_circle == Fix16(raw.anti_dz_circle) &&
            anti_dz_circle_y_scale == Fix16(raw.anti_dz_circle_y_scale) &&
            anti_dz_square == Fix16(raw.anti_dz_square) &&
            anti_dz_square_y_scale == Fix16(raw.anti_dz_square_y_scale) &&
            anti_dz_angular == Fix16(raw.anti_dz_angular) &&
            anti_dz_outer == Fix16(raw.anti_dz_outer) &&
            axis_restrict == Fix16(raw.axis_restrict) &&
            angle_restrict == Fix16(raw.angle_restrict) &&
            diag_scale_min == Fix16(raw.diag_scale_min) &&
            diag_scale_max == Fix16(raw.diag_scale_max) &&
            curve == Fix16(raw.curve) &&
            uncap_radius == raw.uncap_radius &&
            invert_y == raw.invert_y &&
            invert_x == raw.invert_x;
}

bool JoystickSettings::raw_has_customization(const JoystickSettingsRaw& raw)
{
    static const JoystickSettingsRaw defaults{};
    return  raw.dz_inner != defaults.dz_inner ||
            raw.dz_outer != defaults.dz_outer ||
            raw.anti_dz_circle != defaults.anti_dz_circle ||
            raw.anti_dz_circle_y_scale != defaults.anti_dz_circle_y_scale ||
            raw.anti_dz_square != defaults.anti_dz_square ||
            raw.anti_dz_square_y_scale != defaults.anti_dz_square_y_scale ||
            raw.anti_dz_angular != defaults.anti_dz_angular ||
            raw.anti_dz_outer != defaults.anti_dz_outer ||
            raw.axis_restrict != defaults.axis_restrict ||
            raw.angle_restrict != defaults.angle_restrict ||
            raw.diag_scale_min != defaults.diag_scale_min ||
            raw.diag_scale_max != defaults.diag_scale_max ||
            raw.curve != defaults.curve ||
            raw.uncap_radius != defaults.uncap_radius ||
            raw.invert_y != defaults.invert_y ||
            raw.invert_x != defaults.invert_x;
}

void JoystickSettings::set_from_raw(const JoystickSettingsRaw& raw)
{
    dz_inner = Fix16(raw.dz_inner);
    dz_outer = Fix16(raw.dz_outer);
    anti_dz_circle = Fix16(raw.anti_dz_circle);
    anti_dz_circle_y_scale = Fix16(raw.anti_dz_circle_y_scale);
    anti_dz_square = Fix16(raw.anti_dz_square);
    anti_dz_square_y_scale = Fix16(raw.anti_dz_square_y_scale);
    anti_dz_angular = Fix16(raw.anti_dz_angular);
    anti_dz_outer = Fix16(raw.anti_dz_outer);
    axis_restrict = Fix16(raw.axis_restrict);
    angle_restrict = Fix16(raw.angle_restrict);
    diag_scale_min = Fix16(raw.diag_scale_min);
    diag_scale_max = Fix16(raw.diag_scale_max);
    curve = Fix16(raw.curve);
    uncap_radius = raw.uncap_radius;
    invert_y = raw.invert_y;
    invert_x = raw.invert_x;
}

void JoystickSettings::set_xbox360_stock_feel(bool right_stick)
{
    /* Reset to firmware defaults, then apply 360-like shaping.
     * Keep remapped deadzone mild (not Microsoft's ~24% software filter) so console games
     * that also apply deadzones are not double-dulled. Curve < 1 softens the center and
     * steepens the outer third — closer to a stock 360 pad than a near-linear Series stick. */
    *this = JoystickSettings{};
    dz_inner = Fix16(right_stick ? 0.09f : 0.08f);
    curve = Fix16(0.35f);
    uncap_radius = false;
}