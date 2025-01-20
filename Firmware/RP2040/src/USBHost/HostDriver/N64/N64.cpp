#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

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

    Gamepad::PadIn gp_in;   

    switch (in_report->buttons & N64::DPAD_MASK)
    {
        case N64::Buttons::DPAD_UP:
            gp_in.dpad |= gamepad.MAP_DPAD_UP;
            break;
        case N64::Buttons::DPAD_UP_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_RIGHT;
            break;
        case N64::Buttons::DPAD_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;  
            break;
        case N64::Buttons::DPAD_RIGHT_DOWN:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT;
            break;
        case N64::Buttons::DPAD_DOWN:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
            break;
        case N64::Buttons::DPAD_DOWN_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_LEFT;
            break;
        case N64::Buttons::DPAD_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
            break;
        case N64::Buttons::DPAD_LEFT_UP:    
            gp_in.dpad |= gamepad.MAP_DPAD_UP_LEFT;
            break;  
        default:
            break;
    }

    if (in_report->buttons & N64::Buttons::A) gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons & N64::Buttons::B) gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons & N64::Buttons::L) gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons & N64::Buttons::R) gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons & N64::Buttons::START) gp_in.buttons |= gamepad.MAP_BUTTON_START;

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

    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report->joystick_x, in_report->joystick_y);
    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(joy_rx, joy_ry);

    gp_in.trigger_l = (in_report->buttons & N64::Buttons::L) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;

    gamepad.set_pad_in(gp_in);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(N64::InReport));
}

bool N64Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}
