#pragma once

#include "board_config.h"
#if (OGX_TYPE == WIRELESS) && (OGX_MCU == MCU_RP2040)

#include <stdint.h>
#include "tusb.h"

#define DESC_STR_MAX 20

#define USBD_VID 0x2E8A /* Raspberry Pi */
#define USBD_PID 0x000A /* Raspberry Pi Pico SDK CDC */

#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN * CFG_TUD_CDC)
#define USBD_MAX_POWER_MA 500

#define USBD_ITF_CDC_0 0
#define USBD_ITF_CDC_1 2
#define USBD_ITF_MAX 4

#define USBD_CDC_0_EP_CMD 0x81
#define USBD_CDC_1_EP_CMD 0x83

#define USBD_CDC_0_EP_OUT 0x01
#define USBD_CDC_1_EP_OUT 0x03

#define USBD_CDC_0_EP_IN 0x82
#define USBD_CDC_1_EP_IN 0x84

#define USBD_CDC_CMD_MAX_SIZE 8
#define USBD_CDC_IN_OUT_MAX_SIZE 64

#define USBD_STR_0 0x00
#define USBD_STR_MANUF 0x01
#define USBD_STR_PRODUCT 0x02
#define USBD_STR_SERIAL 0x03
#define USBD_STR_SERIAL_LEN 17
#define USBD_STR_CDC 0x04

static const uint8_t uart_string_language[] = {0x09, 0x04};
static const uint8_t uart_string_manufacturer[] = "Raspberry Pi";
static const uint8_t uart_string_product[] = "Pico";
static const uint8_t uart_string_version[] = "1.0";

static const uint8_t *uart_string_descriptors[] =
{
	uart_string_language,
	uart_string_manufacturer,
	uart_string_product,
	uart_string_version
};

static const tusb_desc_device_t uart_device_descriptor = {
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = TUSB_CLASS_MISC,
	.bDeviceSubClass = MISC_SUBCLASS_COMMON,
	.bDeviceProtocol = MISC_PROTOCOL_IAD,
	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
	.idVendor = USBD_VID,
	.idProduct = USBD_PID,
	.bcdDevice = 0x0100,
	.iManufacturer = USBD_STR_MANUF,
	.iProduct = USBD_STR_PRODUCT,
	.iSerialNumber = USBD_STR_SERIAL,
	.bNumConfigurations = 1,
};

static const uint8_t uart_configuration_descriptor[USBD_DESC_LEN] = {
	TUD_CONFIG_DESCRIPTOR(1, USBD_ITF_MAX, USBD_STR_0, USBD_DESC_LEN,
		TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, USBD_MAX_POWER_MA),

	TUD_CDC_DESCRIPTOR(USBD_ITF_CDC_0, USBD_STR_CDC, USBD_CDC_0_EP_CMD,
		USBD_CDC_CMD_MAX_SIZE, USBD_CDC_0_EP_OUT, USBD_CDC_0_EP_IN,
		USBD_CDC_IN_OUT_MAX_SIZE),

	// TUD_CDC_DESCRIPTOR(USBD_ITF_CDC_1, USBD_STR_CDC, USBD_CDC_1_EP_CMD,
	// 	USBD_CDC_CMD_MAX_SIZE, USBD_CDC_1_EP_OUT, USBD_CDC_1_EP_IN,
	// 	USBD_CDC_IN_OUT_MAX_SIZE),
};

#endif // (OGX_TYPE == WIRELESS) && (OGX_MCU == MCU_RP2040)