#ifndef _SWITCH_2_PRO_HOST_H_
#define _SWITCH_2_PRO_HOST_H_

#include <cstdint>

#include "USBHost/HostDriver/SwitchPro/SwitchPro.h"

/**
 * Nintendo Switch 2 Pro Controller (USB PID 0x2069): same bulk bring-up + HID init as SwitchProHost.
 * Decoding is **InReport only** (10-byte payload) for buttons/sticks. IMU bytes after the
 * 10-byte payload are parsed separately for PS3/PS4 motion output (no button coupling).
 */
class Switch2ProHost : public SwitchProHost
{
public:
    explicit Switch2ProHost(uint8_t idx)
        : SwitchProHost(idx) {}

    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;

#ifdef OGXM_SWITCH2_HID_RAW_LOG
private:
    static constexpr std::uint16_t kSwitch2RawLogCap = 64;
    std::uint8_t switch2_dbg_prev_[kSwitch2RawLogCap]{};
    std::uint16_t switch2_dbg_prev_len_{0};
    std::uint8_t switch2_dbg_prev_btn_[3]{};
    bool switch2_dbg_have_btn_{false};
#endif
};

#endif
