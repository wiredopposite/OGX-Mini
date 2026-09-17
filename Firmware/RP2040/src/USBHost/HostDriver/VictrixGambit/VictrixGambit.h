#ifndef _VICTRIX_GAMBIT_HOST_H_
#define _VICTRIX_GAMBIT_HOST_H_

#include <array>
#include <cstdint>

#include "tusb.h"
#include "USBHost/HostDriver/HostDriver.h"

/**
 * Victrix Gambit Tournament Controller — wired USB only.
 *
 * Physically verified: 0E6F:0250
 * GIP discovery follows xone order: WAIT_ANNOUNCE → IDENTIFY → POWER/LED/AUTH.
 */
class VictrixGambitHost : public HostDriver
{
public:
    static constexpr uint16_t kVid = 0x0E6F;
    static constexpr uint16_t kPidPhysical = 0x0250;
    static constexpr uint16_t kPidDocumented = 0x02D6;
    static constexpr uint16_t kIdentifyBufMax = 1024;

    VictrixGambitHost(uint8_t idx)
        : HostDriver(idx) {}

    static bool is_known_id(uint16_t vid, uint16_t pid)
    {
        return vid == kVid && (pid == kPidPhysical || pid == kPidDocumented);
    }

    static void on_out_xfer_complete(uint8_t address, uint8_t instance, bool success,
                                     const uint8_t* data, uint16_t len);
    static void on_in_xfer_result(uint8_t address, uint8_t instance, bool success, uint16_t len);
    static void on_set_interface_complete(uint8_t daddr, bool success, xfer_result_t result,
                                          uintptr_t user_data);

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                    const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                        const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;
    void disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    enum class InitState : uint8_t
    {
        Idle = 0,
        DisableAudioInterface,
        WaitAudioInterfaceComplete,
        WaitAnnounce,
        SendIdentify,
        WaitIdentifyTxComplete,
        WaitIdentifyResponse,
        GamepadIdentified,
        SendPower,
        WaitPowerComplete,
        SendLed,
        WaitLedComplete,
        SendAuth,
        WaitAuthComplete,
        WaitForInput,
        Ready,
        InitFailed
    };

    enum class PendingCmd : uint8_t
    {
        None = 0,
        PowerOn,
        LedEnable,
        AuthDone,
        Identify
    };

    struct GipHdr
    {
        uint8_t command{0};
        uint8_t options{0};
        uint8_t sequence{0};
        uint32_t packet_length{0};
        uint32_t chunk_offset{0};
        uint8_t hdr_len{0};
    };

    void log_enumeration(uint8_t address, uint8_t instance);
    void set_state(InitState s);
    const char* state_name(InitState s) const;
    const char* cmd_name(PendingCmd c) const;
    bool arm_rx(const char* why, bool quiet = false);
    void begin_disable_audio_interface();
    void handle_set_interface_complete(bool success, xfer_result_t result);
    void enter_wait_announce();
    void schedule_retry_submit(uint32_t delay_ms);
    void schedule_status_tick();
    void log_perf();
    void try_submit_current();
    void handle_out_complete(bool success, const uint8_t* data, uint16_t len);
    void on_any_gip_rx(const uint8_t* report, uint16_t len);
    void maybe_log_rx_packet(const uint8_t* report, uint16_t len, const GipHdr* hdr);
    void log_announce_once(const uint8_t* report, uint16_t len, const GipHdr& hdr);
    void handle_announce(const uint8_t* report, uint16_t len, const GipHdr& hdr);
    bool decode_gip_header(const uint8_t* data, uint16_t len, GipHdr& out) const;
    void maybe_ack(const uint8_t* report, uint16_t len, const GipHdr& hdr, uint16_t remaining);
    void handle_identify_packet(const uint8_t* report, uint16_t len, const GipHdr& hdr);
    void on_identify_descriptor_complete();
    bool descriptor_has_gamepad_class(const uint8_t* data, uint16_t len) const;
    void begin_post_identify_bringup();
    void emit_pad_in(Gamepad& gamepad, uint8_t guide_pressed);
    void map_input(Gamepad& gamepad, const uint8_t* report, uint16_t len, uint8_t guide_pressed,
                   Gamepad::PadIn& gp_in);
    void cancel_guide_orphan();
    void schedule_guide_orphan_clear(Gamepad& gamepad);
    void log_input_change(const Gamepad::PadIn& gp_in);

    uint8_t address_{0};
    uint8_t instance_{0};
    uint16_t pid_{0};
    InitState state_{InitState::Idle};
    PendingCmd pending_cmd_{PendingCmd::None};
    uint8_t pending_seq_{0};
    uint16_t pending_len_{0};
    std::array<uint8_t, 16> pending_wire_{};

    uint32_t retry_task_id_{0};
    uint32_t status_task_id_{0};
    uint8_t init_gen_{0};
    uint8_t submit_retries_{0};

    uint32_t tx_requested_{0};
    uint32_t tx_completed_{0};
    uint32_t tx_failed_{0};
    uint32_t rx_arm_attempts_{0};
    uint32_t rx_arm_ok_{0};
    uint32_t rx_arm_fail_{0};
    uint32_t rx_complete_{0};
    uint32_t rx_packets_{0};
    uint8_t rx_log_remaining_{40};
    bool gip_traffic_seen_{false};
    bool controller_active_{false};
    bool identify_requested_{false};
    bool announce_logged_{false};
    bool identify_rx_logged_{false};
    uint32_t wait_announce_start_ms_{0};

    std::array<uint8_t, kIdentifyBufMax> identify_buf_{};
    uint16_t identify_total_{0};
    bool identify_assembling_{false};

    std::array<uint8_t, 64> prev_report_{};
    uint16_t prev_report_len_{0};
    uint8_t guide_pressed_{0};
    uint32_t last_guide_07_ms_{0};
    uint32_t guide_orphan_task_id_{0};
    uint8_t guide_orphan_gen_{0};

    uint16_t prev_buttons_{0};
    uint8_t prev_dpad_{0};
    bool enum_log_done_{false};
};

#endif // _VICTRIX_GAMBIT_HOST_H_
