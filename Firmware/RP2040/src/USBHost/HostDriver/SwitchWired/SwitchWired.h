#ifndef _SWITCH_WIRED_HOST_H_
#define _SWITCH_WIRED_HOST_H_

#include <cstdint>

#include "Descriptors/SwitchWired.h"
#include "USBHost/HostDriver/HostDriver.h"

class SwitchWiredHost : public HostDriver
{
public:
    SwitchWiredHost(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    SwitchWired::InReport prev_in_report_{};
};

#endif // _SWITCH_WIRED_HOST_H_