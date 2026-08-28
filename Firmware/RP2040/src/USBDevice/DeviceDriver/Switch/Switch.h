#ifndef _SWITCH_DEVICE_H_
#define _SWITCH_DEVICE_H_

#include <cstdint>
#include <array>
#include <cstring>

#include "USBDevice/DeviceDriver/DeviceDriver.h"
#include "Descriptors/SwitchProDevice.h"

class SwitchDevice : public DeviceDriver
{
public:
    void initialize() override;
    void process(const uint8_t idx, Gamepad& gamepad) override;
    uint16_t get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) override;
    void set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) override;
    bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) override;
    const uint16_t* get_descriptor_string_cb(uint8_t index, uint16_t langid) override;
    const uint8_t* get_descriptor_device_cb() override;
    const uint8_t* get_hid_descriptor_report_cb(uint8_t itf) override;
    const uint8_t* get_descriptor_configuration_cb(uint8_t index) override;
    const uint8_t* get_descriptor_device_qualifier_cb() override;

private:
    void gamepad_to_switch_report(const Gamepad::PadIn& gp_in, SwitchPro::SwitchReport& out, const Gamepad& gamepad);
    void build_standard_report(const SwitchPro::SwitchReport& sw);
    void build_subcommand_reply(const SwitchPro::SwitchReport& sw);
    void set_timer();
    /** Decode HD rumble from a console output report into rumble_l_/rumble_r_. */
    void parse_host_rumble(uint8_t report_id, uint8_t const* buffer, uint16_t bufsize);

    std::array<uint8_t, SwitchPro::REPORT_SIZE> report_{};
    SwitchPro::SwitchReport switch_report_{};
    std::array<uint8_t, SwitchPro::REPORT_SIZE> pending_output_{};
    bool has_pending_output_ = false;

    uint8_t timer_ = 0;
    uint32_t timestamp_ms_ = 0;
    uint8_t vibration_report_ = 0x00;
    uint8_t vibration_idx_ = 0;
    bool vibration_enabled_ = false;
    std::array<uint8_t, 6> addr_ = { 0x7C, 0xBB, 0x8A, 0x12, 0x34, 0x56 };

    // Latest rumble decoded from host 0x01/0x10/0x11 output; applied in process().
    uint8_t rumble_l_ = 0;
    uint8_t rumble_r_ = 0;
    bool rumble_dirty_ = false;

    // USB init handshake: reply to 0x80 0x01 (MAC), 0x80 0x02 (handshake), 0x80 0x04/0x05 (timeout)
    std::array<uint8_t, SwitchPro::USB_INIT_REPORT_SIZE> report_81_{};
    bool has_pending_81_ = false;
};

#endif
