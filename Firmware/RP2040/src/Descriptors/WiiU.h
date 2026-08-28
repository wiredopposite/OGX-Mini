#ifndef _WIIU_DESCRIPTORS_H_
#define _WIIU_DESCRIPTORS_H_

#include <stdint.h>

#include "tusb.h"

namespace WiiU
{
	// Wii U GameCube Adapter protocol (same as Dolphin / gc_adapter_read.py)
	static constexpr uint8_t HID_REPORT_ID = 0x21;
	static constexpr uint8_t INIT_PAYLOAD  = 0x13;
	static constexpr uint8_t CONTROLLER_PAYLOAD_SIZE = 37;  // report_id + 4 * 9 bytes per port

	// Controller type in first byte of each 9-byte port block
	static constexpr uint8_t PORT_WIRED    = (1 << 4);
	static constexpr uint8_t PORT_WIRELESS = (1 << 5);

	// GameCube button bits (bytes 1 and 2 of each port block)
	namespace GCButtons
	{
		// Byte 1
		static constexpr uint8_t A     = (1 << 0);
		static constexpr uint8_t B     = (1 << 1);
		static constexpr uint8_t X     = (1 << 2);
		static constexpr uint8_t Y     = (1 << 3);
		static constexpr uint8_t LEFT  = (1 << 4);
		static constexpr uint8_t RIGHT = (1 << 5);
		static constexpr uint8_t DOWN  = (1 << 6);
		static constexpr uint8_t UP    = (1 << 7);
		// Byte 2
		static constexpr uint8_t START = (1 << 0);
		static constexpr uint8_t Z     = (1 << 1);
		static constexpr uint8_t R     = (1 << 2);
		static constexpr uint8_t L     = (1 << 3);
	}

	static constexpr uint8_t STICK_CENTER = 128;

	#pragma pack(push, 1)
	// Full adapter input report: report ID 0x21 + 4 ports × 9 bytes
	struct InReport
	{
		uint8_t report_id{ HID_REPORT_ID };
		uint8_t port_data[4][9];  // [port][type, b1, b2, stickX, stickY, substickX, substickY, triggerL, triggerR]
	};
	static_assert(sizeof(InReport) == CONTROLLER_PAYLOAD_SIZE, "WiiU::InReport must be 37 bytes");
	#pragma pack(pop)

	static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
	static const uint8_t STRING_MANUFACTURER[] = "Nintendo";
	static const uint8_t STRING_PRODUCT[]      = "GameCube For Switch";
	static const uint8_t STRING_SERIAL[]       = "GH-GC-001 T8";
	static const uint8_t STRING_VERSION[]      = "1.0";

	static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) =
	{
		STRING_LANGUAGE,
		STRING_MANUFACTURER,
		STRING_PRODUCT,
		STRING_SERIAL,
		STRING_VERSION
	};

	// Nintendo Wii U GameCube Adapter
	static const uint8_t DEVICE_DESCRIPTORS[] =
	{
		0x12,        // bLength
		0x01,        // bDescriptorType (Device)
		0x00, 0x02,  // bcdUSB 2.00
		0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
		0x00,        // bDeviceSubClass
		0x00,        // bDeviceProtocol
		0x40,        // bMaxPacketSize0 64
		0x7E, 0x05,  // idVendor 0x057E (Nintendo)
		0x37, 0x03,  // idProduct 0x0337 (Wii U GameCube Adapter)
		0x00, 0x01,  // bcdDevice 1.00
		0x01,        // iManufacturer (String Index)
		0x02,        // iProduct (String Index)
		0x03,        // iSerialNumber (String Index)
		0x01,        // bNumConfigurations 1
	};

	// HID report descriptor (exact dump from real Nintendo Wii U GameCube Adapter 057e:0337)
	static const uint8_t REPORT_DESCRIPTORS[] =
	{
		0x05, 0x05, 0x09, 0x00, 0xA1, 0x01, 0x85, 0x11, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x15, 0x00, 0x26,
		0xFF, 0x00, 0x75, 0x08, 0x95, 0x05, 0x91, 0x00, 0xC0, 0xA1, 0x01, 0x85, 0x21, 0x19, 0x00, 0x2A,
		0xFF, 0x00, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x25, 0x81, 0x00, 0xC0, 0xA1, 0x01,
		0x85, 0x12, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x01,
		0x91, 0x00, 0xC0, 0xA1, 0x01, 0x85, 0x22, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x15, 0x00, 0x26, 0xFF,
		0x00, 0x75, 0x08, 0x95, 0x19, 0x81, 0x00, 0xC0, 0xA1, 0x01, 0x85, 0x13, 0x19, 0x00, 0x2A, 0xFF,
		0x00, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x01, 0x91, 0x00, 0xC0, 0xA1, 0x01, 0x85,
		0x23, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81,
		0x00, 0xC0, 0xA1, 0x01, 0x85, 0x14, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x15, 0x00, 0x26, 0xFF, 0x00,
		0x75, 0x08, 0x95, 0x01, 0x91, 0x00, 0xC0, 0xA1, 0x01, 0x85, 0x24, 0x19, 0x00, 0x2A, 0xFF, 0x00,
		0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x00, 0xC0, 0xA1, 0x01, 0x85, 0x15,
		0x19, 0x00, 0x2A, 0xFF, 0x00, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x01, 0x91, 0x00,
		0xC0, 0xA1, 0x01, 0x85, 0x25, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75,
		0x08, 0x95, 0x02, 0x81, 0x00, 0xC0,
	};
	static_assert(sizeof(REPORT_DESCRIPTORS) == 214, "WiiU report descriptor must be 214 bytes");

	// HID class descriptor (matches real adapter: bcdHID 1.10, report len 214)
	static const uint8_t HID_DESCRIPTORS[] =
	{
		0x09,        // bLength
		0x21,        // bDescriptorType (HID)
		0x10, 0x01,  // bcdHID 1.10
		0x00,        // bCountryCode
		0x01,        // bNumDescriptors
		0x22,        // bDescriptorType[0] (Report)
		0xD6, 0x00,  // wDescriptorLength 214
	};

	// Raw configuration descriptor (matches real adapter: IN 37 bytes, OUT 5 bytes, bInterval 8, bmAttributes 0xe0)
	static const uint8_t CONFIGURATION_DESCRIPTORS[] =
	{
		// Configuration descriptor
		0x09,        // bLength
		0x02,        // bDescriptorType (Configuration)
		0x29, 0x00,  // wTotalLength 41
		0x01,        // bNumInterfaces 1
		0x01,        // bConfigurationValue 1
		0x00,        // iConfiguration (String Index)
		0xE0,        // bmAttributes (Self Powered | Remote Wakeup)
		0xFA,        // bMaxPower 500mA (0xFA * 2mA)

		// Interface descriptor (HID)
		0x09,        // bLength
		0x04,        // bDescriptorType (Interface)
		0x00,        // bInterfaceNumber 0
		0x00,        // bAlternateSetting 0
		0x02,        // bNumEndpoints 2
		0x03,        // bInterfaceClass (HID)
		0x00,        // bInterfaceSubClass
		0x00,        // bInterfaceProtocol
		0x00,        // iInterface (String Index)

		// HID descriptor
		0x09,        // bLength
		0x21,        // bDescriptorType (HID)
		0x10, 0x01,  // bcdHID 1.10
		0x00,        // bCountryCode
		0x01,        // bNumDescriptors
		0x22,        // bDescriptorType[0] (Report)
		0xD6, 0x00,  // wDescriptorLength 214

		// Endpoint descriptor (OUT 0x02, 5 bytes, interval 8)
		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x02,        // bEndpointAddress (OUT)
		0x03,        // bmAttributes (Interrupt)
		0x05, 0x00,  // wMaxPacketSize 5
		0x08,        // bInterval 8

		// Endpoint descriptor (IN 0x81, 37 bytes, interval 8)
		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x81,        // bEndpointAddress (IN)
		0x03,        // bmAttributes (Interrupt)
		0x25, 0x00,  // wMaxPacketSize 37
		0x08,        // bInterval 8
	};

}; // namespace WiiU

#endif // _WIIU_DESCRIPTORS_H_
