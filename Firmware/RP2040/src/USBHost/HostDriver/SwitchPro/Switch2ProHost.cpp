#include <cstring>
#include <tuple>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/SwitchPro/Switch2ProHost.h"

#include "Board/ogxm_log.h"
#include "Gamepad/MotionImu.h"

namespace {

/** Same 10-byte payload resolution as SwitchProHost (report id 0x09 / 0x30 / 0x31 + timer, or raw 10). */
const uint8_t* switch2_payload(const uint8_t* report, uint16_t len, uint16_t& out_payload_len)
{
    out_payload_len = len;
    if (len == sizeof(SwitchPro::InReport))
    {
        return report;
    }
    if (len >= 2U + sizeof(SwitchPro::InReport))
    {
        const uint8_t rid = report[0];
        if (rid == SwitchPro::REPORT_ID_STANDARD || rid == SwitchPro::REPORT_ID_SWITCH2_FULL ||
            rid == SwitchPro::REPORT_ID_FULL_ALT)
        {
            out_payload_len = static_cast<uint16_t>(len - 2U);
            return report + 2;
        }
    }
    return nullptr;
}

} // namespace

void Switch2ProHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report,
                                    uint16_t len)
{
    if (switch2_bringup_active_ || init_state_ != InitState::DONE)
    {
        SwitchProHost::process_report(gamepad, address, instance, report, len);
        return;
    }

#ifdef OGXM_SWITCH2_HID_RAW_LOG
    if (len > 0)
    {
        const std::uint16_t n = len < kSwitch2RawLogCap ? len : kSwitch2RawLogCap;
        // Log only when the three digital button bytes change. Timer (byte1), sticks (6–11), and IMU (12+)
        // all move nearly every packet and will flood UART if included in the compare.
        std::uint8_t btn[3]{};
        bool have_btn = false;
        if (len >= 2U + sizeof(SwitchPro::InReport))
        {
            const std::uint8_t rid = report[0];
            if ((rid == SwitchPro::REPORT_ID_STANDARD || rid == SwitchPro::REPORT_ID_SWITCH2_FULL ||
                 rid == SwitchPro::REPORT_ID_FULL_ALT) &&
                len >= 6U)
            {
                btn[0] = report[3];
                btn[1] = report[4];
                btn[2] = report[5];
                have_btn = true;
            }
        }
        else if (len == sizeof(SwitchPro::InReport))
        {
            btn[0] = report[1];
            btn[1] = report[2];
            btn[2] = report[3];
            have_btn = true;
        }
        const bool btn_changed =
            have_btn &&
            (!switch2_dbg_have_btn_ || std::memcmp(btn, switch2_dbg_prev_btn_, sizeof(btn)) != 0);
        const bool len_changed = (switch2_dbg_prev_len_ != n);
        if (have_btn && (btn_changed || len_changed))
        {
            OGXM_LOG("S2Pro HID len=%u (3 digital btn bytes changed; full dump follows)\n", static_cast<unsigned>(len));
            OGXM_LOG_HEX(report, n);
            std::memcpy(switch2_dbg_prev_, report, n);
            switch2_dbg_prev_len_ = n;
            std::memcpy(switch2_dbg_prev_btn_, btn, sizeof(btn));
            switch2_dbg_have_btn_ = true;
        }
    }
#endif

    uint16_t payload_len = 0;
    const uint8_t* payload = switch2_payload(report, len, payload_len);
    if (payload == nullptr || payload_len < sizeof(SwitchPro::InReport))
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    const SwitchPro::InReport* in_report = reinterpret_cast<const SwitchPro::InReport*>(payload);
    if (std::memcmp(&prev_in_report_.buttons, in_report->buttons, 9) == 0 &&
        MotionImu::switch_usb_imu_unchanged(prev_imu_, payload, payload_len))
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp_in{};

    // Switch 2 Pro (USB 0x2069): same 10-byte InReport offsets as the original Pro, but the three
    // "button" bytes do not follow classic Buttons0/1/2 semantics. Layout from maintainer capture.
    const uint8_t br = in_report->buttons[0];
    const uint8_t bm = in_report->buttons[1];
    const uint8_t bl = in_report->buttons[2];

    // Face: hardware order is B,A,Y,X on bits 0–3 (classic Pro uses Y,X,B,A on the same bits).
    if (br & (1U << 2))
        gp_in.buttons |= gamepad.MAP_BUTTON_X; // west → Xbox X
    if (br & (1U << 0))
        gp_in.buttons |= gamepad.MAP_BUTTON_A; // south → Xbox A
    if (br & (1U << 1))
        gp_in.buttons |= gamepad.MAP_BUTTON_B; // east → Xbox B
    if (br & (1U << 3))
        gp_in.buttons |= gamepad.MAP_BUTTON_Y; // north → Xbox Y

    // Bit 4 in first byte: hardware RB (capture: pressed 0x10, released 0x00 in bytes [3][4][5]).
    if (br & (1U << 4))
        gp_in.buttons |= gamepad.MAP_BUTTON_RB;

    // Right byte: classic R is Start; classic ZR bit is stick L3 on Switch 2 Pro (not R3).
    if (br & SwitchPro::Buttons0::R)
        gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (br & SwitchPro::Buttons0::ZR)
        gp_in.buttons |= gamepad.MAP_BUTTON_L3;

    // Bit 5 in misc byte: hardware ZL digital (capture: 00 20 00 vs released; classic CAPTURE bit).
    // Bit 5 in first byte: hardware ZR / RT digital (capture: 20 00 00 vs released).
    // Analog travel may exist elsewhere in the 64-byte report; these are full-digital paths.
    const bool zl_on = (bm & (1U << 5)) != 0;
    const bool zr_on = (br & (1U << 5)) != 0;

    // Bit 6 in misc byte: hardware Minus (−) → Back/Select (capture: 00 40 00 vs released 00 00 00).
    if (bm & (1U << 6))
        gp_in.buttons |= gamepad.MAP_BUTTON_BACK;

    // Bit 7 in misc byte: hardware R3 (was mapped to L3; swapped to match physical sticks).
    if (bm & (1U << 7))
        gp_in.buttons |= gamepad.MAP_BUTTON_R3;

    // "Misc" byte: d-pad uses some classic − / + / L3 / R3 bit positions; L uses the Home bit.
    // L3 here is d-pad left (capture: 00 04 00); stick R3 is bm bit 7 above.
    if (bm & SwitchPro::Buttons1::MINUS)
        gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (bm & SwitchPro::Buttons1::L3)
        gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (bm & SwitchPro::Buttons1::PLUS)
        gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
    if (bm & SwitchPro::Buttons1::R3)
        gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (bm & SwitchPro::Buttons1::HOME)
        gp_in.buttons |= gamepad.MAP_BUTTON_LB;

    // "Left" byte: Home on classic d-pad-up bit; Capture shares classic d-pad-down (left unmapped).
    //
    // Reference only — GL / GR / Chat (intentionally unmapped), all in bl: GL bit 3 (00 00 08);
    // GR bit 2 (00 00 04); Chat bit 4 (00 00 10); released 00 00 00.
    if (bl & SwitchPro::Buttons2::DPAD_UP)
        gp_in.buttons |= gamepad.MAP_BUTTON_SYS;

    gp_in.trigger_l = zl_on ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
    gp_in.trigger_r = zr_on ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;

    uint16_t joy_lx = in_report->joysticks[0] | ((in_report->joysticks[1] & 0xF) << 8);
    uint16_t joy_ly = (in_report->joysticks[1] >> 4) | (in_report->joysticks[2] << 4);
    uint16_t joy_rx = in_report->joysticks[3] | ((in_report->joysticks[4] & 0xF) << 8);
    uint16_t joy_ry = (in_report->joysticks[4] >> 4) | (in_report->joysticks[5] << 4);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) =
        gamepad.scale_joystick_l(normalize_axis(joy_lx), normalize_axis(joy_ly), true);

    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) =
        gamepad.scale_joystick_r(normalize_axis(joy_rx), normalize_axis(joy_ry), true);

    if (payload_len >= 22) {
        gp_in.motion_source = Gamepad::PadIn::MOTION_SRC_SWITCH_USB;
        MotionImu::fill_from_switch_usb_payload(gp_in.accel, gp_in.gyro, payload, payload_len);
    }

    gamepad.set_pad_in(gp_in);
    std::memcpy(&prev_in_report_, in_report, sizeof(SwitchPro::InReport));
    if (payload_len >= 22) {
        constexpr uint16_t kInReportSize = 10;
        constexpr uint16_t kImuSampleSize = sizeof(MotionImu::SwitchImuSample);
        uint16_t imu_off = kInReportSize;
        if (payload_len >= kInReportSize + (3 * kImuSampleSize)) {
            imu_off = static_cast<uint16_t>(kInReportSize + (2 * kImuSampleSize));
        }
        std::memcpy(prev_imu_, payload + imu_off, sizeof(prev_imu_));
    }

    tuh_hid_receive_report(address, instance);
}
