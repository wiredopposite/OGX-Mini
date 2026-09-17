#ifndef GAMESIR_CYCLONE2_HOST_H_
#define GAMESIR_CYCLONE2_HOST_H_

#include <cstdint>

#include "Descriptors/XInput.h"
#include "USBHost/HostDriver/GameSirCyclone2/Cyclone2Ds4Wired.h"
#include "USBHost/HostDriver/GameSirCyclone2/Cyclone2HidPassive.h"
#include "USBHost/HostDriver/GameSirCyclone2/Cyclone2SwitchWired.h"
#include "USBHost/HostDriver/HostDriver.h"

/**
 * Dedicated GameSir Cyclone 2 host — owns multiple personalities of the same pad.
 *
 * XInput (green) + Switch/NS (red) + DS4 (blue) + HID passive (yellow).
 * Switch NS reuses shared SwitchProHost as protocol engine (physical ownership stays Cyclone).
 * Yellow HID is enumerate + listen only (no application OUT — can wedge otherwise).
 */
class GameSirCyclone2Host : public HostDriver
{
public:
    enum class Personality : uint8_t {
        XInput = 0,
        SwitchNs,
        Ds4,
        HidPassive,
        Unknown,
    };

    explicit GameSirCyclone2Host(uint8_t idx)
        : HostDriver(idx), switch_wired_(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                    const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                        const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;
    void disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

    /** Known Cyclone 2 wired PC/XInput VID/PID pairs (not all GameSir). */
    static bool is_known_id(uint16_t vid, uint16_t pid);
    /** Yellow HID/Android personality (3537:0575). */
    static bool is_hid_passive_id(uint16_t vid, uint16_t pid);
    /** True if this host should claim the device (XInput ID, session, or Cyclone NS fingerprint). */
    static bool should_claim(uint16_t vid, uint16_t pid);
    /** Prefer this overload from HID mount — can read bcd/product before tuh_mount_cb. */
    static bool should_claim(uint8_t address, uint16_t vid, uint16_t pid);
    static Personality infer_personality(uint16_t vid, uint16_t pid);

private:
    void dump_device_banner(uint8_t address, uint8_t instance,
                            const uint8_t* report_desc, uint16_t desc_len);
    void dump_usb_tree(uint8_t address);
    void dump_hid_report_descriptor(const uint8_t* report_desc, uint16_t desc_len);
    void classify_and_log_interface(uint8_t address, uint8_t instance,
                                    const uint8_t* report_desc, uint16_t desc_len);
    void log_mode_status_banner(uint16_t vid, uint16_t pid);
    void log_raw_report(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len);
    void process_xinput_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                               const uint8_t* report, uint16_t len);
    void process_enhanced_report(uint8_t address, uint8_t instance,
                                 const uint8_t* report, uint16_t len);
    void maybe_start_vendor_heartbeat(uint8_t address, uint8_t instance,
                                      const uint8_t* report_desc, uint16_t desc_len);
    bool is_vendor_hid_descriptor(const uint8_t* report_desc, uint16_t desc_len);
    void log_input_source_summary_once();

    Personality personality_{Personality::Unknown};
    Cyclone2SwitchWired switch_wired_;
    Cyclone2Ds4Wired ds4_wired_{};
    Cyclone2HidPassive hid_passive_{};

    uint8_t address_{0};
    uint8_t instance_{0};
    uint8_t itf_num_{0xFF};
    bool is_hid_path_{false};
    bool is_boot_kb_mouse_{false};
    bool vendor_hid_candidate_{false};
    bool heartbeat_enabled_{false};
    bool heartbeat_announced_{false};
    bool enhanced_report_seen_{false};
    bool enhanced_report_changing_{false};
    bool enhanced_banner_printed_{false};
    bool input_source_summary_printed_{false};
    bool report_id_0f_present_{false};
    bool report_id_12_present_{false};
    uint32_t init_ms_{0};
    uint32_t next_heartbeat_ms_{0};
    uint32_t last_unchanged_log_ms_{0};
    uint16_t prev_len_{0};
    uint8_t prev_report_[64]{};
    XInput::InReport prev_xinput_report_{};
    uint16_t prev_12_len_{0};
    uint8_t prev_12_report_[64]{};
    uint32_t tid_heartbeat_{0};
};

#endif // GAMESIR_CYCLONE2_HOST_H_
