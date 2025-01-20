#include <cstring>
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include <hardware/irq.h>

#include "host/usbh.h"

#include "TaskQueue/TaskQueue.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/Xbox360W.h"
#include "Board/ogxm_log.h"

Xbox360WHost::~Xbox360WHost()
{
    TaskQueue::Core1::cancel_delayed_task(tid_chatpad_keepalive_);
}

void Xbox360WHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    tuh_xinput::receive_report(address, instance);
}

void Xbox360WHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const XInput::InReportWireless* in_report = reinterpret_cast<const XInput::InReportWireless*>(report);

    if (in_report->command[1] & 2)
    {
        if (in_report->chatpad_status == 0x00)
        {
            Gamepad::ChatpadIn gp_in_chatpad;
            gp_in_chatpad[0] = in_report->chatpad[0];
            gp_in_chatpad[1] = in_report->chatpad[1];
            gp_in_chatpad[2] = in_report->chatpad[2];

            gamepad.set_chatpad_in(gp_in_chatpad);
        }
        else if (in_report->chatpad_status == 0xF0 && in_report->chatpad[0] == 0x03)
        {
            tuh_xinput::xbox360_chatpad_init(address, instance);
        }

        tuh_xinput::receive_report(address, instance);
        return;
    }

    if (!(in_report->command[1] & 1) ||
        !(in_report->report_size == 0x13) ||
        std::memcmp(&prev_in_report_, in_report, std::min(static_cast<size_t>(len), sizeof(XInput::InReportWireless))) == 0)
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

    gp_in.trigger_l = gamepad.scale_trigger_l(in_report->trigger_l);
    gp_in.trigger_r = gamepad.scale_trigger_r(in_report->trigger_r);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(in_report->joystick_lx, in_report->joystick_ly, true);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report->joystick_rx, in_report->joystick_ry, true);

    gamepad.set_pad_in(gp_in);

    tuh_xinput::receive_report(address, instance);
    prev_in_report_ = *in_report;
}

bool Xbox360WHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    Gamepad::PadOut gp_out = gamepad.get_pad_out(); 
    return tuh_xinput::set_rumble(address, instance, gp_out.rumble_l, gp_out.rumble_r, false);
}

void Xbox360WHost::connect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    tid_chatpad_keepalive_ = TaskQueue::Core1::get_new_task_id();

    //Might not be ready for leds yet, needs delay
    TaskQueue::Core1::queue_delayed_task(TaskQueue::Core1::get_new_task_id(), 1000, false, 
    [address, instance, this]
    {
        tuh_xinput::set_led(address, instance, idx_ + 1, false);
        tuh_xinput::xbox360_chatpad_init(address, instance);
        
        TaskQueue::Core1::queue_delayed_task(tid_chatpad_keepalive_, tuh_xinput::KEEPALIVE_MS, true, 
        [address, instance]
        {
            OGXM_LOG("XInput Chatpad Keepalive\r\n");
            tuh_xinput::xbox360_chatpad_keepalive(address, instance);
        });
    });
}

void Xbox360WHost::disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    TaskQueue::Core1::cancel_delayed_task(tid_chatpad_keepalive_);
}