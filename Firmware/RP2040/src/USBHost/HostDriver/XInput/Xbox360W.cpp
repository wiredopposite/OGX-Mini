#include <cstring>
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include <hardware/irq.h>

#include "host/usbh.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/Xbox360W.h"

Xbox360WHost::~Xbox360WHost()
{
    TaskQueue::Core1::cancel_delayed_task(timer_info_.task_id);
}

void Xbox360WHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    std::memset(&prev_in_report_, 0, sizeof(XInput::InReportWireless));

    timer_info_.address = address;
    timer_info_.instance = instance;
    timer_info_.led_quadrant = idx_ + 1;
    timer_info_.task_id = TaskQueue::Core1::get_new_task_id();

    //Repeatedly set the LED incase of disconnect, may rework the XInput driver to handle this
    TaskQueue::Core1::queue_delayed_task(timer_info_.task_id, 1000, true, [this]
    {
        tuh_xinput::set_led(timer_info_.address, timer_info_.instance, timer_info_.led_quadrant, false);
    });
    
    tuh_xinput::receive_report(address, instance);
}

void Xbox360WHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const XInput::InReportWireless* in_report = reinterpret_cast<const XInput::InReportWireless*>(report);
    if (std::memcmp(&prev_in_report_, in_report, std::min(static_cast<size_t>(len), sizeof(XInput::InReportWireless))) == 0)
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp_in;

    if (in_report->buttons[0] & XInput::Buttons0::DPAD_UP)    gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (in_report->buttons[0] & XInput::Buttons0::DPAD_DOWN)  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (in_report->buttons[0] & XInput::Buttons0::DPAD_LEFT)  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (in_report->buttons[0] & XInput::Buttons0::DPAD_RIGHT) gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;

    if (in_report->buttons[0] & XInput::Buttons0::START)  gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report->buttons[0] & XInput::Buttons0::BACK)   gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons[0] & XInput::Buttons0::L3)     gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report->buttons[0] & XInput::Buttons0::R3)     gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (in_report->buttons[1] & XInput::Buttons1::LB)     gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons[1] & XInput::Buttons1::RB)     gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons[1] & XInput::Buttons1::HOME)   gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in_report->buttons[1] & XInput::Buttons1::A)      gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons[1] & XInput::Buttons1::B)      gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons[1] & XInput::Buttons1::X)      gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->buttons[1] & XInput::Buttons1::Y)      gp_in.buttons |= gamepad.MAP_BUTTON_Y;

    gp_in.trigger_l = in_report->trigger_l;
    gp_in.trigger_r = in_report->trigger_r;

    gp_in.joystick_lx = in_report->joystick_lx;
    gp_in.joystick_ly = Scale::invert_joy(in_report->joystick_ly);
    gp_in.joystick_rx = in_report->joystick_rx;
    gp_in.joystick_ry = Scale::invert_joy(in_report->joystick_ry);

    if ((in_report->command[1] & 2) && (in_report->chatpad_status == 0x00))
    {
        gp_in.chatpad[0] = in_report->chatpad[0];
        gp_in.chatpad[1] = in_report->chatpad[1];
        gp_in.chatpad[2] = in_report->chatpad[2];
    }

    gamepad.set_pad_in(gp_in);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(XInput::InReportWireless));
}

bool Xbox360WHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    Gamepad::PadOut gp_out = gamepad.get_pad_out(); 
    return tuh_xinput::set_rumble(address, instance, gp_out.rumble_l, gp_out.rumble_r, false);
}