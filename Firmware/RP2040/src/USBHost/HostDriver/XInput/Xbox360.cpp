#include <cstring>

#include "host/usbh.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/Xbox360.h"

void Xbox360Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
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

    Gamepad::PadIn gp_in;

    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_UP)    gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_DOWN)  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_LEFT)  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (in_report_->buttons[0] & XInput::Buttons0::DPAD_RIGHT) gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;

    if (in_report_->buttons[0] & XInput::Buttons0::START)  gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report_->buttons[0] & XInput::Buttons0::BACK)   gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report_->buttons[0] & XInput::Buttons0::L3)     gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report_->buttons[0] & XInput::Buttons0::R3)     gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (in_report_->buttons[1] & XInput::Buttons1::LB)     gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report_->buttons[1] & XInput::Buttons1::RB)     gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report_->buttons[1] & XInput::Buttons1::HOME)   gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in_report_->buttons[1] & XInput::Buttons1::A)      gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report_->buttons[1] & XInput::Buttons1::B)      gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report_->buttons[1] & XInput::Buttons1::X)      gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report_->buttons[1] & XInput::Buttons1::Y)      gp_in.buttons |= gamepad.MAP_BUTTON_Y;

    gp_in.trigger_l = gamepad.scale_trigger_l(in_report_->trigger_l);
    gp_in.trigger_r = gamepad.scale_trigger_r(in_report_->trigger_r);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(in_report_->joystick_lx, in_report_->joystick_ly, true);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report_->joystick_rx, in_report_->joystick_ry, true);

    gamepad.set_pad_in(gp_in);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report_, sizeof(XInput::InReport));
}

bool Xbox360Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    Gamepad::PadOut gp_out = gamepad.get_pad_out();
    return tuh_xinput::set_rumble(address, 0, gp_out.rumble_l, gp_out.rumble_r, false);
}