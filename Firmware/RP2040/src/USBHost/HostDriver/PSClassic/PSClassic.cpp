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

    gamepad.reset_buttons();

    switch (in_report->buttons & PSClassic::DPAD_MASK)
    {
        case PSClassic::Buttons::UP:
            gamepad.set_dpad_up();
            break;
        case PSClassic::Buttons::DOWN:
            gamepad.set_dpad_down();
            break;
        case PSClassic::Buttons::LEFT:
            gamepad.set_dpad_left();
            break;
        case PSClassic::Buttons::RIGHT: 
            gamepad.set_dpad_right();
            break;
        case PSClassic::Buttons::UP_RIGHT:
            gamepad.set_dpad_up_right();
            break;
        case PSClassic::Buttons::DOWN_RIGHT:
            gamepad.set_dpad_down_right();
            break;
        case PSClassic::Buttons::DOWN_LEFT:
            gamepad.set_dpad_down_left();
            break;
        case PSClassic::Buttons::UP_LEFT:
            gamepad.set_dpad_up_left();
            break;
        default:
            break;
    }

    if (in_report->buttons & PSClassic::Buttons::SQUARE)   gamepad.set_button_x();
    if (in_report->buttons & PSClassic::Buttons::CROSS)    gamepad.set_button_a();
    if (in_report->buttons & PSClassic::Buttons::CIRCLE)   gamepad.set_button_b();
    if (in_report->buttons & PSClassic::Buttons::TRIANGLE) gamepad.set_button_y();
    if (in_report->buttons & PSClassic::Buttons::L1)       gamepad.set_button_lb();
    if (in_report->buttons & PSClassic::Buttons::R1)       gamepad.set_button_rb();
    if (in_report->buttons & PSClassic::Buttons::SELECT)   gamepad.set_button_back();
    if (in_report->buttons & PSClassic::Buttons::START)    gamepad.set_button_start();

    gamepad.set_trigger_l(in_report->buttons);
    gamepad.set_trigger_r(in_report->buttons);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, &in_report, sizeof(PSClassic::InReport));
}

bool PSClassicHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}