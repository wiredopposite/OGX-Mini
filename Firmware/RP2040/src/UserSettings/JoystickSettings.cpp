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