#ifndef _PS4_HOST_H_
#define _PS4_HOST_H_

#include <cstdint>

#include "Descriptors/PS4.h"
#include "USBHost/HostDriver/HostDriver.h"

class PS4Host : public HostDriver
{
public:
    PS4Host(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    PS4::InReport in_report_{};
    PS4::InReport prev_in_report_{};
    PS4::OutReport out_report_{};
};

#endif // _PS4_HOST_H_