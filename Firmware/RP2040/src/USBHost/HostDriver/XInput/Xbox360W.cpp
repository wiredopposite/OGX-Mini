#include <cstring>
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include <hardware/irq.h>

#include "host/usbh.h"

#include "TaskQueue/TaskQueue.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/Xbox360W.h"
#include "Board/ogxm_log.h"
#include "Descriptors/XInput.h"

Xbox360WHost::~Xbox360WHost()
{
    TaskQueue::Core1::cancel_delayed_task(tid_chatpad_keepalive_);
}

void Xbox360WHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    (void)gamepad;
    (void)address;
    (void)instance;
    (void)report_desc;
    (void)desc_len;
    /* IN polling is started from tuh_xinput set_config for every wireless port. */
}

void Xbox360WHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    if (report[1] & 2)
    {
        if (len >= sizeof(XInput::InReportWireless))
        {
            const XInput::InReportWireless* in_report = reinterpret_cast<const XInput::InReportWireless*>(report);
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
        }

        tuh_xinput::receive_report(address, instance);
        return;
    }

    if (!(report[1] & 1) || len < 18 ||
        (report[5] != 0x13 && report[5] != 0x14))
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    if (std::memcmp(prev_in_report_, report, std::min(static_cast<size_t>(len), sizeof(prev_in_report_))) == 0)
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    const uint16_t wButtons = static_cast<uint16_t>(report[6]) |
                              (static_cast<uint16_t>(report[7]) << 8);

    Gamepad::PadIn gp_in;

    if (wButtons & (1U << 0))  gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (wButtons & (1U << 1))  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (wButtons & (1U << 2))  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (wButtons & (1U << 3))  gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
    if (wButtons & (1U << 4))  gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (wButtons & (1U << 5))  gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (wButtons & (1U << 6))  gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (wButtons & (1U << 7))  gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (wButtons & (1U << 8))  gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (wButtons & (1U << 9))  gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (wButtons & (1U << 10)) gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (wButtons & (1U << 12)) gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (wButtons & (1U << 13)) gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (wButtons & (1U << 14)) gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (wButtons & (1U << 15)) gp_in.buttons |= gamepad.MAP_BUTTON_Y;

    gp_in.trigger_l = gamepad.scale_trigger_l(report[8]);
    gp_in.trigger_r = gamepad.scale_trigger_r(report[9]);

    const int16_t joy_lx = static_cast<int16_t>(static_cast<uint16_t>(report[10]) |
                         (static_cast<uint16_t>(report[11]) << 8));
    const int16_t joy_ly = static_cast<int16_t>(static_cast<uint16_t>(report[12]) |
                         (static_cast<uint16_t>(report[13]) << 8));
    const int16_t joy_rx = static_cast<int16_t>(static_cast<uint16_t>(report[14]) |
                         (static_cast<uint16_t>(report[15]) << 8));
    const int16_t joy_ry = static_cast<int16_t>(static_cast<uint16_t>(report[16]) |
                         (static_cast<uint16_t>(report[17]) << 8));

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(joy_lx, joy_ly, true);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(joy_rx, joy_ry, true);

    gamepad.set_pad_in(gp_in);

    std::memcpy(prev_in_report_, report, std::min(static_cast<size_t>(len), sizeof(prev_in_report_)));
    tuh_xinput::receive_report(address, instance);
}

bool Xbox360WHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    const uint8_t rumble_instance =
        (active_address_ == address && active_instance_ != 0xFF) ? active_instance_ : instance;
    if (!tuh_xinput::is_connected(address, rumble_instance))
    {
        return false;
    }
    Gamepad::PadOut gp_out = gamepad.get_pad_out();
    if (gp_out.rumble_l == 0 && gp_out.rumble_r == 0)
    {
        return false;
    }
    return tuh_xinput::set_rumble(address, rumble_instance, gp_out.rumble_l, gp_out.rumble_r, false);
}

void Xbox360WHost::connect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;
    active_address_ = address;
    active_instance_ = instance;
}

void Xbox360WHost::disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;
    (void)address;
    (void)instance;
    TaskQueue::Core1::cancel_delayed_task(tid_chatpad_keepalive_);
    std::memset(prev_in_report_, 0, sizeof(prev_in_report_));
    active_instance_ = 0xFF;
    active_address_ = 0xFF;
}
