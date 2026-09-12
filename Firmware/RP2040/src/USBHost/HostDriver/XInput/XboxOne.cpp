#include <cstring>

#include "host/usbh.h"
#include "Board/board_api.h"
#include "TaskQueue/TaskQueue.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/XboxOne.h"
#include "USBHost/HostDriver/XInput/XboxArcadeStick.h"

/** While INPUT reports keep arriving, drop latched Guide if 0x07 went quiet. */
static constexpr uint32_t GUIDE_STALE_MS = 400u;
/**
 * #27: Series/One Guide is GIP VIRTUAL_KEY (0x07). Hold BUTTON_SYS while pressed; clear on
 * real release. Issue27Test forced an ~80 ms pulse which made taps OK but broke long-hold
 * (shutdown menu). Only orphan-clear after a long timeout if release never arrives.
 */
static constexpr uint32_t GUIDE_ORPHAN_MS = 5000u;

void XboxOneHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len)
{
    uint16_t vid = 0;
    uint16_t pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    gip_arcade_stick_ = XboxArcadeStick::is_xbox_one_gip(vid, pid);
    prev_arcade_len_ = 0;
    std::memset(&prev_in_report_, 0, sizeof(prev_in_report_));
    guide_pressed_ = 0;
    last_guide_07_ms_ = 0;
    cancel_guide_orphan();
    (void)gamepad;
    (void)report_desc;
    (void)desc_len;

    /* All GIP pads (first-party Series/One, PowerA, arcade) need start_xboxone:
     * IN arm + POWER_ON (+ S_INIT for non-arcade). Issue27Test2 only deferred this
     * for arcade sticks — Series X (045E:0B12) and PowerA got IN only: rumble/host
     * LED could change but Guide stayed dark and no input (#27 / #87). */
    const uint8_t addr = address;
    const uint8_t inst = instance;
    TaskQueue::Core1::queue_delayed_task(
        TaskQueue::Core1::get_new_task_id(), 50, false,
        [addr, inst]() { tuh_xinput::start_xboxone(addr, inst); });
}

static void map_gip_buttons(Gamepad& gamepad, const XboxOne::InReport* in_report, Gamepad::PadIn& gp_in,
    uint8_t guide_pressed)
{
    const uint16_t b = in_report->buttons;
    if (b & XboxOne::GipWireButtons::DPAD_UP)    gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (b & XboxOne::GipWireButtons::DPAD_DOWN)  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (b & XboxOne::GipWireButtons::DPAD_LEFT)  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (b & XboxOne::GipWireButtons::DPAD_RIGHT) gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;

    if (b & XboxOne::GipWireButtons::LEFT_THUMB)  gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (b & XboxOne::GipWireButtons::RIGHT_THUMB) gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (b & XboxOne::GipWireButtons::LEFT_SHOULDER)  gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (b & XboxOne::GipWireButtons::RIGHT_SHOULDER) gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (b & XboxOne::GipWireButtons::BACK)  gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (b & XboxOne::GipWireButtons::START) gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (b & XboxOne::GipWireButtons::SYNC)  gp_in.buttons |= gamepad.MAP_BUTTON_MISC;
    /* Some GIP reports set Guide in bit1 of the button word (WiredOpposite Buttons0::GUIDE). */
    if (b & XboxOne::Buttons0::GUIDE) gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (guide_pressed) gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (b & XboxOne::GipWireButtons::A)     gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (b & XboxOne::GipWireButtons::B)     gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (b & XboxOne::GipWireButtons::X)     gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (b & XboxOne::GipWireButtons::Y)     gp_in.buttons |= gamepad.MAP_BUTTON_Y;

    gp_in.trigger_l = gamepad.scale_trigger_l(static_cast<uint8_t>(in_report->trigger_l >> 2));
    gp_in.trigger_r = gamepad.scale_trigger_r(static_cast<uint8_t>(in_report->trigger_r >> 2));

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(in_report->joystick_lx, in_report->joystick_ly, true);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report->joystick_rx, in_report->joystick_ry, true);
}

void XboxOneHost::emit_pad_in(Gamepad& gamepad, uint8_t guide_pressed)
{
    Gamepad::PadIn gp_in;
    if (gip_arcade_stick_ && prev_arcade_len_ >= 23)
    {
        XboxArcadeStick::map_gip_arcade_report(prev_arcade_report_.data(), prev_arcade_len_, gamepad, gp_in);
        if (guide_pressed)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
        }
    }
    else
    {
        map_gip_buttons(gamepad, &prev_in_report_, gp_in, guide_pressed);
    }
    gamepad.set_pad_in(gp_in);
}

void XboxOneHost::cancel_guide_orphan()
{
    if (guide_orphan_task_id_ != 0)
    {
        TaskQueue::Core1::cancel_delayed_task(guide_orphan_task_id_);
        guide_orphan_task_id_ = 0;
    }
    ++guide_orphan_gen_;
}

void XboxOneHost::schedule_guide_orphan_clear(Gamepad& gamepad)
{
    cancel_guide_orphan();
    const uint8_t gen = guide_orphan_gen_;
    Gamepad* gp = &gamepad;
    guide_orphan_task_id_ = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(
        guide_orphan_task_id_, GUIDE_ORPHAN_MS, false,
        [this, gp, gen]() {
            guide_orphan_task_id_ = 0;
            if (gen != guide_orphan_gen_ || !guide_pressed_)
            {
                return;
            }
            guide_pressed_ = 0;
            emit_pad_in(*gp, 0);
        });
}

void XboxOneHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const uint8_t cmd = report[0];
    if (cmd == XboxOne::GIP_CMD_VIRTUAL_KEY)
    {
        last_guide_07_ms_ = board_api::ms_since_boot();
        uint8_t pressed = 0;
        if (len >= 5) {
            if (report[4] == 0x5B && len >= 6)
                pressed = (report[5] & 0x01) ? 1 : 0;
            else
                pressed = (report[4] & 0x01) ? 1 : 0;
        }
        guide_pressed_ = pressed;
        /* Always push PadIn on both edges so XInput sees press and release without waiting
         * for another 0x20 INPUT report (#27). */
        emit_pad_in(gamepad, guide_pressed_);
        if (pressed)
        {
            schedule_guide_orphan_clear(gamepad);
        }
        else
        {
            cancel_guide_orphan();
        }
        tuh_xinput::receive_report(address, instance);
        return;
    }

    if (cmd != XboxOne::GIP_CMD_INPUT)
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    if (gip_arcade_stick_)
    {
        if (guide_pressed_ && (board_api::ms_since_boot() - last_guide_07_ms_) > GUIDE_STALE_MS)
        {
            guide_pressed_ = 0;
            cancel_guide_orphan();
        }

        Gamepad::PadIn gp_in;
        XboxArcadeStick::map_gip_arcade_report(report, len, gamepad, gp_in);
        if (guide_pressed_)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
        }

        gamepad.set_pad_in(gp_in);
        tuh_xinput::receive_report(address, instance);

        const uint16_t copy_len = std::min<uint16_t>(len, static_cast<uint16_t>(prev_arcade_report_.size()));
        std::memcpy(prev_arcade_report_.data(), report, copy_len);
        prev_arcade_len_ = copy_len;
        return;
    }

    if (len < sizeof(XboxOne::InReport))
    {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    const XboxOne::InReport* in_report = reinterpret_cast<const XboxOne::InReport*>(report);
    if (guide_pressed_ && (board_api::ms_since_boot() - last_guide_07_ms_) > GUIDE_STALE_MS) {
        guide_pressed_ = 0;
        cancel_guide_orphan();
    }

    /* Always map + set_pad_in. Older memcmp used &prev_in_report_+4 (struct stride), not bytes. */
    Gamepad::PadIn gp_in;
    map_gip_buttons(gamepad, in_report, gp_in, guide_pressed_);
    gamepad.set_pad_in(gp_in);

    tuh_xinput::receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(XboxOne::InReport));
}

bool XboxOneHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    Gamepad::PadOut gp_out = gamepad.get_pad_out();
    return tuh_xinput::set_rumble(address, instance, gp_out.rumble_l, gp_out.rumble_r, false);
}
