#pragma once

#include <cstdint>

#include "Descriptors/PS4.h"
#include "Gamepad/Gamepad.h"

/**
 * DS4-compatible USB input for GameSir Cyclone 2 (blue).
 * Reuses PS4::InReport layout helpers; does not modify PS4Host.
 */
class Cyclone2Ds4Wired {
public:
    void reset();
    void start(uint8_t address, uint8_t instance, uint8_t player_idx);
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                        const uint8_t* report, uint16_t len);
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance);
    bool ready() const { return ready_; }

private:
    void send_lightbar(uint8_t address, uint8_t instance);
    bool decode_report(Gamepad& gamepad, const uint8_t* report, uint16_t len);
    void map_from_ps4_in(Gamepad& gamepad, const PS4::InReport& in);
    void log_pipeline(Gamepad& gamepad, const uint8_t* report, uint16_t len, uint8_t rid);

    bool ready_{false};
    bool lightbar_sent_{false};
    bool logged_banner_{false};
    uint8_t player_idx_{0};
    PS4::OutReport out_{};
    PS4::InReport prev_{};
    uint8_t prev_raw_[64]{};
    uint16_t prev_raw_len_{0};
    uint32_t last_stage_log_ms_{0};
};
