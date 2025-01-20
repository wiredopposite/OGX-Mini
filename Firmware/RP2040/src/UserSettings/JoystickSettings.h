#ifndef _JOYSTICK_SETTINGS_H_
#define _JOYSTICK_SETTINGS_H_

#include <cstdint>

#include "libfixmath/fix16.hpp"

struct JoystickSettingsRaw;

struct JoystickSettings
{
    Fix16 dz_inner{Fix16(0.0f)};
    Fix16 dz_outer{Fix16(1.0f)};

    Fix16 anti_dz_circle{Fix16(0.0f)};
    Fix16 anti_dz_circle_y_scale{Fix16(0.0f)};
    Fix16 anti_dz_square{Fix16(0.0f)};
    Fix16 anti_dz_square_y_scale{Fix16(0.0f)};
    Fix16 anti_dz_angular{Fix16(0.0f)};
    Fix16 anti_dz_outer{Fix16(1.0f)};
    
    Fix16 axis_restrict{Fix16(0.0f)};
    Fix16 angle_restrict{Fix16(0.0f)};
    
    Fix16 diag_scale_min{Fix16(1.0f)};
    Fix16 diag_scale_max{Fix16(1.0f)};

    Fix16 curve{Fix16(1.0f)};

    bool uncap_radius{true};
    bool invert_y{false};
    bool invert_x{false};

    bool is_same(const JoystickSettingsRaw& raw) const;
    void set_from_raw(const JoystickSettingsRaw& raw);
};

#pragma pack(push, 1)
struct JoystickSettingsRaw
{
    fix16_t dz_inner{fix16_from_int(0)};
    fix16_t dz_outer{fix16_from_int(1)};

    fix16_t anti_dz_circle{fix16_from_int(0)};
    fix16_t anti_dz_circle_y_scale{fix16_from_int(0)};
    fix16_t anti_dz_square{fix16_from_int(0)};
    fix16_t anti_dz_square_y_scale{fix16_from_int(0)};
    fix16_t anti_dz_angular{fix16_from_int(0)};
    fix16_t anti_dz_outer{fix16_from_int(1)};
    
    fix16_t axis_restrict{fix16_from_int(0)};
    fix16_t angle_restrict{fix16_from_int(0)};
    
    fix16_t diag_scale_min{fix16_from_int(1)};
    fix16_t diag_scale_max{fix16_from_int(1)};

    fix16_t curve{fix16_from_int(1)};

    uint8_t uncap_radius{true};
    uint8_t invert_y{false};
    uint8_t invert_x{false};
};
static_assert(sizeof(JoystickSettingsRaw) == 55, "JoystickSettingsRaw is an unexpected size");
#pragma pack(pop)

#endif // _JOYSTICK_SETTINGS_H_