#include "stdint.h"
#include <hardware/irq.h>
#include <hardware/structs/sio.h>
#include <hardware/uart.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <string.h>

#include <hardware/flash.h>
#include <tusb.h>
#include "device/usbd_pvt.h"
#include "class/cdc/cdc_device.h"

#include "usbd/drivers/uartbridge/UARTBridgeDriver.h"
#include "usbd/drivers/uartbridge/uart_bridge_task.h"
#include "usbd/descriptors/UARTBridgeDescriptors.h"

void UARTBridgeDriver::initialize()
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

void UARTBridgeDriver::process(int idx, Gamepad * gamepad, uint8_t * outBuffer) 
{
	for (itf = 0; itf < CFG_TUD_CDC; itf++) 
	{
		update_uart_cfg(itf);
		uart_write_bytes(itf);
	}
}

uint16_t UARTBridgeDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	return sizeof(buffer);
}

void UARTBridgeDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{
}

bool UARTBridgeDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * UARTBridgeDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	static char usbd_serial[USBD_STR_SERIAL_LEN] = "000000000000";

	static const char *const usbd_desc_str[] = {
		nullptr, // Placeholder for index 0, which is not used directly
		"Raspberry Pi", // USBD_STR_MANUF
		"Pico", // USBD_STR_PRODUCT
		usbd_serial, // USBD_STR_SERIAL
		"Board CDC" // USBD_STR_CDC
	};
	
	static uint16_t desc_str[DESC_STR_MAX];
	uint8_t len;

	if (index == 0) {
		desc_str[1] = 0x0409;
		len = 1;
	} else {
		const char *str;
		char serial[USBD_STR_SERIAL_LEN];

		if (index >= sizeof(usbd_desc_str) / sizeof(usbd_desc_str[0]))
			return NULL;

		str = usbd_desc_str[index];
		for (len = 0; len < DESC_STR_MAX - 1 && str[len]; ++len)
			desc_str[1 + len] = str[len];
	}

	desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);

	return desc_str;
}

const uint8_t * UARTBridgeDriver::get_descriptor_device_cb() {
    return (const uint8_t *) &uart_device_descriptor;
}

const uint8_t * UARTBridgeDriver::get_hid_descriptor_report_cb(uint8_t itf) {
    return nullptr;
}

const uint8_t * UARTBridgeDriver::get_descriptor_configuration_cb(uint8_t index) {
    return uart_configuration_descriptor;
}

const uint8_t * UARTBridgeDriver::get_descriptor_device_qualifier_cb() {
	return nullptr;
}

void UARTBridgeDriver::update_rumble(int idx, Gamepad * gamepad) {}