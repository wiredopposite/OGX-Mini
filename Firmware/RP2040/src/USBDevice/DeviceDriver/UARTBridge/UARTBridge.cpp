#include "class/cdc/cdc_device.h"
#include "bsp/board_api.h"

#include "Descriptors/CDCDev.h"
#include "USBDevice/DeviceDriver/UARTBridge/UARTBridge.h"
#include "USBDevice/DeviceDriver/UARTBridge/uart_bridge/uart_bridge.h"

void UARTBridgeDevice::initialize()
{
    class_driver_ = 
    {
        .name = TUD_DRV_NAME("UART_BRIDGE"),
        .init = cdcd_init,
        .deinit = cdcd_deinit,
        .reset = cdcd_reset,
        .open = cdcd_open,
        .control_xfer_cb = cdcd_control_xfer_cb,
        .xfer_cb = cdcd_xfer_cb,
        .sof = NULL
    };
}

void UARTBridgeDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    if (!task_running_)
    {
        task_running_ = true;
        uart_bridge_run();
    }
}

uint16_t UARTBridgeDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	return reqlen;
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
    static uint16_t desc_str[32 + 1];
    size_t char_count = 0;

    switch(index)
    {
        case 0:
            std::memcpy(&desc_str[1], CDCDesc::DESC_STRING[0], 2);
            char_count = 1;
            break;

        case 3:
            char_count = board_usb_get_serial(&desc_str[1], 32);
            break;

        default:
            if (index >= sizeof(CDCDesc::DESC_STRING) / sizeof(CDCDesc::DESC_STRING[0]))
            {
                return nullptr;
            }
            const char *str = reinterpret_cast<const char *>(CDCDesc::DESC_STRING[index]);
            char_count = std::strlen(str);
            const size_t max_count = sizeof(desc_str) / sizeof(desc_str[0]) - 1;    
            if (char_count > max_count)
            {
                char_count = max_count;
            }

            for (size_t i = 0; i < char_count; i++)
            {
                desc_str[1 + i] = str[i];
            }
            break;
    }

    desc_str[0] = static_cast<uint16_t>((TUSB_DESC_STRING << 8) | (2 * char_count + 2));
    return desc_str;
}

const uint8_t * UARTBridgeDevice::get_descriptor_device_cb() 
{
	return reinterpret_cast<const uint8_t*>(&CDCDesc::DESC_DEVICE);
}

const uint8_t * UARTBridgeDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * UARTBridgeDevice::get_descriptor_configuration_cb(uint8_t index) 
{
	return CDCDesc::DESC_CONFIG;
}

const uint8_t * UARTBridgeDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}