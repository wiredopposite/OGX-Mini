#include <cstdint>
#include <cstdio>

#include "tusb.h"
#include "class/hid/hid_device.h"
#include "device/usbd_pvt.h"

#include "USBDevice/DeviceManager.h"
#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"

#if defined(CONFIG_EN_BLUETOOTH) && defined(CONFIG_TARGET_PICO_W)
#include "Bluepad32/Bluepad32.h"
#endif

const usbd_class_driver_t *usbd_app_driver_get_cb(uint8_t *driver_count) 
{
	DeviceDriverType dt = DeviceManager::get_instance().get_driver_type();
	if (dt == DeviceDriverType::PS1PS2 || dt == DeviceDriverType::GAMECUBE || dt == DeviceDriverType::DREAMCAST) {
		*driver_count = 0;
		return nullptr;
	}
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
	// Do not echo received output back as input report (e.g. Wii U GC adapter init 0x13 must only be received;
	// TinyUSB re-arms the OUT endpoint after this returns so the host can keep sending 0x13).
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
	printf("tud_vendor_cb bReq 0x%02x stage %u type 0x%02x wIdx 0x%04x\n",
		(unsigned)request->bRequest, (unsigned)stage, (unsigned)request->bmRequestType, (unsigned)request->wIndex);
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

#if defined(CONFIG_EN_BLUETOOTH) && defined(CONFIG_TARGET_PICO_W)
void tud_resume_cb(void) {
	bluepad32::on_usb_device_resume();
}
#endif