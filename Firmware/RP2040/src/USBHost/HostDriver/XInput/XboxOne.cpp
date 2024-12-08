#include <cstring>

#include "host/usbh.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/XboxOne.h"

void XboxOneHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    tuh_xinput::receive_report(address, instance);
}

void XboxOneHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const XboxOne::InReport* in_report = reinterpret_cast<const XboxOne::InReport*>(report);
    if (std::memcmp(&prev_in_report_ + 4, in_report + 4, 14) == 0)
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    if (in_report->buttons[1] & XboxOne::Buttons1::DPAD_UP)    gamepad.set_dpad_up();
    if (in_report->buttons[1] & XboxOne::Buttons1::DPAD_DOWN)  gamepad.set_dpad_down();
    if (in_report->buttons[1] & XboxOne::Buttons1::DPAD_LEFT)  gamepad.set_dpad_left();
    if (in_report->buttons[1] & XboxOne::Buttons1::DPAD_RIGHT) gamepad.set_dpad_right();

    if (in_report->buttons[1] & XboxOne::Buttons1::L3)    gamepad.set_button_l3();
    if (in_report->buttons[1] & XboxOne::Buttons1::R3)    gamepad.set_button_r3();
    if (in_report->buttons[1] & XboxOne::Buttons1::LB)    gamepad.set_button_lb();
    if (in_report->buttons[1] & XboxOne::Buttons1::RB)    gamepad.set_button_rb();
    if (in_report->buttons[0] & XboxOne::Buttons0::BACK)  gamepad.set_button_back();
    if (in_report->buttons[0] & XboxOne::Buttons0::START) gamepad.set_button_start();
    if (in_report->buttons[0] & XboxOne::Buttons0::SYNC)  gamepad.set_button_misc();
    if (in_report->buttons[0] & XboxOne::Buttons0::GUIDE) gamepad.set_button_sys();
    if (in_report->buttons[0] & XboxOne::Buttons0::A)     gamepad.set_button_a();
    if (in_report->buttons[0] & XboxOne::Buttons0::B)     gamepad.set_button_b();
    if (in_report->buttons[0] & XboxOne::Buttons0::X)     gamepad.set_button_x();
    if (in_report->buttons[0] & XboxOne::Buttons0::Y)     gamepad.set_button_y();

    gamepad.set_trigger_l(static_cast<uint8_t>(in_report->trigger_l >> 2));
    gamepad.set_trigger_r(static_cast<uint8_t>(in_report->trigger_r >> 2));

    gamepad.set_joystick_lx(in_report->joystick_lx);
    gamepad.set_joystick_ly(in_report->joystick_ly, true);
    gamepad.set_joystick_rx(in_report->joystick_rx);
    gamepad.set_joystick_ry(in_report->joystick_ry, true);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, 18);
}

bool XboxOneHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return tuh_xinput::set_rumble(address, instance, gamepad.get_rumble_l().uint8(), gamepad.get_rumble_r().uint8(), false);
}