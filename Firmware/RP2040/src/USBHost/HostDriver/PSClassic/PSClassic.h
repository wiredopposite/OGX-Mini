#ifndef _PSCLASSIC_HOST_H_
#define _PSCLASSIC_HOST_H_

#include <cstdint>

#include "Descriptors/PSClassic.h"
#include "USBHost/HostDriver/HostDriver.h"

class PSClassicHost : public HostDriver
{
public:
    PSClassicHost(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    PSClassic::InReport prev_in_report_{};
};

#endif // _PSCLASSIC_HOST_H_