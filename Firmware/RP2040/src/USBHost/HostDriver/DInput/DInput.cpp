#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/DInput/DInput.h"

void DInputHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    gamepad.set_analog_enabled(true);
    tuh_hid_receive_report(address, instance);
}

void DInputHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const DInput::InReport* in_report = reinterpret_cast<const DInput::InReport*>(report);
    if (std::memcmp(&prev_in_report_, in_report, sizeof(DInput::InReport)) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    switch (in_report->dpad & DInput::DPAD_MASK)
    {
        case DInput::DPad::UP:
            gamepad.set_dpad_up();
            break;
        case DInput::DPad::DOWN:
            gamepad.set_dpad_down();
            break;
        case DInput::DPad::LEFT:
            gamepad.set_dpad_left();
            break;
        case DInput::DPad::RIGHT: 
            gamepad.set_dpad_right();
            break;
        case DInput::DPad::UP_RIGHT:
            gamepad.set_dpad_up_right();
            break;
        case DInput::DPad::DOWN_RIGHT:
            gamepad.set_dpad_down_right();
            break;
        case DInput::DPad::DOWN_LEFT:
            gamepad.set_dpad_down_left();
            break;
        case DInput::DPad::UP_LEFT:
            gamepad.set_dpad_up_left();
            break;
        default:
            break;
    }

    if (in_report->buttons[0] & DInput::Buttons0::SQUARE)   gamepad.set_button_x();
    if (in_report->buttons[0] & DInput::Buttons0::CROSS)    gamepad.set_button_a();
    if (in_report->buttons[0] & DInput::Buttons0::CIRCLE)   gamepad.set_button_b();
    if (in_report->buttons[0] & DInput::Buttons0::TRIANGLE) gamepad.set_button_y();
    if (in_report->buttons[0] & DInput::Buttons0::L1)       gamepad.set_button_lb();
    if (in_report->buttons[0] & DInput::Buttons0::R1)       gamepad.set_button_rb();
    if (in_report->buttons[1] & DInput::Buttons1::L3)       gamepad.set_button_l3();
    if (in_report->buttons[1] & DInput::Buttons1::R3)       gamepad.set_button_r3();    
    if (in_report->buttons[1] & DInput::Buttons1::SELECT)   gamepad.set_button_back();
    if (in_report->buttons[1] & DInput::Buttons1::START)    gamepad.set_button_start();
    if (in_report->buttons[1] & DInput::Buttons1::PS)       gamepad.set_button_sys();
    if (in_report->buttons[1] & DInput::Buttons1::TP)       gamepad.set_button_misc();

    if (gamepad.analog_enabled())
    {
        gamepad.set_analog_up(in_report->up_axis);
        gamepad.set_analog_down(in_report->down_axis);
        gamepad.set_analog_left(in_report->left_axis);
        gamepad.set_analog_right(in_report->right_axis);
        gamepad.set_analog_a(in_report->cross_axis);
        gamepad.set_analog_b(in_report->circle_axis);
        gamepad.set_analog_x(in_report->square_axis);
        gamepad.set_analog_y(in_report->triangle_axis);
        gamepad.set_analog_lb(in_report->l1_axis);
        gamepad.set_analog_rb(in_report->r1_axis);
    }

    if (in_report->l2_axis != 0)
    {
        gamepad.set_trigger_l(in_report->l2_axis);
    }
    else
    {
        gamepad.set_trigger_l((in_report->buttons[0] & DInput::Buttons0::L2) ? UINT_8::MAX : UINT_8::MIN);
    }
    if (in_report->r2_axis != 0)
    {
        gamepad.set_trigger_r(in_report->r2_axis);
    }
    else
    {
        gamepad.set_trigger_r((in_report->buttons[0] & DInput::Buttons0::R2) ? UINT_8::MAX : UINT_8::MIN);
    }

    gamepad.set_joystick_lx(in_report->joystick_lx);
    gamepad.set_joystick_ly(in_report->joystick_ly);
    gamepad.set_joystick_rx(in_report->joystick_rx);
    gamepad.set_joystick_ry(in_report->joystick_ry);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(DInput::InReport));
}

bool DInputHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}