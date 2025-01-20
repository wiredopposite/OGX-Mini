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

    Gamepad::PadIn gp_in;   

    switch (in_report->dpad)
    {
        case SwitchWired::DPad::UP:
            gp_in.dpad |= gamepad.MAP_DPAD_UP;
            break;
        case SwitchWired::DPad::DOWN:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
            break;
        case SwitchWired::DPad::LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
            break;
        case SwitchWired::DPad::RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
            break;
        case SwitchWired::DPad::UP_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_RIGHT;
            break;
        case SwitchWired::DPad::DOWN_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT;
            break;
        case SwitchWired::DPad::DOWN_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_LEFT;
            break;
        case SwitchWired::DPad::UP_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_LEFT;
            break;
        default:
            break;
    }

    if (in_report->buttons & SwitchWired::Buttons::Y)       gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->buttons & SwitchWired::Buttons::B)       gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons & SwitchWired::Buttons::A)       gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons & SwitchWired::Buttons::X)       gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (in_report->buttons & SwitchWired::Buttons::L)       gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons & SwitchWired::Buttons::R)       gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons & SwitchWired::Buttons::MINUS)   gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons & SwitchWired::Buttons::PLUS)    gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report->buttons & SwitchWired::Buttons::HOME)    gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in_report->buttons & SwitchWired::Buttons::CAPTURE) gp_in.buttons |= gamepad.MAP_BUTTON_MISC;   
    if (in_report->buttons & SwitchWired::Buttons::L3)      gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report->buttons & SwitchWired::Buttons::R3)      gp_in.buttons |= gamepad.MAP_BUTTON_R3;

    gp_in.trigger_l = (in_report->buttons & SwitchWired::Buttons::ZL) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
    gp_in.trigger_r = (in_report->buttons & SwitchWired::Buttons::ZR) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(in_report->joystick_lx, in_report->joystick_ly);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report->joystick_rx, in_report->joystick_ry);

    gamepad.set_pad_in(gp_in);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(SwitchWired::InReport));
}

bool SwitchWiredHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}