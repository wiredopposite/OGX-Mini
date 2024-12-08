#ifndef _XBOX_ONE_HOST_H_
#define _XBOX_ONE_HOST_H_

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
    XboxOne::InReport prev_in_report_;
};

#endif // _XBOX_ONE_HOST_H_