#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/PS5/PS5.h"

void PS5Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    std::memset(&out_report_, 0, sizeof(out_report_));
    out_report_.report_id = PS5::OUT_REPORT_ID;
    out_report_.valid_flag0 = 0x02;
    out_report_.valid_flag1 = 0x02;
    out_report_.valid_flag2 = 0x04;
    
    tuh_hid_receive_report(address, instance);
}

void PS5Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    in_report_ = *reinterpret_cast<const PS5::InReport*>(report);
    in_report_.seq_number = 0;

    if (std::memcmp(&prev_in_report_, &in_report_, PS5::IN_REPORT_CMP_SIZE) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    switch (in_report_.buttons[0] & PS5::DPAD_MASK)
    {
        case PS5::Buttons0::DPAD_UP:
            gamepad.set_dpad_up();
            break;
        case PS5::Buttons0::DPAD_UP_RIGHT:
            gamepad.set_dpad_up_right();
            break;
        case PS5::Buttons0::DPAD_RIGHT:
            gamepad.set_dpad_right();
            break;
        case PS5::Buttons0::DPAD_RIGHT_DOWN:
            gamepad.set_dpad_down_right();
            break;
        case PS5::Buttons0::DPAD_DOWN:
            gamepad.set_dpad_down();
            break;
        case PS5::Buttons0::DPAD_DOWN_LEFT:
            gamepad.set_dpad_down_left();
            break;
        case PS5::Buttons0::DPAD_LEFT:
            gamepad.set_dpad_left();
            break;
        case PS5::Buttons0::DPAD_LEFT_UP:
            gamepad.set_dpad_up_left();
            break;
        default:
            break;
    }

    if (in_report_.buttons[0] & PS5::Buttons0::SQUARE)   gamepad.set_button_x();
    if (in_report_.buttons[0] & PS5::Buttons0::CROSS)    gamepad.set_button_a();
    if (in_report_.buttons[0] & PS5::Buttons0::CIRCLE)   gamepad.set_button_b();
    if (in_report_.buttons[0] & PS5::Buttons0::TRIANGLE) gamepad.set_button_y();
    if (in_report_.buttons[1] & PS5::Buttons1::L1)       gamepad.set_button_lb();
    if (in_report_.buttons[1] & PS5::Buttons1::R1)       gamepad.set_button_rb();
    if (in_report_.buttons[1] & PS5::Buttons1::L3)       gamepad.set_button_l3();
    if (in_report_.buttons[1] & PS5::Buttons1::R3)       gamepad.set_button_r3();
    if (in_report_.buttons[1] & PS5::Buttons1::SHARE)    gamepad.set_button_back();
    if (in_report_.buttons[1] & PS5::Buttons1::OPTIONS)  gamepad.set_button_start();
    if (in_report_.buttons[2] & PS5::Buttons2::PS)       gamepad.set_button_sys();
    if (in_report_.buttons[2] & PS5::Buttons2::MUTE)     gamepad.set_button_misc();
    
    gamepad.set_trigger_l(in_report_.trigger_l);
    gamepad.set_trigger_r(in_report_.trigger_r);

    gamepad.set_joystick_lx(in_report_.joystick_lx);
    gamepad.set_joystick_ly(in_report_.joystick_ly);
    gamepad.set_joystick_rx(in_report_.joystick_rx);
    gamepad.set_joystick_ry(in_report_.joystick_ry);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, &in_report_, PS5::IN_REPORT_CMP_SIZE);
}

bool PS5Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    out_report_.motor_left = gamepad.get_rumble_l().uint8();
    out_report_.motor_right = gamepad.get_rumble_r().uint8();

    if (tuh_hid_send_report(address, instance, 0, &out_report_, sizeof(PS5::OutReport)))
    {
        manage_rumble(gamepad);
        return true;
    }
    return false;
}