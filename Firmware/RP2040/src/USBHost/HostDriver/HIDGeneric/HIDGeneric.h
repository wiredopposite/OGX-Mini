#ifndef _HID_GENERIC_HOST_H_
#define _HID_GENERIC_HOST_H_

#include <cstdint>
#include <array>
#include <memory>

#include "tusb_option.h"

#include "USBHost/HIDParser/HIDJoystick.h"
#include "USBHost/HostDriver/HostDriver.h"

class HIDHost : public HostDriver
{
public:
    HIDHost(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, uint8_t const* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    std::array<uint8_t, 0x100> report_desc_buffer_;
    uint16_t report_desc_len_{0};
    std::array<uint8_t, CFG_TUH_HID_EPIN_BUFSIZE> prev_report_in_{0};
    std::unique_ptr<HIDJoystick> hid_joystick_;
    HIDJoystickData hid_joystick_data_;
};

#endif // _HID_GENERIC_HOST_H_