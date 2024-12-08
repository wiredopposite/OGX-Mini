#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/PS4/PS4.h"

void PS4Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    out_report_.report_id = 0x05;
    out_report_.set_led = 1;
    out_report_.lightbar_blue = 0xFF / 2;

    tuh_hid_receive_report(address, instance);
}

void PS4Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    std::memcpy(&in_report_, report, std::min(static_cast<size_t>(len), sizeof(PS4::InReport)));
    in_report_.buttons[2] &= PS4::COUNTER_MASK;
    if (std::memcmp(reinterpret_cast<const PS4::InReport*>(report), &prev_in_report_, sizeof(PS4::InReport)) == 0)
    {
        return;
    }

    gamepad.reset_buttons();

    switch (in_report_.buttons[0] & PS4::DPAD_MASK)
    {
        case PS4::Buttons0::DPAD_UP:
            gamepad.set_dpad_up();
            break;
        case PS4::Buttons0::DPAD_DOWN:
            gamepad.set_dpad_down();
            break;
        case PS4::Buttons0::DPAD_LEFT:
            gamepad.set_dpad_left();    
            break;
        case PS4::Buttons0::DPAD_RIGHT: 
            gamepad.set_dpad_right();
            break;
        case PS4::Buttons0::DPAD_UP_RIGHT:
            gamepad.set_dpad_up_right();
            break;
        case PS4::Buttons0::DPAD_RIGHT_DOWN:
            gamepad.set_dpad_down_right();
            break;
        case PS4::Buttons0::DPAD_DOWN_LEFT:
            gamepad.set_dpad_down_left();
            break;
        case PS4::Buttons0::DPAD_LEFT_UP:
            gamepad.set_dpad_up_left();
            break;
        default:
            break;
    }

    if (in_report_.buttons[0] & PS4::Buttons0::SQUARE)   gamepad.set_button_x();
    if (in_report_.buttons[0] & PS4::Buttons0::CROSS)    gamepad.set_button_a();
    if (in_report_.buttons[0] & PS4::Buttons0::CIRCLE)   gamepad.set_button_b();
    if (in_report_.buttons[0] & PS4::Buttons0::TRIANGLE) gamepad.set_button_y();    
    if (in_report_.buttons[1] & PS4::Buttons1::L1)       gamepad.set_button_lb();
    if (in_report_.buttons[1] & PS4::Buttons1::R1)       gamepad.set_button_rb();
    if (in_report_.buttons[1] & PS4::Buttons1::L3)       gamepad.set_button_l3();
    if (in_report_.buttons[1] & PS4::Buttons1::R3)       gamepad.set_button_r3();
    if (in_report_.buttons[1] & PS4::Buttons1::SHARE)    gamepad.set_button_back();
    if (in_report_.buttons[1] & PS4::Buttons1::OPTIONS)  gamepad.set_button_start();
    if (in_report_.buttons[2] & PS4::Buttons2::PS)       gamepad.set_button_sys();
    if (in_report_.buttons[2] & PS4::Buttons2::TP)       gamepad.set_button_misc();

    gamepad.set_trigger_l(in_report_.trigger_l);
    gamepad.set_trigger_r(in_report_.trigger_r);

    gamepad.set_joystick_lx(in_report_.joystick_lx);
    gamepad.set_joystick_ly(in_report_.joystick_ly);
    gamepad.set_joystick_rx(in_report_.joystick_rx);
    gamepad.set_joystick_ry(in_report_.joystick_ry);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, &in_report_, sizeof(PS4::InReport));
}

bool PS4Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    out_report_.motor_left = gamepad.get_rumble_l().uint8();
    out_report_.motor_right = gamepad.get_rumble_r().uint8();
    out_report_.set_rumble = (out_report_.motor_left != 0 || out_report_.motor_right != 0) ? 1 : 0;

    if (tuh_hid_send_report(address, instance, 0, reinterpret_cast<const uint8_t*>(&out_report_), sizeof(PS4::OutReport)))
    {
        manage_rumble(gamepad);
        return true;
    }
    return false;
}