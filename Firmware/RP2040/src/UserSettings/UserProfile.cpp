#include <cstring>

#include "Gamepad.h"
#include "UserSettings/UserProfile.h"

UserProfile::UserProfile()
{
    id = 1;

    dz_trigger_l = 0;
    dz_trigger_r = 0;
    dz_joystick_l = 0;
    dz_joystick_r = 0;

    invert_ly = 0;
    invert_ry = 0;

    dpad_up = Gamepad::DPAD_UP;
    dpad_down = Gamepad::DPAD_DOWN;
    dpad_left = Gamepad::DPAD_LEFT;
    dpad_right = Gamepad::DPAD_RIGHT;

    button_a = Gamepad::BUTTON_A;
    button_b = Gamepad::BUTTON_B;
    button_x = Gamepad::BUTTON_X;
    button_y = Gamepad::BUTTON_Y;
    button_l3 = Gamepad::BUTTON_L3;
    button_r3 = Gamepad::BUTTON_R3;
    button_back = Gamepad::BUTTON_BACK;
    button_start = Gamepad::BUTTON_START;
    button_lb = Gamepad::BUTTON_LB;
    button_rb = Gamepad::BUTTON_RB;
    button_sys = Gamepad::BUTTON_SYS;
    button_misc = Gamepad::BUTTON_MISC;

    analog_enabled = 1;

    analog_off_up = 0;
    analog_off_down = 1;
    analog_off_left = 2;
    analog_off_right = 3;
    analog_off_a = 4;
    analog_off_b = 5;
    analog_off_x = 6;
    analog_off_y = 7;
    analog_off_lb = 8;
    analog_off_rb = 9;
}