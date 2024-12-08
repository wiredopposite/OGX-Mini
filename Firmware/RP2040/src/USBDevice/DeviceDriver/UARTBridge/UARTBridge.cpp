#include "USBDevice/DeviceDriver/UARTBridge/UARTBridge.h"
#include "USBDevice/DeviceDriver/UARTBridge/uart_bridge/uart_bridge.h"

void UARTBridgeDevice::initialize()
{
    class_driver_ = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "UART",
	#endif
		.init = cdcd_init,
		.reset = cdcd_reset,
		.open = cdcd_open,
		.control_xfer_cb = cdcd_control_xfer_cb,
		.xfer_cb = cdcd_xfer_cb,
		.sof = NULL
	};
}

void UARTBridgeDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    if (!uart_initialized_)
    {
        uart_initialized_ = true;
        uart_bridge_run();
    }
}

uint16_t UARTBridgeDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	return sizeof(buffer);
}

void UARTBridgeDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{

}

bool UARTBridgeDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * UARTBridgeDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	return uart_bridge_descriptor_string_cb(index, langid);
}

const uint8_t * UARTBridgeDevice::get_descriptor_device_cb() 
{
    return uart_bridge_descriptor_device_cb();
}

const uint8_t * UARTBridgeDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * UARTBridgeDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return uart_bridge_descriptor_configuration_cb(index);
}

const uint8_t * UARTBridgeDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}