#ifndef _XBOX_ONE_HOST_H_
#define _XBOX_ONE_HOST_H_

#include <array>
#include <cstdint>

#include "Descriptors/XboxOne.h"
#include "USBHost/HostDriver/HostDriver.h"

class XboxOneHost : public HostDriver
{
public:
    XboxOneHost(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    void emit_pad_in(Gamepad& gamepad, uint8_t guide_pressed);
    void cancel_guide_orphan();
    void schedule_guide_orphan_clear(Gamepad& gamepad);

    XboxOne::InReport prev_in_report_;
    std::array<uint8_t, 32> prev_arcade_report_{};
    uint16_t prev_arcade_len_{0};
    bool gip_arcade_stick_{false};
    uint8_t guide_pressed_{0};
    uint32_t last_guide_07_ms_{0};
    uint32_t guide_orphan_task_id_{0};
    uint8_t guide_orphan_gen_{0};
};

#endif // _XBOX_ONE_HOST_H_
