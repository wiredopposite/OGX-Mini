#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/PS5/PS5.h"

void PS5Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    out_report_.report_id = PS5::OutReportID::CONTROL;
    out_report_.control_flag[0] = 2;
    out_report_.control_flag[1] = 2;
    out_report_.led_control_flag = 0x01 | 0x02;
    out_report_.pulse_option = 1;
    out_report_.led_brightness = 0xFF;
    out_report_.player_number = idx_ + 1;
    out_report_.lightbar_blue = 0xFF;

    while (!tuh_hid_send_report(address, instance, 0, &out_report_, sizeof(PS5::OutReport)))
    {
        tuh_task();
    }

    out_report_ = PS5::OutReport();
    out_report_.report_id = PS5::OutReportID::RUMBLE;
    out_report_.control_flag[0] = 2;
    out_report_.control_flag[1] = 2;
    out_report_.led_control_flag = 0x04;
    
    tuh_hid_receive_report(address, instance);
}

void PS5Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const PS5::InReport* in_report = reinterpret_cast<const PS5::InReport*>(report);

    if (std::memcmp(&prev_in_report_.joystick_lx, &in_report->joystick_lx, sizeof(uint8_t) * 6) == 0 &&
        std::memcmp(prev_in_report_.buttons, in_report->buttons, sizeof(in_report->buttons)) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp_in;   

    switch (in_report->buttons[0] & PS5::DPAD_MASK)
    {
        case PS5::Buttons0::DPAD_UP:
            gp_in.dpad |= gamepad.MAP_DPAD_UP;
            break;
        case PS5::Buttons0::DPAD_UP_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_RIGHT;
            break;
        case PS5::Buttons0::DPAD_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
            break;
        case PS5::Buttons0::DPAD_RIGHT_DOWN:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT;
            break;
        case PS5::Buttons0::DPAD_DOWN:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
            break;
        case PS5::Buttons0::DPAD_DOWN_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_LEFT;
            break;
        case PS5::Buttons0::DPAD_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
            break;
        case PS5::Buttons0::DPAD_LEFT_UP:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_LEFT;
            break;
        default:
            break;
    }

    if (in_report->buttons[0] & PS5::Buttons0::SQUARE)   gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->buttons[0] & PS5::Buttons0::CROSS)    gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons[0] & PS5::Buttons0::CIRCLE)   gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons[0] & PS5::Buttons0::TRIANGLE) gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (in_report->buttons[1] & PS5::Buttons1::L1)       gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons[1] & PS5::Buttons1::R1)       gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons[1] & PS5::Buttons1::L3)       gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report->buttons[1] & PS5::Buttons1::R3)       gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (in_report->buttons[1] & PS5::Buttons1::SHARE)    gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons[1] & PS5::Buttons1::OPTIONS)  gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report->buttons[2] & PS5::Buttons2::PS)       gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in_report->buttons[2] & PS5::Buttons2::MUTE)     gp_in.buttons |= gamepad.MAP_BUTTON_MISC;

    gp_in.trigger_l = gamepad.scale_trigger_l(in_report->trigger_l);
    gp_in.trigger_r = gamepad.scale_trigger_r(in_report->trigger_r);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(in_report->joystick_lx, in_report->joystick_ly);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report->joystick_rx, in_report->joystick_ry);
    
    gamepad.set_pad_in(gp_in);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(PS5::InReport));
}

bool PS5Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    Gamepad::PadOut gp_out = gamepad.get_pad_out();
    out_report_.motor_left = gp_out.rumble_l;
    out_report_.motor_right = gp_out.rumble_r;

    if (tuh_hid_send_report(address, instance, 0, &out_report_, sizeof(PS5::OutReport)))
    {
        manage_rumble(gamepad);
        return true;
    }
    return false;
}