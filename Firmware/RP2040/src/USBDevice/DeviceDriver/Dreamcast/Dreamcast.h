/* Dreamcast Maple Bus output (device) – stub until DreamPicoPort Maple Bus HAL is ported.
 * See Firmware/RP2040/docs/Dreamcast_Port.md for port steps from https://github.com/OrangeFox86/DreamPicoPort */

#ifndef _D_DREAMCAST_DRIVER_H_
#define _D_DREAMCAST_DRIVER_H_

#include "USBDevice/DeviceDriver/DeviceDriver.h"

class DreamcastDevice : public DeviceDriver
{
public:
    void initialize() override;
    void process(const uint8_t idx, class Gamepad& gamepad) override;
    uint16_t get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t req_len) override;
    void set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t buffer_size) override;
    bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) override;
    const uint16_t* get_descriptor_string_cb(uint8_t index, uint16_t langid) override;
    const uint8_t* get_descriptor_device_cb() override;
    const uint8_t* get_hid_descriptor_report_cb(uint8_t itf) override;
    const uint8_t* get_descriptor_configuration_cb(uint8_t index) override;
    const uint8_t* get_descriptor_device_qualifier_cb() override;
};

/** Entry point for core1 when driver is DREAMCAST (Maple Bus device loop). */
void dreamcast_core1_entry(void);

/** Call before launching core1: true = run as Maple device (USB/BT -> DC); false = bus used as host only. */
void dreamcast_set_core1_device_mode(bool enable);

#endif
