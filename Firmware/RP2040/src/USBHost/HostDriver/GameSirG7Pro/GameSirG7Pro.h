#ifndef _GAMESIR_G7_PRO_HOST_H_
#define _GAMESIR_G7_PRO_HOST_H_

#include <cstdint>
#include <cstring>

#include "Descriptors/GameSirG7Pro.h"
#include "USBHost/HostDriver/HostDriver.h"

/** Wired HID GameSir G7 Pro (3537:1022). Dedicated mapping — do not fold into DInputHost. */
class GameSirG7ProHost : public HostDriver
{
public:
    explicit GameSirG7ProHost(uint8_t idx)
        : HostDriver(idx)
    {
        std::memset(prev_report_, 0, sizeof(prev_report_));
    }

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc,
                    uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report,
                        uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

    static bool is_known_id(uint16_t vid, uint16_t pid)
    {
        return vid == GameSirG7Pro::VID && pid == GameSirG7Pro::PID;
    }

private:
    uint8_t prev_report_[GameSirG7Pro::USB_REPORT_LEN]{};
    uint16_t prev_len_{0};
};

#endif // _GAMESIR_G7_PRO_HOST_H_
