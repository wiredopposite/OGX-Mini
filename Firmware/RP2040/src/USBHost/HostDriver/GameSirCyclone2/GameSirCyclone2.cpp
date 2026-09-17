#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Trace.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Transport.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <tuple>

#include "Board/ogxm_log.h"
#include "TaskQueue/TaskQueue.h"
#include "USBHost/HostDriver/XInput/XboxArcadeStick.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "class/hid/hid_host.h"
#include "host/usbh.h"
#include "pico/time.h"

namespace {

constexpr uint16_t kGameSirVid = 0x3537;
constexpr uint16_t kNintendoVid = 0x057E;
constexpr uint16_t kCyclone2XinputPids[] = {0x100B, 0x1053};
constexpr uint8_t kVendorReportIdCmd = 0x0F;
constexpr uint8_t kVendorCmdHeartbeat = 0xF2;
constexpr uint8_t kVendorReportIdEnhanced = 0x12;
constexpr uint32_t kHeartbeatPeriodMs = 1000;
constexpr uint32_t kUnchangedLogPeriodMs = 2000;

bool is_known_cyclone_xinput_pid(uint16_t pid) {
    for (uint16_t known : kCyclone2XinputPids) {
        if (pid == known) {
            return true;
        }
    }
    return false;
}

const char* endpoint_type_name(uint8_t attr) {
    switch (attr & 0x03u) {
        case 0: return "CTRL";
        case 1: return "ISO";
        case 2: return "BULK";
        case 3: return "INT";
        default: return "?";
    }
}

void utf16_to_ascii(const uint8_t* buf, uint16_t buflen, char* out, size_t out_len) {
    if (!out || out_len == 0) return;
    out[0] = '\0';
    if (!buf || buflen < 2) return;
    const uint8_t str_len = buf[0];
    if (str_len < 2 || buf[1] != 0x03) return;
    size_t o = 0;
    for (uint8_t i = 2; i + 1 < str_len && i + 1 < buflen && o + 1 < out_len; i += 2) {
        const uint8_t lo = buf[i];
        const uint8_t hi = buf[i + 1];
        out[o++] = (hi == 0 && lo >= 0x20 && lo < 0x7F) ? static_cast<char>(lo) : '?';
    }
    out[o] = '\0';
}

}  // namespace

bool GameSirCyclone2Host::is_known_id(uint16_t vid, uint16_t pid) {
    return vid == kGameSirVid && is_known_cyclone_xinput_pid(pid);
}

bool GameSirCyclone2Host::is_hid_passive_id(uint16_t vid, uint16_t pid) {
    return vid == kGameSirVid && pid == 0x0575;
}

GameSirCyclone2Host::Personality GameSirCyclone2Host::infer_personality(uint16_t vid, uint16_t pid) {
    if (is_known_id(vid, pid)) {
        return Personality::XInput;
    }
    if (vid == kNintendoVid && pid == 0x2009) {
        return Personality::SwitchNs;
    }
    if (vid == 0x054C && pid == 0x09CC) {
        return Personality::Ds4;
    }
    if (is_hid_passive_id(vid, pid)) {
        return Personality::HidPassive;
    }
    return Personality::Unknown;
}

bool GameSirCyclone2Host::should_claim(uint16_t vid, uint16_t pid) {
    if (is_known_id(vid, pid) || is_hid_passive_id(vid, pid)) {
        return true;
    }
    if (GameSirCyclone2Trace::should_own_switch_ns(vid, pid)) {
        return true;
    }
    if (GameSirCyclone2Trace::should_own_hid_passive(vid, pid)) {
        return true;
    }
    return GameSirCyclone2Trace::should_own_ds4(vid, pid);
}

bool GameSirCyclone2Host::should_claim(uint8_t address, uint16_t vid, uint16_t pid) {
    if (should_claim(vid, pid)) {
        return true;
    }
    return GameSirCyclone2Trace::looks_like_cyclone_switch_ns(address, vid, pid);
}

void GameSirCyclone2Host::log_mode_status_banner(uint16_t vid, uint16_t pid) {
    OGXM_LOG("\n================================================\n");
    OGXM_LOG("GAMESIR CYCLONE 2\n");
    OGXM_LOG("================================================\n");
    OGXM_LOG("Connection:\nWIRED\n");
    OGXM_LOG("Mode:\n%s\n",
             personality_ == Personality::XInput ? "XINPUT" :
             personality_ == Personality::SwitchNs ? "SWITCH" :
             personality_ == Personality::Ds4 ? "DS4" :
             personality_ == Personality::HidPassive ? "HID_PASSIVE" : "UNKNOWN");
    OGXM_LOG("VID:\n%04X\n", vid);
    OGXM_LOG("PID:\n%04X\n", pid);
    OGXM_LOG("Driver:\nGAMESIR CYCLONE 2\n");
    OGXM_LOG("Parser:\n%s\n",
             personality_ == Personality::XInput ? "WIRED_XINPUT" :
             personality_ == Personality::SwitchNs ? "WIRED_SWITCH_NS" :
             personality_ == Personality::Ds4 ? "WIRED_DS4" :
             personality_ == Personality::HidPassive ? "PASSIVE_PROBE" : "?");
    if (personality_ == Personality::SwitchNs) {
        OGXM_LOG("Standard controls:\nTESTING\n");
        OGXM_LOG("L4:\nUNKNOWN (not expected via Switch report / 0x12)\n");
        OGXM_LOG("R4:\nUNKNOWN (not expected via Switch report / 0x12)\n");
        OGXM_LOG("M:\nUNKNOWN (not expected via Switch report / 0x12)\n");
        OGXM_LOG("Home:\nTESTING\n");
    } else if (personality_ == Personality::Ds4) {
        OGXM_LOG("Standard controls:\nTESTING\n");
        OGXM_LOG("L4:\nN/A (vendor 0x12 not in DS4 mode)\n");
        OGXM_LOG("R4:\nN/A (vendor 0x12 not in DS4 mode)\n");
        OGXM_LOG("M:\nN/A (vendor 0x12 not in DS4 mode)\n");
        OGXM_LOG("PS/Home:\nTESTING\n");
    } else if (personality_ == Personality::HidPassive) {
        OGXM_LOG("Standard controls:\nNOT MAPPED (passive only)\n");
        OGXM_LOG("OUT traffic:\nBLOCKED\n");
    }
    OGXM_LOG("================================================\n");
}

void GameSirCyclone2Host::dump_hid_report_descriptor(const uint8_t* report_desc, uint16_t desc_len) {
    OGXM_LOG("[CYCLONE2] HID report descriptor length: %u\n", static_cast<unsigned>(desc_len));
    if (!report_desc || desc_len == 0) {
        OGXM_LOG("[CYCLONE2] HID report descriptor unavailable on this mount\n");
        return;
    }
    OGXM_LOG_HEX(report_desc, desc_len);
}

void GameSirCyclone2Host::dump_usb_tree(uint8_t address) {
    uint8_t cfg[512]{};
    const uint8_t cfg_rc = tuh_descriptor_get_configuration_sync(address, 0, cfg, sizeof(cfg));
    if (cfg_rc != XFER_RESULT_SUCCESS) {
        OGXM_LOG("[CYCLONE2] configuration descriptor fetch failed rc=%u\n", static_cast<unsigned>(cfg_rc));
        return;
    }

    const auto* conf = reinterpret_cast<const tusb_desc_configuration_t*>(cfg);
    const uint16_t total = tu_le16toh(conf->wTotalLength);
    const uint16_t len = TU_MIN(total, static_cast<uint16_t>(sizeof(cfg)));

    OGXM_LOG("Configuration count: 1\n");
    OGXM_LOG("Interface count: %u\n", conf->bNumInterfaces);

    const uint8_t* p = tu_desc_next(cfg);
    const uint8_t* end = cfg + len;
    while (p < end && tu_desc_type(p) != 0) {
        const uint8_t dlen = tu_desc_len(p);
        if (dlen < 2 || p + dlen > end) break;

        if (tu_desc_type(p) == TUSB_DESC_INTERFACE) {
            const auto* itf = reinterpret_cast<const tusb_desc_interface_t*>(p);
            OGXM_LOG("\nInterface %u:\n", itf->bInterfaceNumber);
            OGXM_LOG("  class: 0x%02X\n", itf->bInterfaceClass);
            OGXM_LOG("  subclass: 0x%02X\n", itf->bInterfaceSubClass);
            OGXM_LOG("  protocol: 0x%02X\n", itf->bInterfaceProtocol);
        } else if (tu_desc_type(p) == TUSB_DESC_ENDPOINT) {
            const auto* ep = reinterpret_cast<const tusb_desc_endpoint_t*>(p);
            const uint8_t addr = ep->bEndpointAddress;
            OGXM_LOG("  endpoint: 0x%02X\n", addr);
            OGXM_LOG("  direction: %s\n", (addr & TUSB_DIR_IN_MASK) ? "IN" : "OUT");
            OGXM_LOG("  transfer type: %s\n", endpoint_type_name(static_cast<uint8_t>(ep->bmAttributes.xfer)));
            OGXM_LOG("  max packet size: %u\n", static_cast<unsigned>(tu_edpt_packet_size(ep)));
            OGXM_LOG("  interval: %u\n", static_cast<unsigned>(ep->bInterval));
        }
        p = tu_desc_next(p);
    }
}

void GameSirCyclone2Host::classify_and_log_interface(uint8_t address, uint8_t instance,
                                                      const uint8_t* report_desc, uint16_t desc_len) {
    (void)report_desc;
    (void)desc_len;
    if (is_hid_path_) {
        tuh_itf_info_t info{};
        if (tuh_hid_itf_get_info(address, instance, &info)) {
            itf_num_ = info.desc.bInterfaceNumber;
            OGXM_LOG("[CYCLONE2] HID interface=%u class=0x%02X subclass=0x%02X proto=0x%02X\n",
                     static_cast<unsigned>(itf_num_), info.desc.bInterfaceClass,
                     info.desc.bInterfaceSubClass, info.desc.bInterfaceProtocol);
            is_boot_kb_mouse_ = (info.desc.bInterfaceProtocol == HID_ITF_PROTOCOL_KEYBOARD ||
                                 info.desc.bInterfaceProtocol == HID_ITF_PROTOCOL_MOUSE);
        }
    } else {
        OGXM_LOG("[CYCLONE2] XInput interface mounted (instance=%u)\n", static_cast<unsigned>(instance));
    }
}

bool GameSirCyclone2Host::is_vendor_hid_descriptor(const uint8_t* report_desc, uint16_t desc_len) {
    if (!report_desc || desc_len < 4) return false;
    bool usage_fff0 = false;
    report_id_0f_present_ = false;
    report_id_12_present_ = false;
    for (uint16_t i = 0; i + 1 < desc_len; ++i) {
        if (i + 2 < desc_len && report_desc[i] == 0x06 && report_desc[i + 1] == 0xF0 && report_desc[i + 2] == 0xFF) {
            usage_fff0 = true;
        }
        if (report_desc[i] == 0x85 && i + 1 < desc_len) {
            if (report_desc[i + 1] == kVendorReportIdCmd) report_id_0f_present_ = true;
            if (report_desc[i + 1] == kVendorReportIdEnhanced) report_id_12_present_ = true;
        }
    }
    return usage_fff0;
}

void GameSirCyclone2Host::dump_device_banner(uint8_t address, uint8_t instance,
                                              const uint8_t* report_desc, uint16_t desc_len) {
    (void)report_desc;
    (void)desc_len;
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    /* Mount-path safe: no *_sync control transfers / hex dumps (nested tuh_task hang risk). */
    OGXM_LOG("[CYCLONE2 RXR] mount addr=%u inst=%u %04X:%04X personality=%u desc_len=%u\n",
             static_cast<unsigned>(address), static_cast<unsigned>(instance),
             vid, pid, static_cast<unsigned>(personality_),
             static_cast<unsigned>(desc_len));
    classify_and_log_interface(address, instance, report_desc, desc_len);
}

void GameSirCyclone2Host::log_raw_report(uint8_t address, uint8_t instance,
                                          const uint8_t* report, uint16_t len) {
    if (!report || len == 0) return;
    const uint16_t n = (len < sizeof(prev_report_)) ? len : static_cast<uint16_t>(sizeof(prev_report_));
    const bool changed = (n != prev_len_) || (std::memcmp(prev_report_, report, n) != 0);
    const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (!changed && (now_ms - last_unchanged_log_ms_) < kUnchangedLogPeriodMs) return;

    prev_len_ = n;
    std::memcpy(prev_report_, report, n);
    last_unchanged_log_ms_ = now_ms;

    OGXM_LOG("\n[%s RX]\n", is_hid_path_ ? "CYCLONE2 ENHANCED" : "CYCLONE2 XINPUT");
    OGXM_LOG("interface: %u\n", static_cast<unsigned>(itf_num_));
    OGXM_LOG("instance: %u\n", static_cast<unsigned>(instance));
    OGXM_LOG("endpoint/path: %s\n", is_hid_path_ ? "HID" : "XINPUT");
    OGXM_LOG("length: %u\n", static_cast<unsigned>(len));
    OGXM_LOG("timestamp_ms: %lu\n", static_cast<unsigned long>(now_ms));
    OGXM_LOG("RAW:\n");
    OGXM_LOG_HEX(report, len);
}

void GameSirCyclone2Host::process_xinput_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                                 const uint8_t* report, uint16_t len) {
    const auto* in = reinterpret_cast<const XInput::InReport*>(report);
    if (std::memcmp(&prev_xinput_report_, in, std::min(static_cast<size_t>(len), sizeof(XInput::InReport))) == 0) {
        tuh_xinput::receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp{};
    if (in->buttons[0] & XInput::Buttons0::DPAD_UP) gp.dpad |= gamepad.MAP_DPAD_UP;
    if (in->buttons[0] & XInput::Buttons0::DPAD_DOWN) gp.dpad |= gamepad.MAP_DPAD_DOWN;
    if (in->buttons[0] & XInput::Buttons0::DPAD_LEFT) gp.dpad |= gamepad.MAP_DPAD_LEFT;
    if (in->buttons[0] & XInput::Buttons0::DPAD_RIGHT) gp.dpad |= gamepad.MAP_DPAD_RIGHT;
    if (in->buttons[0] & XInput::Buttons0::START) gp.buttons |= gamepad.MAP_BUTTON_START;
    if (in->buttons[0] & XInput::Buttons0::BACK) gp.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in->buttons[0] & XInput::Buttons0::L3) gp.buttons |= gamepad.MAP_BUTTON_L3;
    if (in->buttons[0] & XInput::Buttons0::R3) gp.buttons |= gamepad.MAP_BUTTON_R3;
    if (in->buttons[1] & XInput::Buttons1::LB) gp.buttons |= gamepad.MAP_BUTTON_LB;
    if (in->buttons[1] & XInput::Buttons1::RB) gp.buttons |= gamepad.MAP_BUTTON_RB;
    if (in->buttons[1] & XInput::Buttons1::HOME) gp.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in->buttons[1] & XInput::Buttons1::A) gp.buttons |= gamepad.MAP_BUTTON_A;
    if (in->buttons[1] & XInput::Buttons1::B) gp.buttons |= gamepad.MAP_BUTTON_B;
    if (in->buttons[1] & XInput::Buttons1::X) gp.buttons |= gamepad.MAP_BUTTON_X;
    if (in->buttons[1] & XInput::Buttons1::Y) gp.buttons |= gamepad.MAP_BUTTON_Y;

    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    if (XboxArcadeStick::is_xbox360_digital_triggers(vid, pid)) {
        gp.trigger_l = in->trigger_l ? 0xFF : 0;
        gp.trigger_r = in->trigger_r ? 0xFF : 0;
    } else {
        gp.trigger_l = gamepad.scale_trigger_l(in->trigger_l);
        gp.trigger_r = gamepad.scale_trigger_r(in->trigger_r);
    }
    std::tie(gp.joystick_lx, gp.joystick_ly) = gamepad.scale_joystick_l(in->joystick_lx, in->joystick_ly, true);
    std::tie(gp.joystick_rx, gp.joystick_ry) = gamepad.scale_joystick_r(in->joystick_rx, in->joystick_ry, true);

    gamepad.set_pad_in(gp);
    std::memcpy(&prev_xinput_report_, in, sizeof(XInput::InReport));
    log_raw_report(address, instance, report, len);
    tuh_xinput::receive_report(address, instance);
}

void GameSirCyclone2Host::process_enhanced_report(uint8_t address, uint8_t instance,
                                                   const uint8_t* report, uint16_t len) {
    if (!report || len == 0) return;
    if (report[0] != kVendorReportIdEnhanced) return;

    enhanced_report_seen_ = true;
    const uint16_t n = (len < sizeof(prev_12_report_)) ? len : static_cast<uint16_t>(sizeof(prev_12_report_));
    const bool changed = (n != prev_12_len_) || (std::memcmp(prev_12_report_, report, n) != 0);
    if (changed) {
        if (prev_12_len_ != 0) enhanced_report_changing_ = true;
        prev_12_len_ = n;
        std::memcpy(prev_12_report_, report, n);
    }

    if (!enhanced_banner_printed_) {
        enhanced_banner_printed_ = true;
        OGXM_LOG("\n================================================\n");
        OGXM_LOG("CYCLONE 2 ENHANCED INPUT ENABLED\n");
        OGXM_LOG("================================================\n");
        OGXM_LOG("Heartbeat: 0F F2\n");
        OGXM_LOG("Report 0x12: FOUND\n");
        OGXM_LOG("Report length: %u\n", static_cast<unsigned>(len));
        OGXM_LOG("Interface: %u  Instance: %u\n", static_cast<unsigned>(itf_num_), static_cast<unsigned>(instance));
        OGXM_LOG("================================================\n");
    }

    log_raw_report(address, instance, report, len);
    if (changed) log_input_source_summary_once();
}

void GameSirCyclone2Host::log_input_source_summary_once() {
    if (input_source_summary_printed_) return;
    input_source_summary_printed_ = true;
    OGXM_LOG("\n[CYCLONE2 INPUT SOURCES]\n");
    OGXM_LOG("A/B/X/Y: XInput=YES Enhanced=YES\n");
    OGXM_LOG("Sticks/Triggers: XInput=YES Enhanced=YES\n");
    OGXM_LOG("L4: XInput=NO Enhanced=YES (bit mapping pending capture)\n");
    OGXM_LOG("R4: XInput=NO Enhanced=YES (bit mapping pending capture)\n");
    OGXM_LOG("M:  XInput=NO Enhanced=YES (bit mapping pending capture)\n");
}

void GameSirCyclone2Host::maybe_start_vendor_heartbeat(uint8_t address, uint8_t instance,
                                                        const uint8_t* report_desc, uint16_t desc_len) {
    vendor_hid_candidate_ = is_vendor_hid_descriptor(report_desc, desc_len);
    if (!vendor_hid_candidate_ || is_boot_kb_mouse_) {
        return;
    }

    heartbeat_enabled_ = true;
    next_heartbeat_ms_ = to_ms_since_boot(get_absolute_time());
    if (!heartbeat_announced_) {
        heartbeat_announced_ = true;
        OGXM_LOG("\n[CYCLONE2 VENDOR HID]\n");
        OGXM_LOG("Interface: %u\n", static_cast<unsigned>(itf_num_));
        OGXM_LOG("Endpoint IN: (from HID interface)\n");
        OGXM_LOG("Endpoint OUT: (from HID interface)\n");
        OGXM_LOG("HID report descriptor length: %u\n", static_cast<unsigned>(desc_len));
        OGXM_LOG("0x0F present: %s\n", report_id_0f_present_ ? "YES" : "NO");
        OGXM_LOG("0x12 present: %s\n", report_id_12_present_ ? "YES" : "NO");
    }

    tid_heartbeat_ = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(tid_heartbeat_, 100, true, [this, address, instance] {
        if (!heartbeat_enabled_) return;
        const uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now < next_heartbeat_ms_) return;
        next_heartbeat_ms_ = now + kHeartbeatPeriodMs;
        const uint8_t cmd[2] = {kVendorReportIdCmd, kVendorCmdHeartbeat};
        const bool ok = tuh_hid_send_report(address, instance, 0, cmd, sizeof(cmd));
        OGXM_LOG("[CYCLONE2] heartbeat 0F F2 itf=%u %s\n",
                 static_cast<unsigned>(itf_num_), ok ? "queued" : "FAILED");
    });
}

void GameSirCyclone2Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                      const uint8_t* report_desc, uint16_t desc_len) {
    (void)gamepad;
    address_ = address;
    instance_ = instance;
    init_ms_ = to_ms_since_boot(get_absolute_time());
    prev_len_ = 0;
    prev_12_len_ = 0;
    std::memset(prev_report_, 0, sizeof(prev_report_));
    std::memset(prev_12_report_, 0, sizeof(prev_12_report_));
    heartbeat_enabled_ = false;

    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    personality_ = infer_personality(vid, pid);

    is_hid_path_ = (report_desc != nullptr && desc_len > 0) ||
                   (personality_ == Personality::SwitchNs) ||
                   (personality_ == Personality::Ds4) ||
                   (personality_ == Personality::HidPassive);
    if (is_hid_path_ && report_desc != nullptr && desc_len > 0) {
        tuh_itf_info_t info{};
        if (tuh_hid_itf_get_info(address, instance, &info)) {
            itf_num_ = info.desc.bInterfaceNumber;
            is_boot_kb_mouse_ = (info.desc.bInterfaceProtocol == HID_ITF_PROTOCOL_KEYBOARD ||
                                 info.desc.bInterfaceProtocol == HID_ITF_PROTOCOL_MOUSE);
        }
    }

    dump_device_banner(address, instance, report_desc, desc_len);

    if (personality_ == Personality::HidPassive) {
        GameSirCyclone2Trace::note_0575_mounted();
        GameSirCyclone2Trace::log_unified_banner(vid, pid, nullptr, false, "HID_PASSIVE");
        GameSirCyclone2Trace::set_hid_passive_safety(address, true);
        hid_passive_.start(address, instance, report_desc, desc_len);
        return;
    }

    GameSirCyclone2Trace::note_controller_personality_mounted(vid, pid);

    if (personality_ == Personality::SwitchNs) {
        GameSirCyclone2Trace::log_switch_driver_selection_banner(address, vid, pid);
        GameSirCyclone2Trace::log_unified_banner(vid, pid, nullptr, true, "SWITCH_PRO_STANDARD");
        switch_wired_.start(gamepad, address, instance, report_desc, desc_len);
        return;
    }

    if (personality_ == Personality::Ds4) {
        GameSirCyclone2Trace::log_unified_banner(vid, pid, nullptr, true, "DS4");
        ds4_wired_.start(address, instance, idx_);
        return;
    }

    if (is_hid_path_) {
        maybe_start_vendor_heartbeat(address, instance, report_desc, desc_len);
        tuh_hid_receive_report(address, instance);
        return;
    }

    GameSirCyclone2Trace::log_unified_banner(vid, pid, nullptr, true, "XINPUT");
    GameSirCyclone2Trace::note_cyclone_xinput_seen();
    (void)tuh_xinput::set_led(address, instance, idx_ + 1, true);
    tuh_xinput::receive_report(address, instance);
}

void GameSirCyclone2Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                         const uint8_t* report, uint16_t len) {
    if (personality_ == Personality::SwitchNs) {
        switch_wired_.process_report(gamepad, address, instance, report, len);
        return;
    }
    if (personality_ == Personality::Ds4) {
        ds4_wired_.process_report(gamepad, address, instance, report, len);
        return;
    }
    if (personality_ == Personality::HidPassive) {
        hid_passive_.process_report(address, instance, report, len);
        return;
    }
    if (is_hid_path_) {
        process_enhanced_report(address, instance, report, len);
        tuh_hid_receive_report(address, instance);
    } else {
        process_xinput_report(gamepad, address, instance, report, len);
    }
}

bool GameSirCyclone2Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) {
    if (personality_ == Personality::SwitchNs) {
        return switch_wired_.send_feedback(gamepad, address, instance);
    }
    if (personality_ == Personality::Ds4) {
        return ds4_wired_.send_feedback(gamepad, address, instance);
    }
    if (personality_ == Personality::HidPassive) {
        if (GameSirCyclone2Trace::hid_passive_safety_active(address)) {
            /* Rumble/LED would be application OUT — block explicitly. */
            return hid_passive_.send_feedback(address, instance);
        }
        return true;
    }
    const Gamepad::PadOut out = gamepad.get_pad_out();
    return tuh_xinput::set_rumble(address, instance, out.rumble_l, out.rumble_r, false);
}

void GameSirCyclone2Host::disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) {
    (void)instance;
    if (tid_heartbeat_ != 0) {
        TaskQueue::Core1::cancel_delayed_task(tid_heartbeat_);
        tid_heartbeat_ = 0;
    }
    heartbeat_enabled_ = false;
    if (personality_ == Personality::HidPassive) {
        GameSirCyclone2Trace::set_hid_passive_safety(address, false);
        hid_passive_.disconnect();
    }
    switch_wired_.disconnect(gamepad, address, instance);
    ds4_wired_.reset();
    personality_ = Personality::Unknown;
}
