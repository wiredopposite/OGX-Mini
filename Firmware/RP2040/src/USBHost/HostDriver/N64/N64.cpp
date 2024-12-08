#include <cstring>

#include "USBHost/HostDriver/N64/N64.h"

void N64Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    tuh_hid_receive_report(address, instance);
}

void N64Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const N64::InReport* in_report = reinterpret_cast<const N64::InReport*>(report);
    if (std::memcmp(in_report, &prev_in_report_, sizeof(N64::InReport)) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    switch (in_report->buttons & N64::DPAD_MASK)
    {
        case N64::Buttons::DPAD_UP:
            gamepad.set_dpad_up();
            break;
        case N64::Buttons::DPAD_UP_RIGHT:
            gamepad.set_dpad_up_right();
            break;
        case N64::Buttons::DPAD_RIGHT:
            gamepad.set_dpad_right();   
            break;
        case N64::Buttons::DPAD_RIGHT_DOWN:
            gamepad.set_dpad_down_right();
            break;
        case N64::Buttons::DPAD_DOWN:
            gamepad.set_dpad_down();
            break;
        case N64::Buttons::DPAD_DOWN_LEFT:
            gamepad.set_dpad_down_left();
            break;
        case N64::Buttons::DPAD_LEFT:
            gamepad.set_dpad_left();
            break;
        case N64::Buttons::DPAD_LEFT_UP:    
            gamepad.set_dpad_up_left();
            break;  
        default:
            break;
    }

    if (in_report->buttons & N64::Buttons::A) gamepad.set_button_a();
    if (in_report->buttons & N64::Buttons::B) gamepad.set_button_b();
    if (in_report->buttons & N64::Buttons::L) gamepad.set_button_lb();
    if (in_report->buttons & N64::Buttons::R) gamepad.set_button_rb();
    if (in_report->buttons & N64::Buttons::START) gamepad.set_button_start();

    uint8_t joy_ry = N64::JOY_MID;
    uint8_t joy_rx = N64::JOY_MID;

    switch (in_report->buttons & (N64::Buttons::C_UP | N64::Buttons::C_RIGHT | N64::Buttons::C_DOWN | N64::Buttons::C_LEFT))
    {
        case N64::Buttons::C_UP:
            joy_ry = N64::JOY_MIN;
            break;
        case N64::Buttons::C_RIGHT:
            joy_rx = N64::JOY_MAX;
            break;
        case N64::Buttons::C_DOWN:
            joy_ry = N64::JOY_MAX;
            break;
        case N64::Buttons::C_LEFT:
            joy_rx = N64::JOY_MIN;
            break;
        case N64::Buttons::C_UP | N64::Buttons::C_RIGHT:
            joy_ry = N64::JOY_MIN;
            joy_rx = N64::JOY_MAX;
            break;
        case N64::Buttons::C_RIGHT | N64::Buttons::C_DOWN:
            joy_ry = N64::JOY_MAX;
            joy_rx = N64::JOY_MAX;
            break;
        case N64::Buttons::C_DOWN | N64::Buttons::C_LEFT:
            joy_ry = N64::JOY_MAX;
            joy_rx = N64::JOY_MIN;
            break;
        case N64::Buttons::C_LEFT | N64::Buttons::C_UP:   
            joy_ry = N64::JOY_MIN;
            joy_rx = N64::JOY_MIN;
            break;
        default:
            break;
    }

    gamepad.set_joystick_ry(joy_ry);
    gamepad.set_joystick_rx(joy_rx);

    gamepad.set_trigger_l((in_report->buttons & N64::Buttons::Z) ? UINT_8::MAX : UINT_8::MIN);

    gamepad.set_joystick_ly(in_report->joystick_y);
    gamepad.set_joystick_lx(in_report->joystick_x);

    tuh_hid_receive_report(address, instance);

    std::memcpy(&prev_in_report_, in_report, sizeof(N64::InReport));
}

bool N64Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}
