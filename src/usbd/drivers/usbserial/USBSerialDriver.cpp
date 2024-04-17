#include "stdint.h"

#include <string.h>

#include <tusb.h>
#include "device/usbd_pvt.h"
#include "class/cdc/cdc_device.h"

#include "utilities/log.h"

#include "usbd/drivers/usbserial/USBSerialDriver.h"
#include "usbd/descriptors/USBSerialDescriptors.h"

void USBSerialDriver::initialize()
{
	#if CFG_TUSB_DEBUG >= 2
		.name = "UART",
	#endif
	class_driver = {
		.init = cdcd_init,
		.reset = cdcd_reset,
		.open = cdcd_open,
		.control_xfer_cb = cdcd_control_xfer_cb,
		.xfer_cb = cdcd_xfer_cb,
		.sof = NULL
	};
}

void USBSerialDriver::process(int idx, Gamepad * gamepad, uint8_t * outbuffer)
{
    uint32_t bytes_written = tud_cdc_write_str(get_log());

    if (bytes_written == strlen(get_log())) 
	{
		tud_cdc_write_flush();
        clear_log();
    };

	sleep_ms(10);
}

uint16_t USBSerialDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	return sizeof(buffer);
}

void USBSerialDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{}

bool USBSerialDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * USBSerialDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	(void) langid;
	size_t chr_count;

	switch ( index ) 
	{
		case STRID_LANGID:
			memcpy(&_desc_str[1], string_desc_arr[0], 2);
			chr_count = 1;
			break;

		case STRID_SERIAL:
			chr_count = board_usb_get_serial(_desc_str + 1, 32);
			break;

		default:
			// Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
			// https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

			if ( !(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) ) return NULL;

			const char *str = string_desc_arr[index];

			// Cap at max char
			chr_count = strlen(str);
			size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
			if ( chr_count > max_count ) chr_count = max_count;

			// Convert ASCII string into UTF-16
			for ( size_t i = 0; i < chr_count; i++ ) {
				_desc_str[1 + i] = str[i];
			}
			break;
	}

	// first byte is length (including header), second byte is string type
	_desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

	return _desc_str;
}

const uint8_t * USBSerialDriver::get_descriptor_device_cb() 
{
    return (uint8_t const *) &usbserial_device_descriptor;;
}

const uint8_t * USBSerialDriver::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * USBSerialDriver::get_descriptor_configuration_cb(uint8_t index) 
{
    return usbserial_configuration_descriptor;
}

const uint8_t * USBSerialDriver::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}

void USBSerialDriver::update_rumble(int idx, Gamepad * gamepad)
{
    
}