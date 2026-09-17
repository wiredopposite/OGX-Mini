#ifndef FLYDIGI_APEX4_WUKONG_HOST_H_
#define FLYDIGI_APEX4_WUKONG_HOST_H_

#include <cstdint>

#include "USBHost/HostDriver/HostDriver.h"

/**
 * Debug-only probe for Flydigi APEX 4 Black Myth: Wukong Edition (wired USB / PC mode).
 *
 * Claims only VID:PID 04B4:2412 (Flydigi V1 composite). Does NOT claim 045E:028E.
 * Does not map input to PadIn yet — logs USB identity, GET_INFO, and raw reports.
 *
 * Bluetooth PC mode uses a separate candidate probe (`FlydigiApex4WukongBtProbe`) and
 * must not claim shared Xbox BT identity 045E:02E0.
 */
class FlydigiApex4WukongHost : public HostDriver
{
public:
    explicit FlydigiApex4WukongHost(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                    const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                        const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

    /** Log USB fingerprint for a generic Xbox 360 identity without claiming it. */
    static void log_xinput_candidate(uint8_t address, uint8_t instance);

private:
    void dump_device_banner(uint8_t address, uint8_t instance,
                            const uint8_t* report_desc, uint16_t desc_len);
    void dump_usb_tree(uint8_t address);
    void dump_hid_report_descriptor(const uint8_t* report_desc, uint16_t desc_len);
    void try_send_get_info(uint8_t address, uint8_t instance);
    void handle_get_info_reply(const uint8_t* report, uint16_t len);
    void log_raw_report(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len);

    uint8_t address_{0};
    uint8_t instance_{0};
    uint8_t itf_num_{0xFF};
    bool get_info_sent_{false};
    bool get_info_ok_{false};
    uint8_t get_info_attempts_{0};
    uint32_t next_get_info_ms_{0};
    uint32_t last_unchanged_log_ms_{0};
    uint16_t prev_len_{0};
    uint8_t prev_report_[64]{};
};

#endif // FLYDIGI_APEX4_WUKONG_HOST_H_
