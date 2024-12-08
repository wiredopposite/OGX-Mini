#include <cstring>

#include "host/usbh.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/Xbox360.h"

void Xbox360Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    std::memset(&prev_in_report_, 0, sizeof(XInput::InReport));
    tuh_xinput::set_led(address, instance, idx_ + 1, true);
    tuh_xinput::receive_report(address, instance);
}

void Xbox360Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const XInput::InReport* in_report_ = reinterpret_cast<const XInput::InReport*>(report);
    if (std::memcmp(&prev_in_report_, in_report_, std::min(static_cast<size_t>(len), sizeof(XInput::InReport))) == 0)
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_UP)    gamepad.set_dpad_up();
    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_DOWN)  gamepad.set_dpad_down();
    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_LEFT)  gamepad.set_dpad_left();
    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_RIGHT) gamepad.set_dpad_right();

    if (in_report_->buttons[0] & XInput::Buttons0::START)  gamepad.set_button_start();
    if (in_report_->buttons[0] & XInput::Buttons0::BACK)   gamepad.set_button_back();
    if (in_report_->buttons[0] & XInput::Buttons0::L3)     gamepad.set_button_l3();
    if (in_report_->buttons[0] & XInput::Buttons0::R3)     gamepad.set_button_r3();
    if (in_report_->buttons[1] & XInput::Buttons1::LB)     gamepad.set_button_lb();
    if (in_report_->buttons[1] & XInput::Buttons1::RB)     gamepad.set_button_rb();
    if (in_report_->buttons[1] & XInput::Buttons1::HOME)   gamepad.set_button_sys();
    if (in_report_->buttons[1] & XInput::Buttons1::A)      gamepad.set_button_a();
    if (in_report_->buttons[1] & XInput::Buttons1::B)      gamepad.set_button_b();
    if (in_report_->buttons[1] & XInput::Buttons1::X)      gamepad.set_button_x();
    if (in_report_->buttons[1] & XInput::Buttons1::Y)      gamepad.set_button_y();

    gamepad.set_trigger_l(in_report_->trigger_l);
    gamepad.set_trigger_r(in_report_->trigger_r);

    gamepad.set_joystick_lx(in_report_->joystick_lx);
    gamepad.set_joystick_ly(in_report_->joystick_ly, true);
    gamepad.set_joystick_rx(in_report_->joystick_rx);
    gamepad.set_joystick_ry(in_report_->joystick_ry, true);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report_, sizeof(XInput::InReport));
}

bool Xbox360Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return tuh_xinput::set_rumble(address, 0, gamepad.get_rumble_l().uint8(), gamepad.get_rumble_r().uint8(), false);
}