#pragma once

#include <cstdint>

#include "Gamepad/Gamepad.h"

/**
 * Cyclone 2 yellow HID (3537:0575) — PASSIVE probe only.
 * Enumerate, dump interfaces, arm IN, log changing reports.
 * Never sends application OUT / feature / vendor commands (wedges some firmware).
 */
class Cyclone2HidPassive {
public:
    void reset();
    void start(uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len);
    void process_report(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len);
    /** Always no-op — safety mode blocks all controller OUT. */
    bool send_feedback(uint8_t address, uint8_t instance);
    void disconnect();

    static constexpr const char* mode_name() { return "HID_PASSIVE"; }

private:
    void dump_personality(uint8_t address, uint8_t instance,
                          const uint8_t* report_desc, uint16_t desc_len);
    void dump_hid_usage_hint(const uint8_t* report_desc, uint16_t desc_len);

    bool started_{false};
    bool banner_printed_{false};
    uint8_t address_{0};
    uint8_t instance_{0};
    uint8_t prev_raw_[64]{};
    uint16_t prev_raw_len_{0};
};
