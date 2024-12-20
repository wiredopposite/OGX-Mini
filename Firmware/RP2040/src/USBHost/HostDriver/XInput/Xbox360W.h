#ifndef _XBOX360_WIRELESS_HOST_H_
#define _XBOX360_WIRELESS_HOST_H_

#include <cstdint>

#include "Descriptors/XInput.h"
#include "USBHost/HostDriver/HostDriver.h"

class Xbox360WHost : public HostDriver
{
public:
    Xbox360WHost(uint8_t idx)
        : HostDriver(idx) {}

    ~Xbox360WHost() override;
    
    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

    void connect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) override;
    void disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    uint32_t tid_chatpad_keepalive_{0};
    XInput::InReportWireless prev_in_report_;
};

#endif // _XBOX360_WIRELESS_HOST_H_