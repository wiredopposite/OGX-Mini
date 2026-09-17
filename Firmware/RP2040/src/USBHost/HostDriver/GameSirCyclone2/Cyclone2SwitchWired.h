#pragma once

#include <cstdint>

#include "Gamepad/Gamepad.h"
#include "USBHost/HostDriver/SwitchPro/SwitchPro.h"

/**
 * Cyclone 2 red/NS Switch transport adapter.
 *
 * Physical ownership stays GameSirCyclone2Host. Protocol is the project's proven
 * SwitchProHost implementation (same engine genuine Switch Pro uses) — not a
 * separate handshake/parser reimplementation.
 */
class Cyclone2SwitchWired {
public:
    explicit Cyclone2SwitchWired(uint8_t idx)
        : engine_(idx) {}

    void reset();
    void start(Gamepad& gamepad, uint8_t address, uint8_t instance,
               const uint8_t* report_desc, uint16_t desc_len);
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                        const uint8_t* report, uint16_t len);
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance);
    void disconnect(Gamepad& gamepad, uint8_t address, uint8_t instance);

    static constexpr const char* protocol_engine_name() { return "SWITCH_PRO_STANDARD"; }
    static constexpr const char* parser_name() { return "SwitchProHost (shared via Cyclone2SwitchWired)"; }

private:
    SwitchProHost engine_;
    bool started_{false};
};
