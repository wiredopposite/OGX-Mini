#include "USBDevice/DeviceDriver/Wii/Wii.h"

void WiiDevice::initialize()
{
    // No USB device (tud) in Wii mode on Pico W; BT is used for Wiimote.
    (void)class_driver_; // unused
}

void WiiDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    (void)idx;
    (void)gamepad;
    // Gamepad is filled by USB host on Core1. Optional: send to Wiimote BT stack here.
}

uint16_t WiiDevice::get_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t*, uint16_t)
{
    return 0;
}

void WiiDevice::set_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t const*, uint16_t) {}

bool WiiDevice::vendor_control_xfer_cb(uint8_t, uint8_t, tusb_control_request_t const*)
{
    return false;
}

const uint16_t* WiiDevice::get_descriptor_string_cb(uint8_t, uint16_t) { return nullptr; }
const uint8_t* WiiDevice::get_descriptor_device_cb() { return nullptr; }
const uint8_t* WiiDevice::get_hid_descriptor_report_cb(uint8_t) { return nullptr; }
const uint8_t* WiiDevice::get_descriptor_configuration_cb(uint8_t) { return nullptr; }
const uint8_t* WiiDevice::get_descriptor_device_qualifier_cb() { return nullptr; }
