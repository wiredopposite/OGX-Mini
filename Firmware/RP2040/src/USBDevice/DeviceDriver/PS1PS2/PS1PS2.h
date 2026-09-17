#ifndef _D_PS1PS2_DRIVER_H_
#define _D_PS1PS2_DRIVER_H_

#include <cstdint>

#include "USBDevice/DeviceDriver/DeviceDriver.h"
#include "USBDevice/DeviceDriver/PS1PS2/psx_simulator.h"

class PS1PS2Device : public DeviceDriver
{
public:
    void initialize() override;
    void process(const uint8_t idx, Gamepad& gamepad) override;
    uint16_t get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t req_len) override;
    void set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t buffer_size) override;
    bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) override;
    const uint16_t* get_descriptor_string_cb(uint8_t index, uint16_t langid) override;
    const uint8_t* get_descriptor_device_cb() override;
    const uint8_t* get_hid_descriptor_report_cb(uint8_t itf) override;
    const uint8_t* get_descriptor_configuration_cb(uint8_t index) override;
    const uint8_t* get_descriptor_device_qualifier_cb() override;

    PSXInputState& get_psx_state() { return psx_report_; }

private:
    static void core1_restart_cb();

    PSXInputState psx_report_{};
};

#endif // _D_PS1PS2_DRIVER_H_
