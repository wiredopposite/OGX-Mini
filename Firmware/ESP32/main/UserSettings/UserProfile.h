#ifndef _USER_PROFILE_H_
#define _USER_PROFILE_H_

#include <cstdint>

#pragma pack(push, 1)
struct UserProfile
{
    uint8_t id;

    uint8_t dz_trigger_l;
    uint8_t dz_trigger_r;

    uint8_t dz_joystick_l;
    uint8_t dz_joystick_r;

    uint8_t invert_ly;
    uint8_t invert_ry;

    uint8_t dpad_up;
    uint8_t dpad_down;
    uint8_t dpad_left;
    uint8_t dpad_right;

    uint16_t button_a;
    uint16_t button_b;
    uint16_t button_x;
    uint16_t button_y;
    uint16_t button_l3;
    uint16_t button_r3;
    uint16_t button_back;
    uint16_t button_start;
    uint16_t button_lb;
    uint16_t button_rb;
    uint16_t button_sys;
    uint16_t button_misc;

    uint8_t analog_enabled;

    uint8_t analog_off_up;
    uint8_t analog_off_down;
    uint8_t analog_off_left;
    uint8_t analog_off_right;
    uint8_t analog_off_a;
    uint8_t analog_off_b;
    uint8_t analog_off_x;
    uint8_t analog_off_y;
    uint8_t analog_off_lb;
    uint8_t analog_off_rb;

    UserProfile();
};
static_assert(sizeof(UserProfile) == 46, "UserProfile struct size mismatch");
#pragma pack(pop)

#endif // _USER_PROFILE_H_