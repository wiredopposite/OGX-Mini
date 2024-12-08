#include <cstring>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

#include "host/usbh.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/Xbox360W.h"

Xbox360WHost::~Xbox360WHost()
{
    cancel_repeating_timer(&timer_info_.timer);
}

void Xbox360WHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    std::memset(&prev_in_report_, 0, sizeof(XInput::InReportWireless));

    timer_info_.address = address;
    timer_info_.instance = instance;
    timer_info_.led_quadrant = idx_ + 1;

    add_repeating_timer_ms(1000, timer_cb, &timer_info_, &timer_info_.timer);
    tuh_xinput::receive_report(address, instance);
}

bool Xbox360WHost::timer_cb(struct repeating_timer *t)
{
    TimerInfo *info = static_cast<TimerInfo *>(t->user_data);
    tuh_xinput::set_led(info->address, info->instance, info->led_quadrant, false);
    return true;
}

void Xbox360WHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const XInput::InReportWireless* in_report = reinterpret_cast<const XInput::InReportWireless*>(report);
    if (std::memcmp(&prev_in_report_, in_report, std::min(static_cast<size_t>(len), sizeof(XInput::InReportWireless))) == 0)
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    if (in_report->buttons[0] & XInput::Buttons0::DPAD_UP)    gamepad.set_dpad_up();
    if (in_report->buttons[0] & XInput::Buttons0::DPAD_DOWN)  gamepad.set_dpad_down();
    if (in_report->buttons[0] & XInput::Buttons0::DPAD_LEFT)  gamepad.set_dpad_left();
    if (in_report->buttons[0] & XInput::Buttons0::DPAD_RIGHT) gamepad.set_dpad_right();

    if (in_report->buttons[0] & XInput::Buttons0::START)  gamepad.set_button_start();
    if (in_report->buttons[0] & XInput::Buttons0::BACK)   gamepad.set_button_back();
    if (in_report->buttons[0] & XInput::Buttons0::L3)     gamepad.set_button_l3();
    if (in_report->buttons[0] & XInput::Buttons0::R3)     gamepad.set_button_r3();
    if (in_report->buttons[1] & XInput::Buttons1::LB)     gamepad.set_button_lb();
    if (in_report->buttons[1] & XInput::Buttons1::RB)     gamepad.set_button_rb();
    if (in_report->buttons[1] & XInput::Buttons1::HOME)   gamepad.set_button_sys();
    if (in_report->buttons[1] & XInput::Buttons1::A)      gamepad.set_button_a();
    if (in_report->buttons[1] & XInput::Buttons1::B)      gamepad.set_button_b();
    if (in_report->buttons[1] & XInput::Buttons1::X)      gamepad.set_button_x();
    if (in_report->buttons[1] & XInput::Buttons1::Y)      gamepad.set_button_y();

    gamepad.set_trigger_l(in_report->trigger_l);
    gamepad.set_trigger_r(in_report->trigger_r);

    gamepad.set_joystick_lx(in_report->joystick_lx);
    gamepad.set_joystick_ly(in_report->joystick_ly, true);
    gamepad.set_joystick_rx(in_report->joystick_rx);
    gamepad.set_joystick_ry(in_report->joystick_ry, true);

    Gamepad::Chatpad gp_chatpad;

    if ((in_report->command[1] & 2) && (in_report->chatpad_status == 0x00))
    {
        gp_chatpad =
        {
            in_report->chatpad[0],
            in_report->chatpad[1],
            in_report->chatpad[2],
        };
    }
    else
    {
        gp_chatpad.fill(0);
    }

    gamepad.set_chatpad(gp_chatpad);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(XInput::InReportWireless));
}

bool Xbox360WHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    return tuh_xinput::set_rumble(address, instance, gamepad.get_rumble_l().uint8(), gamepad.get_rumble_r().uint8(), false);
}