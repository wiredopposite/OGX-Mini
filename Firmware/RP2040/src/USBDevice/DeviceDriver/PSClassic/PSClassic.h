#ifndef _D_PSCLASSIC_DRIVER_H_
#define _D_PSCLASSIC_DRIVER_H_

#include <cstdint>

#include "tusb.h"

#include "Descriptors/PSClassic.h"
#include "USBDevice/DeviceDriver/DeviceDriver.h"

class PSClassicDevice : public DeviceDriver
{
public:
    void initialize() override;
    void process(const uint8_t idx, Gamepad& gamepad) override;
    uint16_t get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) override;
    void set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) override;
    bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) override;
    const uint16_t* get_descriptor_string_cb(uint8_t index, uint16_t langid) override;
    const uint8_t* get_descriptor_device_cb() override;
    const uint8_t* get_hid_descriptor_report_cb(uint8_t itf) ;
    const uint8_t* get_descriptor_configuration_cb(uint8_t index) override;
    const uint8_t* get_descriptor_device_qualifier_cb() override;

private:
    static constexpr int16_t JOY_POS_THRESHOLD = 10000;
    static constexpr int16_t JOY_NEG_THRESHOLD = -10000;
    static constexpr int16_t JOY_POS_45_THRESHOLD = JOY_POS_THRESHOLD * 2;
    static constexpr int16_t JOY_NEG_45_THRESHOLD = JOY_NEG_THRESHOLD * 2;

    PSClassic::InReport in_report_{0};

    inline bool meets_pos_threshold(int16_t joy_l, int16_t joy_r) { return (joy_l >= JOY_POS_THRESHOLD) || (joy_r >= JOY_POS_THRESHOLD); }
    inline bool meets_neg_threshold(int16_t joy_l, int16_t joy_r) { return (joy_l <= JOY_NEG_THRESHOLD) || (joy_r <= JOY_NEG_THRESHOLD); }
    inline bool meets_pos_45_threshold(int16_t joy_l, int16_t joy_r) { return (joy_l >= JOY_POS_45_THRESHOLD) || (joy_r >= JOY_POS_45_THRESHOLD); }
    inline bool meets_neg_45_threshold(int16_t joy_l, int16_t joy_r) { return (joy_l <= JOY_NEG_45_THRESHOLD) || (joy_r <= JOY_NEG_45_THRESHOLD); }
};

#endif // _D_PSCLASSIC_DRIVER_H_