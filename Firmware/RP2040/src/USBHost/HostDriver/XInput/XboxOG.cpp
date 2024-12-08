#include <cstring>

#include "host/usbh.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/XboxOG.h"

void XboxOGHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    gamepad.set_analog_enabled(true);
    std::memset(&prev_in_report_, 0, sizeof(XboxOG::GP::InReport));
    tuh_xinput::receive_report(address, instance);
}

void XboxOGHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const XboxOG::GP::InReport* in_report = reinterpret_cast<const XboxOG::GP::InReport*>(report);
    if (std::memcmp(&prev_in_report_, in_report, std::min(static_cast<size_t>(len), sizeof(XboxOG::GP::InReport))) == 0)
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_UP)    gamepad.set_dpad_up();
    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_DOWN)  gamepad.set_dpad_down();
    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_LEFT)  gamepad.set_dpad_left();
    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_RIGHT) gamepad.set_dpad_right();

    if (in_report->a) gamepad.set_button_a();
    if (in_report->b) gamepad.set_button_b();
    if (in_report->x) gamepad.set_button_x();
    if (in_report->y) gamepad.set_button_y();
    if (in_report->black) gamepad.set_button_rb();
    if (in_report->white) gamepad.set_button_lb();

    if (in_report->buttons & XboxOG::GP::Buttons::START)   gamepad.set_button_start();
    if (in_report->buttons & XboxOG::GP::Buttons::BACK)    gamepad.set_button_back();
    if (in_report->buttons & XboxOG::GP::Buttons::L3)      gamepad.set_button_l3();
    if (in_report->buttons & XboxOG::GP::Buttons::R3)      gamepad.set_button_r3();

    if (gamepad.analog_enabled())
    {
        gamepad.set_analog_a(in_report->a);
        gamepad.set_analog_b(in_report->b);
        gamepad.set_analog_x(in_report->x);
        gamepad.set_analog_y(in_report->y);
        gamepad.set_analog_lb(in_report->black);
        gamepad.set_analog_rb(in_report->white);
        gamepad.set_analog_up((in_report->buttons & XboxOG::GP::Buttons::DPAD_UP) ? UINT_8::MAX : UINT_8::MIN);
        gamepad.set_analog_down((in_report->buttons & XboxOG::GP::Buttons::DPAD_DOWN) ? UINT_8::MAX : UINT_8::MIN);
        gamepad.set_analog_left((in_report->buttons & XboxOG::GP::Buttons::DPAD_LEFT) ? UINT_8::MAX : UINT_8::MIN);
        gamepad.set_analog_right((in_report->buttons & XboxOG::GP::Buttons::DPAD_RIGHT) ? UINT_8::MAX : UINT_8::MIN);
    }

    gamepad.set_trigger_l(in_report->trigger_l);
    gamepad.set_trigger_r(in_report->trigger_r);

    gamepad.set_joystick_lx(in_report->joystick_lx);
    gamepad.set_joystick_ly(in_report->joystick_ly, true);
    gamepad.set_joystick_rx(in_report->joystick_rx);
    gamepad.set_joystick_ry(in_report->joystick_ry, true);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(XboxOG::GP::InReport));
}

bool XboxOGHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return tuh_xinput::set_rumble(address, instance, gamepad.get_rumble_l().uint8(), gamepad.get_rumble_r().uint8(), false);
}