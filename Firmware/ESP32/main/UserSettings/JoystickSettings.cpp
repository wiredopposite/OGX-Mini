#include "Board/ogxm_log.h"
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

void JoystickSettingsRaw::log_values()
{
    OGXM_LOG("dz_inner: %f\n", fix16_to_float(dz_inner));
    OGXM_LOG_HEX("dz_inner: ", reinterpret_cast<uint8_t*>(&dz_inner), sizeof(dz_inner));
    OGXM_LOG("dz_outer: %f\n", fix16_to_float(dz_outer));
    OGXM_LOG("anti_dz_circle: %f\n", fix16_to_float(anti_dz_circle));
    OGXM_LOG("anti_dz_circle_y_scale: %f\n", fix16_to_float(anti_dz_circle_y_scale));
    OGXM_LOG("anti_dz_square: %f\n", fix16_to_float(anti_dz_square));
    OGXM_LOG("anti_dz_square_y_scale: %f\n", fix16_to_float(anti_dz_square_y_scale));
    OGXM_LOG("anti_dz_angular: %f\n", fix16_to_float(anti_dz_angular));
    OGXM_LOG("anti_dz_outer: %f\n", fix16_to_float(anti_dz_outer));
    OGXM_LOG("axis_restrict: %f\n", fix16_to_float(axis_restrict));
    OGXM_LOG("angle_restrict: %f\n", fix16_to_float(angle_restrict));
    OGXM_LOG("diag_scale_min: %f\n", fix16_to_float(diag_scale_min));
    OGXM_LOG("diag_scale_max: %f\n", fix16_to_float(diag_scale_max));
    OGXM_LOG("curve: %f\n", fix16_to_float(curve));
    OGXM_LOG("uncap_radius: %d\n", uncap_radius);
    OGXM_LOG("invert_y: %d\n", invert_y);
    OGXM_LOG("invert_x: %d\n", invert_x);
}