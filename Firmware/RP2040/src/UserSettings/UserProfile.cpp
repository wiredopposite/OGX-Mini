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

    dpad_up = Gamepad::DPad::UP;
    dpad_down = Gamepad::DPad::DOWN;
    dpad_left = Gamepad::DPad::LEFT;
    dpad_right = Gamepad::DPad::RIGHT;

    button_a = Gamepad::Button::A;
    button_b = Gamepad::Button::B;
    button_x = Gamepad::Button::X;
    button_y = Gamepad::Button::Y;
    button_l3 = Gamepad::Button::L3;
    button_r3 = Gamepad::Button::R3;
    button_back = Gamepad::Button::BACK;
    button_start = Gamepad::Button::START;
    button_lb = Gamepad::Button::LB;
    button_rb = Gamepad::Button::RB;
    button_sys = Gamepad::Button::SYS;
    button_misc = Gamepad::Button::MISC;

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