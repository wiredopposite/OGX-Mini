#ifndef _DEVICE_DRIVER_H_
#define _DEVICE_DRIVER_H_

#include <cstdint>

#include "tusb.h"
#include "class/hid/hid.h"
#include "device/usbd_pvt.h"

#include "Gamepad.h"

class DeviceDriver
{
public:
    enum class Type : uint8_t
    {
        NONE = 0,
        PS3 = 1,
        DINPUT = 2,
        SWITCH = 4,
        PSCLASSIC = 5,
        XINPUT = 6,
        XBOXOG = 7,
        XBOXOG_SB = 8,
        XBOXOG_XR = 9,
        WEBAPP = 100,
        UART_BRIDGE = 101
    };

    virtual void initialize() = 0;
    virtual void process(const uint8_t idx, Gamepad& gamepad)  = 0;
    virtual uint16_t get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t req_len) = 0;
    virtual void set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t buffer_size) = 0;
    virtual bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) = 0;
    virtual const uint16_t* get_descriptor_string_cb(uint8_t index, uint16_t langid) = 0;
    virtual const uint8_t* get_descriptor_device_cb() = 0;
    virtual const uint8_t* get_hid_descriptor_report_cb(uint8_t itf) = 0;
    virtual const uint8_t* get_descriptor_configuration_cb(uint8_t index) = 0;
    virtual const uint8_t* get_descriptor_device_qualifier_cb() = 0;
    
    const usbd_class_driver_t* get_class_driver() { return &class_driver_; };

protected:
    usbd_class_driver_t class_driver_;

    uint16_t* get_string_descriptor(const char* value, uint8_t index);
};

#endif // _DEVICE_DRIVER_H_