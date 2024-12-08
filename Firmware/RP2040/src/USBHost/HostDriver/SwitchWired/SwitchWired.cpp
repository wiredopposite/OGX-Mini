#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/SwitchWired/SwitchWired.h"

void SwitchWiredHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    tuh_hid_receive_report(address, instance);
}

void SwitchWiredHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const SwitchWired::InReport* in_report = reinterpret_cast<const SwitchWired::InReport*>(report);
    if (std::memcmp(&prev_in_report_, in_report, sizeof(SwitchWired::InReport)) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    switch (in_report->dpad)
    {
        case SwitchWired::DPad::UP:
            gamepad.set_dpad_up();
            break;
        case SwitchWired::DPad::DOWN:
            gamepad.set_dpad_down();
            break;
        case SwitchWired::DPad::LEFT:
            gamepad.set_dpad_left();
            break;
        case SwitchWired::DPad::RIGHT:
            gamepad.set_dpad_right();
            break;
        case SwitchWired::DPad::UP_RIGHT:
            gamepad.set_dpad_up_right();
            break;
        case SwitchWired::DPad::DOWN_RIGHT:
            gamepad.set_dpad_down_right();
            break;
        case SwitchWired::DPad::DOWN_LEFT:
            gamepad.set_dpad_down_left();
            break;
        case SwitchWired::DPad::UP_LEFT:
            gamepad.set_dpad_up_left();
            break;
        default:
            break;
    }

    if (in_report->buttons & SwitchWired::Buttons::Y)       gamepad.set_button_x();
    if (in_report->buttons & SwitchWired::Buttons::B)       gamepad.set_button_a();
    if (in_report->buttons & SwitchWired::Buttons::A)       gamepad.set_button_b();
    if (in_report->buttons & SwitchWired::Buttons::X)       gamepad.set_button_y();
    if (in_report->buttons & SwitchWired::Buttons::L)       gamepad.set_button_lb();
    if (in_report->buttons & SwitchWired::Buttons::R)       gamepad.set_button_rb();
    if (in_report->buttons & SwitchWired::Buttons::MINUS)   gamepad.set_button_back();
    if (in_report->buttons & SwitchWired::Buttons::PLUS)    gamepad.set_button_start();
    if (in_report->buttons & SwitchWired::Buttons::HOME)    gamepad.set_button_sys();
    if (in_report->buttons & SwitchWired::Buttons::CAPTURE) gamepad.set_button_misc();
    if (in_report->buttons & SwitchWired::Buttons::L3)      gamepad.set_button_l3();
    if (in_report->buttons & SwitchWired::Buttons::R3)      gamepad.set_button_r3();

    gamepad.set_trigger_l((in_report->buttons & SwitchWired::Buttons::ZL) ? UINT_8::MAX : UINT_8::MIN);
    gamepad.set_trigger_r((in_report->buttons & SwitchWired::Buttons::ZR) ? UINT_8::MAX : UINT_8::MIN);

    gamepad.set_joystick_lx(in_report->joystick_lx);
    gamepad.set_joystick_ly(in_report->joystick_ly);
    gamepad.set_joystick_rx(in_report->joystick_rx);
    gamepad.set_joystick_ry(in_report->joystick_ry);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(SwitchWired::InReport));
}

bool SwitchWiredHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}