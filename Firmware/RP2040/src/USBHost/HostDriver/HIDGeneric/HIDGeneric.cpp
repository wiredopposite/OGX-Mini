#include <memory>

#include "USBHost/HIDParser/HIDReportDescriptor.h"
#include "USBHost/HostDriver/HIDGeneric/HIDGeneric.h"

void HIDHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    if (!report_desc || desc_len == 0)
    {
        return;
    }
    
    report_desc_len_ = desc_len;
    std::memcpy(report_desc_buffer_.data(), report_desc, std::min(static_cast<size_t>(report_desc_len_), report_desc_buffer_.size()));
    hid_joystick_ = std::make_unique<HIDJoystick>(std::make_shared<HIDReportDescriptor>(report_desc_buffer_.data(), report_desc_len_));

    tuh_hid_receive_report(address, instance);
}

void HIDHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    if (std::memcmp(prev_report_in_.data(), report, len) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    std::memcpy(prev_report_in_.data(), report, len);
    if (!hid_joystick_->parseData(const_cast<uint8_t*>(report), len, &hid_joystick_data_))
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    gamepad.reset_pad();

    switch (hid_joystick_data_.hat_switch)
    {
        case HIDJoystickHatSwitch::UP:
            gamepad.set_dpad_up();
            break;
        case HIDJoystickHatSwitch::UP_RIGHT:
            gamepad.set_dpad_up_right();
            break;
        case HIDJoystickHatSwitch::RIGHT:
            gamepad.set_dpad_right();
            break;
        case HIDJoystickHatSwitch::DOWN_RIGHT:
            gamepad.set_dpad_down_right();
            break;
        case HIDJoystickHatSwitch::DOWN:
            gamepad.set_dpad_down();
            break;
        case HIDJoystickHatSwitch::DOWN_LEFT:
            gamepad.set_dpad_down_left();
            break;
        case HIDJoystickHatSwitch::LEFT:
            gamepad.set_dpad_left();
            break;
        case HIDJoystickHatSwitch::UP_LEFT:
            gamepad.set_dpad_up_left();
            break;
        default:
            break;
    }

    gamepad.set_joystick_lx(hid_joystick_data_.X);
    gamepad.set_joystick_ly(hid_joystick_data_.Y);
    gamepad.set_joystick_rx(hid_joystick_data_.Z);
    gamepad.set_joystick_ry(hid_joystick_data_.Rz);

    if (hid_joystick_data_.buttons[1])  gamepad.set_button_x();
    if (hid_joystick_data_.buttons[2])  gamepad.set_button_a();
    if (hid_joystick_data_.buttons[3])  gamepad.set_button_b();
    if (hid_joystick_data_.buttons[4])  gamepad.set_button_y();
    if (hid_joystick_data_.buttons[5])  gamepad.set_button_lb();
    if (hid_joystick_data_.buttons[6])  gamepad.set_button_rb();
    if (hid_joystick_data_.buttons[7])  gamepad.set_trigger_l(UINT_8::MAX);
    if (hid_joystick_data_.buttons[8])  gamepad.set_trigger_r(UINT_8::MAX);
    if (hid_joystick_data_.buttons[9])  gamepad.set_button_back();
    if (hid_joystick_data_.buttons[10]) gamepad.set_button_start();
    if (hid_joystick_data_.buttons[11]) gamepad.set_button_l3();
    if (hid_joystick_data_.buttons[12]) gamepad.set_button_r3();
    if (hid_joystick_data_.buttons[13]) gamepad.set_button_sys();
    if (hid_joystick_data_.buttons[14]) gamepad.set_button_misc();

    tuh_hid_receive_report(address, instance);
}

bool HIDHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return true;
}