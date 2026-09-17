#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/GameSirCyclone2/Cyclone2Ds4Wired.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <tuple>

#include "Board/ogxm_log.h"
#include "pico/time.h"

void Cyclone2Ds4Wired::reset() {
    ready_ = false;
    lightbar_sent_ = false;
    logged_banner_ = false;
    prev_raw_len_ = 0;
    std::memset(&out_, 0, sizeof(out_));
    std::memset(&prev_, 0, sizeof(prev_));
    std::memset(prev_raw_, 0, sizeof(prev_raw_));
}

void Cyclone2Ds4Wired::send_lightbar(uint8_t address, uint8_t instance) {
    out_.report_id = 0x05;
    out_.set_led = 1;
    out_.lightbar_blue = 0xFF / 2;
    out_.lightbar_red = 0;
    out_.lightbar_green = 0;

    /* Non-blocking: one try only — never spin tuh_task() from mount/init. */
    if (tuh_hid_send_report(address, instance, 0, reinterpret_cast<const uint8_t*>(&out_),
                            sizeof(PS4::OutReport))) {
        lightbar_sent_ = true;
        OGXM_LOG("[CYCLONE2 DS4] lightbar queued\n");
    } else {
        OGXM_LOG("[CYCLONE2 DS4] lightbar deferred (EP busy)\n");
    }
}

void Cyclone2Ds4Wired::start(uint8_t address, uint8_t instance, uint8_t player_idx) {
    reset();
    player_idx_ = player_idx;
    OGXM_LOG("\n[CYCLONE2 DS4]\n");
    OGXM_LOG("parser installed: YES\n");
    OGXM_LOG("DS4 setup: CALLED\n");
    OGXM_LOG("calibration request sent: NO (USB path uses lightbar wake only)\n");
    send_lightbar(address, instance);
    ready_ = true;
    OGXM_LOG("device READY: YES\n");
    OGXM_LOG("\n[CYCLONE2 PARSER]\nmode=DS4\nready=YES\n");
    OGXM_LOG("parse_input_report=Cyclone2Ds4Wired\n");
    logged_banner_ = true;
    tuh_hid_receive_report(address, instance);
}

void Cyclone2Ds4Wired::map_from_ps4_in(Gamepad& gamepad, const PS4::InReport& in) {
    Gamepad::PadIn gp{};

    switch (in.buttons[0] & PS4::DPAD_MASK) {
        case PS4::Buttons0::DPAD_UP: gp.dpad |= gamepad.MAP_DPAD_UP; break;
        case PS4::Buttons0::DPAD_DOWN: gp.dpad |= gamepad.MAP_DPAD_DOWN; break;
        case PS4::Buttons0::DPAD_LEFT: gp.dpad |= gamepad.MAP_DPAD_LEFT; break;
        case PS4::Buttons0::DPAD_RIGHT: gp.dpad |= gamepad.MAP_DPAD_RIGHT; break;
        case PS4::Buttons0::DPAD_UP_RIGHT: gp.dpad |= gamepad.MAP_DPAD_UP_RIGHT; break;
        case PS4::Buttons0::DPAD_RIGHT_DOWN: gp.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT; break;
        case PS4::Buttons0::DPAD_DOWN_LEFT: gp.dpad |= gamepad.MAP_DPAD_DOWN_LEFT; break;
        case PS4::Buttons0::DPAD_LEFT_UP: gp.dpad |= gamepad.MAP_DPAD_UP_LEFT; break;
        default: break;
    }

    if (in.buttons[0] & PS4::Buttons0::SQUARE) gp.buttons |= gamepad.MAP_BUTTON_X;
    if (in.buttons[0] & PS4::Buttons0::CROSS) gp.buttons |= gamepad.MAP_BUTTON_A;
    if (in.buttons[0] & PS4::Buttons0::CIRCLE) gp.buttons |= gamepad.MAP_BUTTON_B;
    if (in.buttons[0] & PS4::Buttons0::TRIANGLE) gp.buttons |= gamepad.MAP_BUTTON_Y;
    if (in.buttons[1] & PS4::Buttons1::L1) gp.buttons |= gamepad.MAP_BUTTON_LB;
    if (in.buttons[1] & PS4::Buttons1::R1) gp.buttons |= gamepad.MAP_BUTTON_RB;
    if (in.buttons[1] & PS4::Buttons1::L3) gp.buttons |= gamepad.MAP_BUTTON_L3;
    if (in.buttons[1] & PS4::Buttons1::R3) gp.buttons |= gamepad.MAP_BUTTON_R3;
    if (in.buttons[1] & PS4::Buttons1::SHARE) gp.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in.buttons[1] & PS4::Buttons1::OPTIONS) gp.buttons |= gamepad.MAP_BUTTON_START;
    if (in.buttons[2] & PS4::Buttons2::PS) gp.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in.buttons[2] & PS4::Buttons2::TP) gp.buttons |= gamepad.MAP_BUTTON_MISC;

    gp.trigger_l = gamepad.scale_trigger_l(in.trigger_l);
    gp.trigger_r = gamepad.scale_trigger_r(in.trigger_r);
    std::tie(gp.joystick_lx, gp.joystick_ly) = gamepad.scale_joystick_l(in.joystick_lx, in.joystick_ly);
    std::tie(gp.joystick_rx, gp.joystick_ry) = gamepad.scale_joystick_r(in.joystick_rx, in.joystick_ry);

    gamepad.set_pad_in(gp);
}

void Cyclone2Ds4Wired::log_pipeline(Gamepad& gamepad, const uint8_t* report, uint16_t len, uint8_t rid) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if ((now - last_stage_log_ms_) < 80) {
        return;
    }
    last_stage_log_ms_ = now;
    const Gamepad::PadIn gp = gamepad.get_pad_in();
    OGXM_LOG("\n[1 RAW] id=0x%02X len=%u\n", rid, static_cast<unsigned>(len));
    OGXM_LOG_HEX(report, len > 24 ? 24 : len);
    OGXM_LOG("[2 MODE PARSER ENTERED] Cyclone2Ds4Wired ready=%s\n", ready_ ? "YES" : "NO");
    OGXM_LOG("[3 DECODED CONTROLLER STATE] klass=GAMEPAD LX=%d LY=%d RX=%d RY=%d L2=%u R2=%u "
             "buttons=0x%04X misc/dpad=0x%02X\n",
             static_cast<int>(gp.joystick_lx), static_cast<int>(gp.joystick_ly),
             static_cast<int>(gp.joystick_rx), static_cast<int>(gp.joystick_ry),
             static_cast<unsigned>(gp.trigger_l), static_cast<unsigned>(gp.trigger_r),
             gp.buttons, gp.dpad);
    OGXM_LOG("[4 BLUEPAD CONTROLLER CALLBACK] N/A (wired USB host path)\n");
    OGXM_LOG("[5 OGX INPUT STATE] buttons=0x%04X dpad=0x%02X\n", gp.buttons, gp.dpad);
    OGXM_LOG("[6 XINPUT OUTPUT STATE] pending via DeviceDriver::process from PadIn\n");
}

bool Cyclone2Ds4Wired::decode_report(Gamepad& gamepad, const uint8_t* report, uint16_t len) {
    if (!report || len < 9) {
        return false;
    }

    PS4::InReport in{};
    uint8_t rid = 0;

    /*
     * USB DS4 variants:
     *  - [0]=0x01 then sticks/buttons (PS4::InReport; often padded to 64)
     *  - report ID stripped: sticks start at [0]
     *  - BT-style 0x11 with 2-byte header before gamepad payload
     */
    if (report[0] == 0x01 && len >= sizeof(PS4::InReport)) {
        rid = 0x01;
        std::memcpy(&in, report, sizeof(PS4::InReport));
    } else if (report[0] == 0x11 && len >= 3 + (sizeof(PS4::InReport) - 1)) {
        rid = 0x11;
        in.report_id = 0x01;
        std::memcpy(&in.joystick_lx, report + 3, sizeof(PS4::InReport) - 1);
    } else if (len >= sizeof(PS4::InReport) - 1 && report[0] != 0x05) {
        rid = 0x00;
        in.report_id = 0x01;
        std::memcpy(&in.joystick_lx, report, sizeof(PS4::InReport) - 1);
    } else {
        OGXM_LOG("[CYCLONE2 DS4] Unexpected report type and len: id=0x%02X len=%u\n",
                 report[0], static_cast<unsigned>(len));
        return false;
    }

    in.buttons[2] = static_cast<uint8_t>(in.buttons[2] & ~PS4::COUNTER_MASK);

    if (std::memcmp(&in, &prev_, sizeof(PS4::InReport)) == 0) {
        return false;
    }

    map_from_ps4_in(gamepad, in);
    std::memcpy(&prev_, &in, sizeof(prev_));
    log_pipeline(gamepad, report, len, rid);
    return true;
}

void Cyclone2Ds4Wired::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                       const uint8_t* report, uint16_t len) {
    if (!report || len == 0) {
        tuh_hid_receive_report(address, instance);
        return;
    }

    const uint16_t n = (len < sizeof(prev_raw_)) ? len : static_cast<uint16_t>(sizeof(prev_raw_));
    const bool changed = (n != prev_raw_len_) || (std::memcmp(prev_raw_, report, n) != 0);
    if (changed) {
        prev_raw_len_ = n;
        std::memcpy(prev_raw_, report, n);
        OGXM_LOG("\n[CYCLONE2 DS4 RAW]\nreport_id: 0x%02X\nlength: %u\nDATA:\n",
                 report[0], static_cast<unsigned>(len));
        OGXM_LOG_HEX(report, n > 32 ? 32 : n);
    }

    (void)decode_report(gamepad, report, len);
    tuh_hid_receive_report(address, instance);
}

bool Cyclone2Ds4Wired::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) {
    if (!lightbar_sent_) {
        send_lightbar(address, instance);
    }
    const Gamepad::PadOut gp_out = gamepad.get_pad_out();
    out_.report_id = 0x05;
    out_.motor_left = gp_out.rumble_l;
    out_.motor_right = gp_out.rumble_r;
    out_.set_rumble = (out_.motor_left != 0 || out_.motor_right != 0) ? 1 : 0;
    out_.set_led = 1;
    if (!tuh_hid_send_ready(address, instance)) {
        return false;
    }
    return tuh_hid_send_report(address, instance, 0, reinterpret_cast<const uint8_t*>(&out_),
                               sizeof(PS4::OutReport));
}
