#include <cstring>

#include "host/usbh.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/XboxOG.h"

void XboxOGHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    gamepad.set_analog_host(true);
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

    Gamepad::PadIn gp_in;

    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_UP)    gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_DOWN)  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_LEFT)  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (in_report->buttons & XboxOG::GP::Buttons::DPAD_RIGHT) gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;

    if (in_report->a) gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->b) gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->x) gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->y) gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (in_report->black) gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->white) gp_in.buttons |= gamepad.MAP_BUTTON_RB;

    if (in_report->buttons & XboxOG::GP::Buttons::START)   gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report->buttons & XboxOG::GP::Buttons::BACK)    gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons & XboxOG::GP::Buttons::L3)      gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report->buttons & XboxOG::GP::Buttons::R3)      gp_in.buttons |= gamepad.MAP_BUTTON_R3;

    if (gamepad.analog_enabled())
    {
        gp_in.analog[gamepad.MAP_ANALOG_OFF_A]  = in_report->a;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_B]  = in_report->b;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_X]  = in_report->x;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_Y]  = in_report->y;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_LB] = in_report->black;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_RB] = in_report->white;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_UP]    = (in_report->buttons & XboxOG::GP::Buttons::DPAD_UP) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_DOWN]  = (in_report->buttons & XboxOG::GP::Buttons::DPAD_DOWN) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_LEFT]  = (in_report->buttons & XboxOG::GP::Buttons::DPAD_LEFT) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_RIGHT] = (in_report->buttons & XboxOG::GP::Buttons::DPAD_RIGHT) ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
    }

    gp_in.trigger_l = gamepad.scale_trigger_l(in_report->trigger_l);
    gp_in.trigger_r = gamepad.scale_trigger_r(in_report->trigger_r);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(in_report->joystick_lx, in_report->joystick_ly, true);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report->joystick_rx, in_report->joystick_ry, true);

    gamepad.set_pad_in(gp_in);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(XboxOG::GP::InReport));
}

bool XboxOGHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    Gamepad::PadOut gp_out = gamepad.get_pad_out();
    return tuh_xinput::set_rumble(address, instance, gp_out.rumble_l, gp_out.rumble_r, false);
}