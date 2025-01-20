#include <cstring>

#include "Gamepad/Gamepad.h"
#include "UserSettings/UserProfile.h"

UserProfile::UserProfile()
{
    id = 1;

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

    analog_enabled = 0;

    analog_off_up = Gamepad::ANALOG_OFF_UP;
    analog_off_down = Gamepad::ANALOG_OFF_DOWN;
    analog_off_left = Gamepad::ANALOG_OFF_LEFT;
    analog_off_right = Gamepad::ANALOG_OFF_RIGHT;
    analog_off_a = Gamepad::ANALOG_OFF_A;
    analog_off_b = Gamepad::ANALOG_OFF_B;
    analog_off_x = Gamepad::ANALOG_OFF_X;
    analog_off_y = Gamepad::ANALOG_OFF_Y;
    analog_off_lb = Gamepad::ANALOG_OFF_LB;
    analog_off_rb = Gamepad::ANALOG_OFF_RB;
}