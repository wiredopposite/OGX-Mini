#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/PSClassic/PSClassic.h"

void PSClassicHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    tuh_hid_receive_report(address, instance);
}

void PSClassicHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const PSClassic::InReport* in_report = reinterpret_cast<const PSClassic::InReport*>(report);
    if (std::memcmp(&prev_in_report_, in_report, sizeof(PSClassic::InReport)) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp_in;

    switch (in_report->buttons & PSClassic::DPAD_MASK)
    {
        case PSClassic::Buttons::UP:
            gp_in.dpad |= gamepad.MAP_DPAD_UP;
            break;
        case PSClassic::Buttons::DOWN:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
            break;
        case PSClassic::Buttons::LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
            break;
        case PSClassic::Buttons::RIGHT: 
            gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
            break;
        case PSClassic::Buttons::UP_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_RIGHT;
            break;
        case PSClassic::Buttons::DOWN_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT;
            break;
        case PSClassic::Buttons::DOWN_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_LEFT;
            break;
        case PSClassic::Buttons::UP_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_LEFT;
            break;
        default:
            break;
    }

    if (in_report->buttons & PSClassic::Buttons::SQUARE)   gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->buttons & PSClassic::Buttons::CROSS)    gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons & PSClassic::Buttons::CIRCLE)   gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons & PSClassic::Buttons::TRIANGLE) gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (in_report->buttons & PSClassic::Buttons::L1)       gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons & PSClassic::Buttons::R1)       gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons & PSClassic::Buttons::SELECT)   gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons & PSClassic::Buttons::START)    gp_in.buttons |= gamepad.MAP_BUTTON_START;

    gp_in.trigger_l = (in_report->buttons & PSClassic::Buttons::L2) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
    gp_in.trigger_r = (in_report->buttons & PSClassic::Buttons::R2) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;

    gamepad.set_pad_in(gp_in);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, &in_report, sizeof(PSClassic::InReport));
}

bool PSClassicHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}