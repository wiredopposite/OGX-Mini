#include <cstdint>

#include "tusb.h"
#include "class/hid/hid_device.h"
#include "device/usbd_pvt.h"

#include "USBDevice/DeviceManager.h"

const usbd_class_driver_t *usbd_app_driver_get_cb(uint8_t *driver_count) 
{
	*driver_count = 1;
	return DeviceManager::get_instance().get_driver()->get_class_driver();
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
	return DeviceManager::get_instance().get_driver()->get_report_cb(itf, report_id, report_type, buffer, reqlen);
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{
	DeviceManager::get_instance().get_driver()->set_report_cb(itf, report_id, report_type, buffer, bufsize);
	tud_hid_report(report_id, buffer, bufsize);
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
	return DeviceManager::get_instance().get_driver()->vendor_control_xfer_cb(rhport, stage, request);
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	return DeviceManager::get_instance().get_driver()->get_descriptor_string_cb(index, langid);
}

uint8_t const *tud_descriptor_device_cb() 
{
	return DeviceManager::get_instance().get_driver()->get_descriptor_device_cb();
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) 
{
	return DeviceManager::get_instance().get_driver()->get_hid_descriptor_report_cb(itf);
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) 
{
	return DeviceManager::get_instance().get_driver()->get_descriptor_configuration_cb(index);
}

uint8_t const* tud_descriptor_device_qualifier_cb() 
{
	return DeviceManager::get_instance().get_driver()->get_descriptor_device_qualifier_cb();
}