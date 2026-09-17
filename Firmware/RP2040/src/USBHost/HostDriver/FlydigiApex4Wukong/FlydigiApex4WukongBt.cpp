#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBt.h"
#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBtProbe.h"

#include <cstdio>
#include <cstring>

#include "Board/ogxm_log.h"
#include "bt/uni_bt_conn.h"
#include "controller/uni_controller.h"
#include "controller/uni_gamepad.h"
#include "parser/uni_hid_parser.h"
#include "sdkconfig.h"
#include "uni_hid_device.h"

namespace {

constexpr uint8_t kReportIdInput = 0x01;
constexpr uint16_t kReportLen = 16;
constexpr int32_t kTriggerButtonThreshold = 32;

constexpr uint16_t kBtnA = 0x0001;
constexpr uint16_t kBtnB = 0x0002;
constexpr uint16_t kBtnX = 0x0004;
constexpr uint16_t kBtnY = 0x0008;
constexpr uint16_t kBtnLb = 0x0010;
constexpr uint16_t kBtnRb = 0x0020;
constexpr uint16_t kBtnView = 0x0040;
constexpr uint16_t kBtnMenu = 0x0080;
constexpr uint16_t kBtnL3 = 0x0100;
constexpr uint16_t kBtnR3 = 0x0200;
constexpr uint16_t kBtnKnownMask = 0x03FFu;

constexpr unsigned kMaxSlots = CONFIG_BLUEPAD32_MAX_DEVICES;

struct SlotState {
    bool candidate{false};
    bool layout_confirmed{false};
    bool final_installed{false};
    bool prev_valid{false};
    uint16_t unknown_bits_seen{0};
    uint8_t prev_report[kReportLen]{};
};

SlotState s_slot[kMaxSlots]{};

int device_slot(const uni_hid_device_t* d)
{
    if (!d)
        return -1;
    const int idx = uni_hid_device_get_idx_for_instance(const_cast<uni_hid_device_t*>(d));
    if (idx < 0 || idx >= static_cast<int>(kMaxSlots))
        return -1;
    return idx;
}

bool device_is_ready(const uni_hid_device_t* d)
{
    return d && uni_bt_conn_get_state(&d->conn) == UNI_BT_CONN_STATE_DEVICE_READY;
}

bool report_matches_apex4_layout(const uint8_t* report, uint16_t len)
{
    if (!report || len != kReportLen || report[0] != kReportIdInput)
        return false;
    /* Hat nibble is typically 1..9 for this layout (9 = neutral). */
    const uint8_t hat = report[13] & 0x0Fu;
    return hat <= 0x0Fu;
}

int32_t normalize_u16_axis(uint16_t raw)
{
    return static_cast<int32_t>(raw) * AXIS_NORMALIZE_RANGE / 65536 - AXIS_NORMALIZE_RANGE / 2;
}

int32_t normalize_trigger10(uint16_t raw10)
{
    return static_cast<int32_t>(raw10 & 0x3FFu) * AXIS_NORMALIZE_RANGE / 1024;
}

#if defined(CONFIG_OGXM_DEBUG)
void log_parser_ownership(const char* state, uni_hid_device_t* d, const char* parser_name)
{
    OGXM_LOG("\n[HID PARSER OWNERSHIP]\n");
    OGXM_LOG("state=%s\n", state);
    OGXM_LOG("parser=%s\n", parser_name);
    if (d) {
        OGXM_LOG("VID=%04X PID=%04X name=\"%s\"\n", d->vendor_id, d->product_id,
                 d->name[0] ? d->name : "");
        OGXM_LOG("init_report=%p\n", reinterpret_cast<const void*>(d->report_parser.init_report));
        OGXM_LOG("parse_input_report=%p\n",
                 reinterpret_cast<const void*>(d->report_parser.parse_input_report));
        OGXM_LOG("parse_usage=%p\n", reinterpret_cast<const void*>(d->report_parser.parse_usage));
        OGXM_LOG("setup=%p\n", reinterpret_cast<const void*>(d->report_parser.setup));
    }
    OGXM_LOG("\n");
}

void log_decoded(const uint8_t* report, const uni_controller_t* ctl, bool post_ready)
{
    const uint16_t lx = static_cast<uint16_t>(report[1] | (report[2] << 8));
    const uint16_t ly = static_cast<uint16_t>(report[3] | (report[4] << 8));
    const uint16_t rx = static_cast<uint16_t>(report[5] | (report[6] << 8));
    const uint16_t ry = static_cast<uint16_t>(report[7] | (report[8] << 8));
    const uint16_t lt = static_cast<uint16_t>((report[9] | (report[10] << 8)) & 0x3FFu);
    const uint16_t rt = static_cast<uint16_t>((report[11] | (report[12] << 8)) & 0x3FFu);
    const uint8_t hat = report[13];
    const uint16_t buttons = static_cast<uint16_t>(report[14] | (report[15] << 8));

    OGXM_LOG("%s\n", post_ready ? "[APEX4 BT RX POST-READY]" : "[APEX4 BT RX PRE-READY]");
    OGXM_LOG("[1] APEX RAW:");
    for (uint16_t i = 0; i < kReportLen; ++i)
        OGXM_LOG(" %02x", report[i]);
    OGXM_LOG("\n");
    OGXM_LOG("[2] APEX PARSED: LX=%u LY=%u RX=%u RY=%u LT=%u RT=%u HAT=%u BUTTONS=0x%04x\n",
             lx, ly, rx, ry, lt, rt, hat, buttons);
    OGXM_LOG("NORMALIZED: axis_x=%ld axis_y=%ld axis_rx=%ld axis_ry=%ld brake=%ld throttle=%ld\n",
             static_cast<long>(ctl->gamepad.axis_x), static_cast<long>(ctl->gamepad.axis_y),
             static_cast<long>(ctl->gamepad.axis_rx), static_cast<long>(ctl->gamepad.axis_ry),
             static_cast<long>(ctl->gamepad.brake), static_cast<long>(ctl->gamepad.throttle));
    OGXM_LOG("  A=%d B=%d X=%d Y=%d LB=%d RB=%d L3=%d R3=%d View=%d Menu=%d dpad=0x%02x\n",
             (ctl->gamepad.buttons & BUTTON_A) != 0, (ctl->gamepad.buttons & BUTTON_B) != 0,
             (ctl->gamepad.buttons & BUTTON_X) != 0, (ctl->gamepad.buttons & BUTTON_Y) != 0,
             (ctl->gamepad.buttons & BUTTON_SHOULDER_L) != 0, (ctl->gamepad.buttons & BUTTON_SHOULDER_R) != 0,
             (ctl->gamepad.buttons & BUTTON_THUMB_L) != 0, (ctl->gamepad.buttons & BUTTON_THUMB_R) != 0,
             (ctl->gamepad.misc_buttons & MISC_BUTTON_SELECT) != 0,
             (ctl->gamepad.misc_buttons & MISC_BUTTON_START) != 0, ctl->gamepad.dpad);
    if (post_ready) {
        OGXM_LOG("[APEX4 -> OGX] klass=%d buttons=0x%04x misc=0x%02x dpad=0x%02x\n",
                 static_cast<int>(ctl->klass), ctl->gamepad.buttons, ctl->gamepad.misc_buttons,
                 ctl->gamepad.dpad);
    }
}

const char* describe_current_parser(const uni_hid_device_t* d)
{
    if (!d)
        return "NONE";
    if (d->report_parser.parse_input_report == flydigi_apex4_bt_parse_input_report)
        return "APEX4_WUKONG_BT";
    if (d->report_parser.parse_usage != nullptr && d->report_parser.parse_input_report == nullptr)
        return "USAGE_BASED (Xbox/generic/etc)";
    if (d->report_parser.parse_input_report != nullptr)
        return "OTHER_RAW";
    return "EMPTY/UNKNOWN";
}
#endif

bool apex_parser_ptrs_installed(const uni_hid_device_t* d)
{
    return d && d->report_parser.parse_input_report == flydigi_apex4_bt_parse_input_report &&
           d->report_parser.parse_usage == nullptr;
}

void install_final_parser(uni_hid_device_t* device, const char* reason)
{
    if (!device)
        return;

    const int slot = device_slot(device);
#if defined(CONFIG_OGXM_DEBUG)
    const char* before = describe_current_parser(device);
    log_parser_ownership(reason, device, before);
#endif

    /*
     * Own decoding only. Clear setup so uni_hid_device_set_ready() does not run
     * Xbox setup() after we take ownership (that would call set_ready_complete
     * while still pointing at Xbox parsers if we only patched parse_*).
     * Clear parse_usage so HID descriptor walking cannot overwrite state.
     */
    device->report_parser.init_report = flydigi_apex4_bt_init_report;
    device->report_parser.parse_input_report = flydigi_apex4_bt_parse_input_report;
    device->report_parser.parse_usage = nullptr;
    device->report_parser.setup = nullptr;
    device->report_parser.device_dump = flydigi_apex4_bt_device_dump;
    /* Leave play_dual_rumble as previously set (often Xbox) until dedicated rumble. */

    if (slot >= 0) {
        s_slot[slot].final_installed = true;
        s_slot[slot].prev_valid = false;
        s_slot[slot].unknown_bits_seen = 0;
    }

#if defined(CONFIG_OGXM_DEBUG)
    log_parser_ownership("AFTER_APEX4_FINAL_INSTALL", device, "APEX4_WUKONG_BT");
    OGXM_LOG("[APEX4 BT] FINAL PARSER = Flydigi APEX 4 Wukong Bluetooth (%s)\n", reason);
#endif
}

void try_finalize(uni_hid_device_t* device, const char* reason)
{
    if (!device)
        return;
    const int slot = device_slot(device);
    if (slot < 0)
        return;
    if (!s_slot[slot].candidate || !s_slot[slot].layout_confirmed)
        return;

    if (apex_parser_ptrs_installed(device) && s_slot[slot].final_installed)
        return;

    install_final_parser(device, reason);
}

} // namespace

extern "C" void flydigi_apex4_bt_device_dump(uni_hid_device_t* d)
{
    (void)d;
#if defined(CONFIG_OGXM_DEBUG)
    OGXM_LOG("\tFlydigi model: 'Flydigi APEX 4 Wukong' (dedicated BT Report 0x01)\n");
#endif
}

extern "C" void flydigi_apex4_bt_mark_candidate(uni_hid_device_t* device)
{
    const int slot = device_slot(device);
    if (slot < 0)
        return;
    if (!s_slot[slot].candidate) {
        s_slot[slot].candidate = true;
#if defined(CONFIG_OGXM_DEBUG)
        OGXM_LOG("[APEX4 BT] candidate marked slot=%d (awaiting 16-byte Report 0x01 layout)\n", slot);
#endif
    }
}

extern "C" void flydigi_apex4_bt_note_raw_report(uni_hid_device_t* device, const uint8_t* report, uint16_t len)
{
    if (!device || !report)
        return;
    const int slot = device_slot(device);
    if (slot < 0)
        return;

    if (!s_slot[slot].candidate && flydigi_apex4_bt_is_candidate(device))
        flydigi_apex4_bt_mark_candidate(device);

    if (!s_slot[slot].candidate)
        return;

    if (!s_slot[slot].layout_confirmed && report_matches_apex4_layout(report, len)) {
        s_slot[slot].layout_confirmed = true;
#if defined(CONFIG_OGXM_DEBUG)
        OGXM_LOG("[APEX4 BT] layout CONFIRMED: Report ID 0x01 len=16 (runtime fingerprint)\n");
#endif
        /* If SDP/parser selection already finished, claim ownership now. */
        try_finalize(device, "LAYOUT_CONFIRMED_AFTER_SELECTION");
    }
}

extern "C" void flydigi_apex4_bt_on_parser_selection_complete(uni_hid_device_t* device)
{
    if (!device)
        return;
#if defined(CONFIG_OGXM_DEBUG)
    log_parser_ownership("SDP_VID_PID_PARSER_SELECTION", device, describe_current_parser(device));
#endif
    if (flydigi_apex4_bt_is_candidate(device))
        flydigi_apex4_bt_mark_candidate(device);
    try_finalize(device, "PARSER_SELECTION_COMPLETE");
}

extern "C" void flydigi_apex4_bt_on_device_ready_transition(uni_hid_device_t* device)
{
    if (!device)
        return;
    if (flydigi_apex4_bt_is_candidate(device))
        flydigi_apex4_bt_mark_candidate(device);
    try_finalize(device, "DEVICE_READY_TRANSITION");
#if defined(CONFIG_OGXM_DEBUG)
    if (apex_parser_ptrs_installed(device)) {
        OGXM_LOG("[APEX4 BT] POST-READY parser = APEX4_WUKONG_BT\n");
    }
#endif
}

extern "C" void flydigi_apex4_bt_on_slot_disconnected(uni_hid_device_t* device)
{
    const int slot = device_slot(device);
    if (slot < 0)
        return;
    s_slot[slot] = {};
}

extern "C" int flydigi_apex4_bt_layout_confirmed(const uni_hid_device_t* device)
{
    const int slot = device_slot(device);
    return (slot >= 0 && s_slot[slot].layout_confirmed) ? 1 : 0;
}

extern "C" int flydigi_apex4_bt_parser_installed(const uni_hid_device_t* device)
{
    return apex_parser_ptrs_installed(device) ? 1 : 0;
}

extern "C" void flydigi_apex4_bt_init_report(uni_hid_device_t* d)
{
    if (!d)
        return;
    uni_controller_t* ctl = &d->controller;
    std::memset(ctl, 0, sizeof(*ctl));
    ctl->klass = UNI_CONTROLLER_CLASS_GAMEPAD;
}

extern "C" void flydigi_apex4_bt_parse_input_report(uni_hid_device_t* d, const uint8_t* report, uint16_t len)
{
    if (!d || !report)
        return;

    if (len < 1)
        return;
    if (report[0] != kReportIdInput) {
#if defined(CONFIG_OGXM_DEBUG)
        static uint8_t s_other_ids[8]{};
        static uint8_t s_other_n = 0;
        bool seen = false;
        for (uint8_t i = 0; i < s_other_n; ++i) {
            if (s_other_ids[i] == report[0]) {
                seen = true;
                break;
            }
        }
        if (!seen && s_other_n < sizeof(s_other_ids)) {
            s_other_ids[s_other_n++] = report[0];
            OGXM_LOG("[APEX4 BT] non-0x01 report id=0x%02x len=%u (not decoded yet)\n", report[0], len);
        }
#endif
        return;
    }
    if (len != kReportLen)
        return;

    const int slot = device_slot(d);
    const bool changed = (slot < 0) || !s_slot[slot].prev_valid ||
                         std::memcmp(s_slot[slot].prev_report, report, kReportLen) != 0;
    if (slot >= 0 && changed) {
        std::memcpy(s_slot[slot].prev_report, report, kReportLen);
        s_slot[slot].prev_valid = true;
    }

    uni_controller_t* ctl = &d->controller;
    ctl->klass = UNI_CONTROLLER_CLASS_GAMEPAD;

    const uint16_t lx = static_cast<uint16_t>(report[1] | (report[2] << 8));
    const uint16_t ly = static_cast<uint16_t>(report[3] | (report[4] << 8));
    const uint16_t rx = static_cast<uint16_t>(report[5] | (report[6] << 8));
    const uint16_t ry = static_cast<uint16_t>(report[7] | (report[8] << 8));
    ctl->gamepad.axis_x = normalize_u16_axis(lx);
    ctl->gamepad.axis_y = normalize_u16_axis(ly);
    ctl->gamepad.axis_rx = normalize_u16_axis(rx);
    ctl->gamepad.axis_ry = normalize_u16_axis(ry);

    const uint16_t lt = static_cast<uint16_t>((report[9] | (report[10] << 8)) & 0x3FFu);
    const uint16_t rt = static_cast<uint16_t>((report[11] | (report[12] << 8)) & 0x3FFu);
    ctl->gamepad.brake = normalize_trigger10(lt);
    ctl->gamepad.throttle = normalize_trigger10(rt);
    if (ctl->gamepad.brake >= kTriggerButtonThreshold)
        ctl->gamepad.buttons |= BUTTON_TRIGGER_L;
    if (ctl->gamepad.throttle >= kTriggerButtonThreshold)
        ctl->gamepad.buttons |= BUTTON_TRIGGER_R;

    const uint8_t hat_raw = report[13] & 0x0Fu;
    if (hat_raw >= 1 && hat_raw <= 8)
        ctl->gamepad.dpad = uni_hid_parser_hat_to_dpad(static_cast<uint8_t>(hat_raw - 1));
    else
        ctl->gamepad.dpad = 0;

    const uint16_t buttons = static_cast<uint16_t>(report[14] | (report[15] << 8));
    if (buttons & kBtnA)
        ctl->gamepad.buttons |= BUTTON_A;
    if (buttons & kBtnB)
        ctl->gamepad.buttons |= BUTTON_B;
    if (buttons & kBtnX)
        ctl->gamepad.buttons |= BUTTON_X;
    if (buttons & kBtnY)
        ctl->gamepad.buttons |= BUTTON_Y;
    if (buttons & kBtnLb)
        ctl->gamepad.buttons |= BUTTON_SHOULDER_L;
    if (buttons & kBtnRb)
        ctl->gamepad.buttons |= BUTTON_SHOULDER_R;
    if (buttons & kBtnView)
        ctl->gamepad.misc_buttons |= MISC_BUTTON_SELECT;
    if (buttons & kBtnMenu)
        ctl->gamepad.misc_buttons |= MISC_BUTTON_START;
    if (buttons & kBtnL3)
        ctl->gamepad.buttons |= BUTTON_THUMB_L;
    if (buttons & kBtnR3)
        ctl->gamepad.buttons |= BUTTON_THUMB_R;

    const uint16_t unknown = static_cast<uint16_t>(buttons & ~kBtnKnownMask);
    if (unknown && slot >= 0) {
        const uint16_t newly = static_cast<uint16_t>(unknown & ~s_slot[slot].unknown_bits_seen);
        if (newly) {
            s_slot[slot].unknown_bits_seen = static_cast<uint16_t>(s_slot[slot].unknown_bits_seen | newly);
#if defined(CONFIG_OGXM_DEBUG)
            for (int bit = 10; bit < 16; ++bit) {
                if (newly & (1u << bit))
                    OGXM_LOG("[APEX4 BT UNKNOWN BUTTON BIT] bit=%d raw_buttons=0x%04x\n", bit, buttons);
            }
#endif
        }
    }

#if defined(CONFIG_OGXM_DEBUG)
    if (changed)
        log_decoded(report, ctl, device_is_ready(d));
#else
    (void)changed;
#endif
}
