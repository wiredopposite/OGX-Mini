#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "libfixmath/fix16.h"
#include "usb/device/device.h"
#include "assert_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed, aligned(4))) {
    fix16_t dz_inner;
    fix16_t dz_outer;

    fix16_t anti_dz_inner;
    fix16_t anti_dz_outer;

    fix16_t curve;
} trigger_settings_t;
_STATIC_ASSERT(sizeof(trigger_settings_t) == 20, "Trigger settings size mismatch");
_STATIC_ASSERT((sizeof(trigger_settings_t) % 4) == 0, "Trigger settings size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    fix16_t dz_inner;
    fix16_t dz_outer;

    fix16_t anti_dz_circle;
    fix16_t anti_dz_circle_y_scale;
    fix16_t anti_dz_square;
    fix16_t anti_dz_square_y_scale;
    fix16_t anti_dz_angular;
    fix16_t anti_dz_outer;
    
    fix16_t axis_restrict;
    fix16_t angle_restrict;
    
    fix16_t diag_scale_min;
    fix16_t diag_scale_max;

    fix16_t curve;

    uint8_t uncap_radius;
    uint8_t invert_y;
    uint8_t invert_x;

    uint8_t reserved;
} joystick_settings_t;
_STATIC_ASSERT(sizeof(joystick_settings_t) == 56, "Joystick settings size mismatch");
_STATIC_ASSERT((sizeof(joystick_settings_t) % 4) == 0, "Joystick settings size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t id; /* Profile ID */

    union {
        struct {
            uint8_t d_up; /* Uint8 bit offset */
            uint8_t d_down; /* Uint8 bit offset */
            uint8_t d_left; /* Uint8 bit offset */
            uint8_t d_right; /* Uint8 bit offset */
        };
        uint8_t dpad[GAMEPAD_DPAD_BIT_COUNT]; /* Raw array of uint8 bit offsets */
    };

    union {
        struct {
            uint8_t btn_a; /* Uint16 bit offset */
            uint8_t btn_b; /* Uint16 bit offset */
            uint8_t btn_x; /* Uint16 bit offset */
            uint8_t btn_y; /* Uint16 bit offset */
            uint8_t btn_l3; /* Uint16 bit offset */
            uint8_t btn_r3; /* Uint16 bit offset */
            uint8_t btn_lb; /* Uint16 bit offset */
            uint8_t btn_rb; /* Uint16 bit offset */
            uint8_t btn_lt; /* Uint16 bit offset */
            uint8_t btn_rt; /* Uint16 bit offset */
            uint8_t btn_back; /* Uint16 bit offset */
            uint8_t btn_start; /* Uint16 bit offset */
            uint8_t btn_sys; /* Uint16 bit offset */
            uint8_t btn_misc; /* Uint16 bit offset */
        };
        uint8_t btns[GAMEPAD_BTN_BIT_COUNT]; /* Raw array of uint16 bit offsets */
    };

    uint8_t analog_en; /* Analog buttons enabled */

    union {
        struct {
            uint8_t a_up; /* Analog button array byte offset */
            uint8_t a_down; /* Analog button array byte offset */
            uint8_t a_left; /* Analog button array byte offset */
            uint8_t a_right; /* Analog button array byte offset */
            uint8_t a_a; /* Analog button array byte offset */
            uint8_t a_b; /* Analog button array byte offset */
            uint8_t a_x; /* Analog button array byte offset */
            uint8_t a_y; /* Analog button array byte offset */
            uint8_t a_lb; /* Analog button array byte offset */
            uint8_t a_rb; /* Analog button array byte offset */
        };
        uint8_t analog[GAMEPAD_ANALOG_COUNT]; /* Raw array of analog button array byte offsets */
    };

    uint8_t reserved[2];

    joystick_settings_t joystick_l;
    joystick_settings_t joystick_r;
    trigger_settings_t trigger_l;
    trigger_settings_t trigger_r;
} user_profile_t;
_STATIC_ASSERT(sizeof(user_profile_t) == 184, "User profile size mismatch");
_STATIC_ASSERT((sizeof(user_profile_t) % 4) == 0, "User profile size mismatch");

void settings_init(void);
bool settings_valid_datetime(void);
void settings_write_datetime(void);
usbd_type_t settings_get_device_type(void);
void settings_get_profile_by_index(uint8_t index, user_profile_t* profile);
void settings_get_profile_by_id(uint8_t id, user_profile_t* profile);
uint8_t settings_get_active_profile_id(uint8_t index);
void settings_store_device_type(usbd_type_t type);
void settings_store_profile(uint8_t index, const user_profile_t* profile);

void settings_get_default_profile(user_profile_t* profile);
bool settings_is_default_joystick(const joystick_settings_t* joy_set);
bool settings_is_default_trigger(const trigger_settings_t* trig_set);

static inline void settings_scale_trigger(const trigger_settings_t* set, uint8_t* trigger) {

}

static inline void settings_scale_joysticks(const joystick_settings_t* set, int16_t* joy_x, int16_t* joy_y, bool invert_y) {
    // #define fix16_i(x) fix16_from_int(x)
    // #define fix16_f(x) fix16_from_float(x)

    // #define FIX_90 fix16_f(90.0f)
    // #define FIX_100 fix16_f(100.0f)
    // #define FIX_180 fix16_f(180.0f)
    // #define FIX_EPSILON fix16_f(0.0001f)
    // #define FIX_EPSILON2 fix16_f(0.001f)
    // #define FIX_ELLIPSE_DEF fix16_f(1.570796f)
    // #define FIX_DIAG_DIVISOR fix16_f(0.29289f)

    // fix16_t x = fix16_div(  set->invert_x 
    //                             ? fix16_i(range_invert_int16(*joy_x)) 
    //                             : fix16_i(*joy_x), 
    //                         fix16_i(R_INT16_MAX));
    // fix16_t y = fix16_div(  (set->invert_y ^ invert_y) 
    //                             ? fix16_i(range_invert_int16(*joy_y)) 
    //                             : fix16_i(*joy_y), 
    //                         fix16_i(R_INT16_MAX));

    // const fix16_t abs_x = fix16_abs(x);
    // const fix16_t abs_y = fix16_abs(y);
    // const fix16_t inv_axis_restrict = fix16_f(1.0f) / (fix16_f(1.0f) - set->axis_restrict);

    // fix16_t rAngle = (abs_x < FIX_EPSILON) 
    //     ? FIX_90 
    //     : fix16_rad2deg(fix16_abs(fix16_atan(fix16_div(y, x))));

    // fix16_t axial_x = ((abs_x <= set->axis_restrict) && (rAngle > fix16_f(45.0f))) 
    //     ? fix16_f(0.0f) 
    //     : fix16_mul((abs_x - set->axis_restrict), inv_axis_restrict);
            
    // fix16_t axial_y = ((abs_y <= set->axis_restrict) && (rAngle <= fix16_f(45.0f))) 
    //     ? fix16_f(0.0f) 
    //     : fix16_mul((abs_y - set->axis_restrict), inv_axis_restrict);

    // fix16_t in_magnitude = fix16_sqrt(fix16_sq(axial_x) + fix16_sq(axial_y));

    // if (in_magnitude < set->dz_inner) {
    //     *joy_x = 0;
    //     *joy_y = 0;
    //     return;
    // }

    // fix16_t angle = 
    //     fix16_abs(axial_x) < FIX_EPSILON 
    //         ? FIX_90 
    //         : fix16_rad2deg(fix16_abs(fix16_atan(axial_y / axial_x)));

    // fix16_t anti_r_scale =  (set->anti_dz_square_y_scale == fix16_f(0.0f)) 
    //                         ? set->anti_dz_square : set->anti_dz_square_y_scale;
    // fix16_t anti_dz_c = set->anti_dz_circle;

    // if ((anti_r_scale > fix16_f(0.0f)) && (anti_dz_c > fix16_f(0.0f))) {
    //     fix16_t anti_ellip_scale = fix16_div(anti_ellip_scale, anti_dz_c);
    //     fix16_t ellipse_angle = fix16_atan(fix16_mul(fix16_div(fix16_f(1.0f), anti_ellip_scale), fix16_tan(fix16_rad2deg(rAngle))));
    //     ellipse_angle = (ellipse_angle < fix16_f(0.0f)) ? FIX_ELLIPSE_DEF : ellipse_angle;

    //     fix16_t ellipse_x = fix16_cos(ellipse_angle);
    //     fix16_t ellipse_y = fix16_sqrt(fix16_mul(fix16_sq(anti_ellip_scale), (fix16_f(1.0f) - fix16_sq(ellipse_x))));
    //     anti_dz_c = fix16_mul(fix16_sqrt(fix16_sq(ellipse_x) + fix16_sq(ellipse_y)), anti_dz_c);
    // }

    // if (anti_dz_c > fix16_f(0.0f)) {
    //     fix16_t a = fix16_mul(anti_dz_c, (fix16_f(1.0f) - fix16_div(set->anti_dz_circle, set->dz_outer)));
    //     fix16_t b = fix16_mul(anti_dz_c, (fix16_f(1.0f) - set->anti_dz_square));
    //     anti_dz_c = fix16_div(anti_dz_c, fix16_div(a, b));
    //     // anti_dz_c = anti_dz_c / ((anti_dz_c * (fix16_f(1.0f) - set->anti_dz_circle / set->dz_outer)) / (anti_dz_c * (fix16_f(1.0f) - set->anti_dz_square)));
    // }

    // if ((abs_x > set->axis_restrict) && (abs_y > set->axis_restrict)) {
    //     const fix16_t FIX_ANGLE_MAX = fix16_div(set->angle_restrict, fix16_f(2.0f));

    //     if ((angle > fix16_f(0.0f)) && (angle < FIX_ANGLE_MAX)) {
    //         angle = fix16_f(0.0f);
    //     }
    //     if (angle > (FIX_90 - FIX_ANGLE_MAX)) {
    //         angle = FIX_90;
    //     }
    //     if (angle > FIX_ANGLE_MAX && angle < (FIX_90 - FIX_ANGLE_MAX)) {
    //         angle = fix16_div(fix16_mul((angle - FIX_ANGLE_MAX), FIX_90), ((FIX_90 - FIX_ANGLE_MAX) - FIX_ANGLE_MAX));
    //     }
    // }

    // fix16_t ref_angle = (angle < FIX_EPSILON2) ? fix16_f(0.0f) : angle;
    // fix16_t diagonal =  (angle > fix16_f(45.0f)) 
    //                     ? fix16_div(fix16_mul(angle - fix16_f(45.0f), (-fix16_f(45.0f))), fix16_f(45.0f)) + fix16_f(45.0f) 
    //                     : angle;

    // const fix16_t angle_comp = set->angle_restrict / fix16_f(2.0f);

    // if (angle < FIX_90 && angle > fix16_f(0.0f)) {
    //     angle = ((angle * ((FIX_90 - angle_comp) - angle_comp)) / FIX_90) + angle_comp;
    // }

    // if ((axial_x < fix16_f(0.0f)) && (axial_y > fix16_f(0.0f))) {
    //     angle = -angle;
    // }
    // if ((axial_x > fix16_f(0.0f)) && (axial_y < fix16_f(0.0f))) {
    //     angle = angle - FIX_180;
    // }
    // if ((axial_x < fix16_f(0.0f)) && (axial_y < fix16_f(0.0f))) {
    //     angle = angle + FIX_180;
    // }

    // //Deadzone Warp
    // fix16_t out_magnitude = (in_magnitude - set->dz_inner) / (set->anti_dz_outer - set->dz_inner);
    // out_magnitude = fix16_pow(out_magnitude, (fix16_f(1.0f) / set->curve)) * (set->dz_outer - anti_dz_c) + anti_dz_c;
    // out_magnitude = (out_magnitude > set->dz_outer && !set->uncap_radius) ? set->dz_outer : out_magnitude;

    // fix16_t d_scale = (((out_magnitude - anti_dz_c) * (set->diag_scale_max - set->diag_scale_min)) / (set->dz_outer - anti_dz_c)) + set->diag_scale_min;		
    // fix16_t c_scale = (diagonal * (fix16_f(1.0f) / fix16_sqrt(fix16_f(2.0f)))) / fix16_f(45.0f); //Both these lines scale the intensity of the warping
    // c_scale       = fix16_f(1.0f) - fix16_sqrt(fix16_f(1.0f) - c_scale * c_scale);     //based on a circular curve to the perfect diagonal
    // d_scale       = (c_scale * (d_scale - fix16_f(1.0f))) / FIX_DIAG_DIVISOR + fix16_f(1.0f);

    // out_magnitude = out_magnitude * d_scale;

    // //Scaling values for square antideadzone
    // fix16_t new_x = fix16_cos(fix16_deg2rad(angle)) * out_magnitude;
    // fix16_t new_y = fix16_sin(fix16_deg2rad(angle)) * out_magnitude;
    
    // //Magic angle wobble fix by user ME.
    // // if (angle > 45.0 && angle < 225.0) {
    // // 	newX = inv(Math.sin(deg2rad(angle - 90.0)))*outputMagnitude;
    // // 	newY = inv(Math.cos(deg2rad(angle - 270.0)))*outputMagnitude;
    // // }
    
    // //Square antideadzone scaling
    // fix16_t output_x = fix16_abs(new_x) * (fix16_f(1.0f) - fix16_div(set->anti_dz_square, set->dz_outer)) + set->anti_dz_square;
    // if (x < fix16_f(0.0f)) {
    //     output_x = -output_x;
    // }
    // if (ref_angle == FIX_90) {
    //     output_x = fix16_f(0.0f);
    // }
    
    // fix16_t output_y = fix16_abs(new_y) * (fix16_f(1.0f) - anti_r_scale / set->dz_outer) + anti_r_scale;
    // if (y < fix16_f(0.0f)) {
    //     output_y = -output_y;
    // }
    // if (ref_angle == fix16_f(0.0f)) {
    //     output_y = fix16_f(0.0f);
    // }

    // output_x = fix16_clamp(output_x, -fix16_f(1.0f), fix16_f(1.0f)) * fix16_i(R_INT16_MAX);
    // output_y = fix16_clamp(output_y, -fix16_f(1.0f), fix16_f(1.0f)) * fix16_i(R_INT16_MAX);

    // *joy_x = (int16_t)fix16_to_int(output_x);
    // *joy_y = (int16_t)fix16_to_int(output_y);

    // // return { static_cast<int16_t>(fix16_to_int(output_x)), static_cast<int16_t>(fix16_to_int(output_y)) };
}

#ifdef __cplusplus
}
#endif