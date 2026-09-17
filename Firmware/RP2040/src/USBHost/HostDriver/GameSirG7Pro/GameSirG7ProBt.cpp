#include "USBHost/HostDriver/GameSirG7Pro/GameSirG7ProBt.h"

#include <cstring>

#include "Board/ogxm_log.h"
#include "controller/uni_controller.h"
#include "controller/uni_gamepad.h"
#include "parser/uni_hid_parser.h"
#include "sdkconfig.h"

namespace {

constexpr uint16_t kVid = 0x3537;
constexpr uint16_t kPid = 0x1022;

/** Bluetooth gamepad report (OpenMicro / doctor capture). */
constexpr uint8_t kReportIdInput = 0x07;
constexpr uint16_t kReportLen = 11;
/** Home arrives on a separate consumer-control report. */
constexpr uint8_t kReportIdHome = 0x02;
constexpr uint8_t kHomeBit = 0x80;

/* byte 6 — face / shoulders (not contiguous Xbox bit order). */
constexpr uint8_t kB6A = 0x01;
constexpr uint8_t kB6B = 0x02;
constexpr uint8_t kB6X = 0x08;
constexpr uint8_t kB6Y = 0x10;
constexpr uint8_t kB6Lb = 0x40;
constexpr uint8_t kB6Rb = 0x80;

/* byte 7 — triggers digital + system + sticks. */
constexpr uint8_t kB7Lt = 0x01;
constexpr uint8_t kB7Rt = 0x02;
constexpr uint8_t kB7View = 0x04;
constexpr uint8_t kB7Menu = 0x08;
constexpr uint8_t kB7L3 = 0x20;
constexpr uint8_t kB7R3 = 0x40;

constexpr int32_t kTriggerDigitalThreshold = 32;

constexpr unsigned kMaxSlots = CONFIG_BLUEPAD32_MAX_DEVICES;
bool s_home_held[kMaxSlots]{};

int device_slot(const uni_hid_device_t* d) {
    if (!d) {
        return -1;
    }
    const int idx = uni_hid_device_get_idx_for_instance(const_cast<uni_hid_device_t*>(d));
    return (idx >= 0 && idx < static_cast<int>(kMaxSlots)) ? idx : -1;
}

bool name_is_g7_pro(const char* name) {
    if (!name || !name[0]) {
        return false;
    }
    /* Exact names seen in pairing / GameSir manuals. */
    if (std::strcmp(name, "GameSir-G7 Pro") == 0 || std::strcmp(name, "Gamesir-G7 Pro") == 0 ||
        std::strcmp(name, "GameSir-G7 Pro 8K") == 0 || std::strcmp(name, "Gamesir-G7 Pro 8K") == 0) {
        return true;
    }
    /* Loose but still GameSir-scoped: avoid claiming unrelated "G7" devices. */
    const bool has_gamesir =
        std::strstr(name, "GameSir") != nullptr || std::strstr(name, "Gamesir") != nullptr;
    return has_gamesir && std::strstr(name, "G7 Pro") != nullptr;
}

int32_t axis_u8_centered(uint8_t v) {
    /* 0..255 center 128 → Bluepad32 -512..511-ish (AXIS_NORMALIZE_RANGE=1024). */
    return (static_cast<int32_t>(v) - 128) * AXIS_NORMALIZE_RANGE / 256;
}

int32_t trigger_u8(uint8_t v) {
    return static_cast<int32_t>(v) * AXIS_NORMALIZE_RANGE / 256;
}

void apply_home(uni_controller_t* ctl, int slot) {
    if (slot >= 0 && s_home_held[static_cast<unsigned>(slot)]) {
        ctl->gamepad.misc_buttons |= MISC_BUTTON_SYSTEM;
    }
}

} // namespace

extern "C" bool gamesir_g7pro_bt_does_name_match(uni_hid_device_t* d, const char* name) {
    if (!d || !name_is_g7_pro(name)) {
        return false;
    }
    uni_hid_device_set_vendor_id(d, kVid);
    uni_hid_device_set_product_id(d, kPid);
    OGXM_LOG("[G7PRO BT] name match \"%s\" → VID=%04X PID=%04X (skip SDP)\n", name, kVid, kPid);
    return true;
}

extern "C" void gamesir_g7pro_bt_install_parser(uni_hid_device_t* d) {
    if (!d) {
        return;
    }
    std::memset(&d->report_parser, 0, sizeof(d->report_parser));
    d->report_parser.init_report = gamesir_g7pro_bt_init_report;
    d->report_parser.parse_input_report = gamesir_g7pro_bt_parse_input_report;
    /* No setup() — uni_hid_device_set_ready() completes immediately. */
    OGXM_LOG("[G7PRO BT] dedicated Report 0x07 parser installed\n");
}

extern "C" bool gamesir_g7pro_bt_parser_installed(const uni_hid_device_t* d) {
    return d && d->report_parser.parse_input_report == gamesir_g7pro_bt_parse_input_report;
}

extern "C" void gamesir_g7pro_bt_init_report(uni_hid_device_t* d) {
    if (!d) {
        return;
    }
    uni_controller_t* ctl = &d->controller;
    std::memset(ctl, 0, sizeof(*ctl));
    ctl->klass = UNI_CONTROLLER_CLASS_GAMEPAD;
}

extern "C" void gamesir_g7pro_bt_parse_input_report(uni_hid_device_t* d, const uint8_t* report, uint16_t len) {
    if (!d || !report || len < 1) {
        return;
    }

    const int slot = device_slot(d);
    uni_controller_t* ctl = &d->controller;
    ctl->klass = UNI_CONTROLLER_CLASS_GAMEPAD;

    if (report[0] == kReportIdHome && len >= 2) {
        if (slot >= 0) {
            s_home_held[static_cast<unsigned>(slot)] = (report[1] & kHomeBit) != 0;
        }
        apply_home(ctl, slot);
        return;
    }

    if (report[0] != kReportIdInput || len < 10) {
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
            OGXM_LOG("[G7PRO BT] ignore report id=0x%02x len=%u\n", report[0], len);
        }
#endif
        return;
    }

    ctl->gamepad.axis_x = axis_u8_centered(report[1]);
    ctl->gamepad.axis_y = axis_u8_centered(report[2]);
    ctl->gamepad.axis_rx = axis_u8_centered(report[3]);
    ctl->gamepad.axis_ry = axis_u8_centered(report[4]);

    const uint8_t hat = report[5] & 0x0Fu;
    if (hat <= 7) {
        ctl->gamepad.dpad = uni_hid_parser_hat_to_dpad(hat);
    } else {
        ctl->gamepad.dpad = 0;
    }

    const uint8_t b6 = report[6];
    const uint8_t b7 = report[7];

    if (b6 & kB6A)
        ctl->gamepad.buttons |= BUTTON_A;
    if (b6 & kB6B)
        ctl->gamepad.buttons |= BUTTON_B;
    if (b6 & kB6X)
        ctl->gamepad.buttons |= BUTTON_X;
    if (b6 & kB6Y)
        ctl->gamepad.buttons |= BUTTON_Y;
    if (b6 & kB6Lb)
        ctl->gamepad.buttons |= BUTTON_SHOULDER_L;
    if (b6 & kB6Rb)
        ctl->gamepad.buttons |= BUTTON_SHOULDER_R;

    if (b7 & kB7L3)
        ctl->gamepad.buttons |= BUTTON_THUMB_L;
    if (b7 & kB7R3)
        ctl->gamepad.buttons |= BUTTON_THUMB_R;
    if (b7 & kB7View)
        ctl->gamepad.misc_buttons |= MISC_BUTTON_SELECT;
    if (b7 & kB7Menu)
        ctl->gamepad.misc_buttons |= MISC_BUTTON_START;

    /* byte 8 = R2 analog, byte 9 = L2 analog (OpenMicro capture). */
    const uint8_t rt = (len >= 9) ? report[8] : 0;
    const uint8_t lt = (len >= 10) ? report[9] : 0;
    ctl->gamepad.throttle = trigger_u8(rt);
    ctl->gamepad.brake = trigger_u8(lt);

    if ((b7 & kB7Lt) || ctl->gamepad.brake >= kTriggerDigitalThreshold)
        ctl->gamepad.buttons |= BUTTON_TRIGGER_L;
    if ((b7 & kB7Rt) || ctl->gamepad.throttle >= kTriggerDigitalThreshold)
        ctl->gamepad.buttons |= BUTTON_TRIGGER_R;

    apply_home(ctl, slot);
}
