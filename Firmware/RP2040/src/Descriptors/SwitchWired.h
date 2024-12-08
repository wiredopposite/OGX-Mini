#ifndef _SWITCH_WIRED_DESCRIPTORS_H_
#define _SWITCH_WIRED_DESCRIPTORS_H_

#include <stdint.h>

#include "tusb.h"

namespace SwitchWired
{
	static constexpr uint8_t JOYSTICK_MID = 0x80;

	namespace DPad
	{
		static constexpr uint8_t UP         = 0x00;
		static constexpr uint8_t UP_RIGHT   = 0x01;
		static constexpr uint8_t RIGHT      = 0x02;
		static constexpr uint8_t DOWN_RIGHT = 0x03;
		static constexpr uint8_t DOWN       = 0x04;
		static constexpr uint8_t DOWN_LEFT  = 0x05;
		static constexpr uint8_t LEFT       = 0x06;
		static constexpr uint8_t UP_LEFT    = 0x07;
		static constexpr uint8_t CENTER     = 0x08;
	};

	namespace Buttons
	{
		static constexpr uint16_t Y       = (1U <<  0);
		static constexpr uint16_t B       = (1U <<  1);
		static constexpr uint16_t A       = (1U <<  2);
		static constexpr uint16_t X       = (1U <<  3);
		static constexpr uint16_t L       = (1U <<  4);
		static constexpr uint16_t R       = (1U <<  5);
		static constexpr uint16_t ZL      = (1U <<  6);
		static constexpr uint16_t ZR      = (1U <<  7);
		static constexpr uint16_t MINUS   = (1U <<  8);
		static constexpr uint16_t PLUS    = (1U <<  9);
		static constexpr uint16_t L3      = (1U << 10);
		static constexpr uint16_t R3      = (1U << 11);
		static constexpr uint16_t HOME    = (1U << 12);
		static constexpr uint16_t CAPTURE = (1U << 13);
	};

	#pragma pack(push, 1)
	struct InReport
	{
		uint16_t buttons{0};
		uint8_t dpad{DPad::CENTER};
		uint8_t joystick_lx{JOYSTICK_MID};
		uint8_t joystick_ly{JOYSTICK_MID};
		uint8_t joystick_rx{JOYSTICK_MID};
		uint8_t joystick_ry{JOYSTICK_MID};
		uint8_t vendor{0};
	};
	static_assert(sizeof(InReport) == 8, "SwitchWired::InReport is not the correct size");
	#pragma pack(pop)

	static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
	static const uint8_t STRING_MANUFACTURER[] = "HORI CO.,LTD.";
	static const uint8_t STRING_PRODUCT[]      = "POKKEN CONTROLLER";
	static const uint8_t STRING_VERSION[]      = "1.0";

	static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) =
	{
		STRING_LANGUAGE,
		STRING_MANUFACTURER,
		STRING_PRODUCT,
		STRING_VERSION
	};

	static const uint8_t DEVICE_DESCRIPTORS[] =
	{
		0x12,        // bLength
		0x01,        // bDescriptorType (Device)
		0x00, 0x02,  // bcdUSB 2.00
		0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
		0x00,        // bDeviceSubClass
		0x00,        // bDeviceProtocol
		0x40,        // bMaxPacketSize0 64
		0x0D, 0x0F,  // idVendor 0x0F0D
		0x92, 0x00,  // idProduct 0x92
		0x00, 0x01,  // bcdDevice 2.00
		0x01,        // iManufacturer (String Index)
		0x02,        // iProduct (String Index)
		0x00,        // iSerialNumber (String Index)
		0x01,        // bNumConfigurations 1
	};

	static const uint8_t HID_DESCRIPTORS[] =
	{
		0x09,        // bLength
		0x21,        // bDescriptorType (HID)
		0x11, 0x01,  // bcdHID 1.11
		0x00,        // bCountryCode
		0x01,        // bNumDescriptors
		0x22,        // bDescriptorType[0] (HID)
		0x56, 0x00,  // wDescriptorLength[0] 86
	};

	static const uint8_t REPORT_DESCRIPTORS[] =
	{
		0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
		0x09, 0x05,        // Usage (Game Pad)
		0xA1, 0x01,        // Collection (Application)
		0x15, 0x00,        //   Logical Minimum (0)
		0x25, 0x01,        //   Logical Maximum (1)
		0x35, 0x00,        //   Physical Minimum (0)
		0x45, 0x01,        //   Physical Maximum (1)
		0x75, 0x01,        //   Report Size (1)
		0x95, 0x10,        //   Report Count (16)
		0x05, 0x09,        //   Usage Page (Button)
		0x19, 0x01,        //   Usage Minimum (0x01)
		0x29, 0x10,        //   Usage Maximum (0x10)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
		0x25, 0x07,        //   Logical Maximum (7)
		0x46, 0x3B, 0x01,  //   Physical Maximum (315)
		0x75, 0x04,        //   Report Size (4)
		0x95, 0x01,        //   Report Count (1)
		0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
		0x09, 0x39,        //   Usage (Hat switch)
		0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
		0x65, 0x00,        //   Unit (None)
		0x95, 0x01,        //   Report Count (1)
		0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x26, 0xFF, 0x00,  //   Logical Maximum (255)
		0x46, 0xFF, 0x00,  //   Physical Maximum (255)
		0x09, 0x30,        //   Usage (X)
		0x09, 0x31,        //   Usage (Y)
		0x09, 0x32,        //   Usage (Z)
		0x09, 0x35,        //   Usage (Rz)
		0x75, 0x08,        //   Report Size (8)
		0x95, 0x04,        //   Report Count (4)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
		0x09, 0x20,        //   Usage (0x20)
		0x95, 0x01,        //   Report Count (1)
		0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x0A, 0x21, 0x26,  //   Usage (0x2621)
		0x95, 0x08,        //   Report Count (8)
		0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0xC0,              // End Collection
	};

	// static const uint8_t CONFIGURATION_DESCRIPTORS[] =
	// {
	// 	0x09,        // bLength
	// 	0x02,        // bDescriptorType (Configuration)
	// 	0x29, 0x00,  // wTotalLength 41
	// 	0x01,        // bNumInterfaces 1
	// 	0x01,        // bConfigurationValue
	// 	0x00,        // iConfiguration (String Index)
	// 	0x80,        // bmAttributes
	// 	0xFA,        // bMaxPower 500mA

	// 	0x09,        // bLength
	// 	0x04,        // bDescriptorType (Interface)
	// 	0x00,        // bInterfaceNumber 0
	// 	0x00,        // bAlternateSetting
	// 	0x02,        // bNumEndpoints 2
	// 	0x03,        // bInterfaceClass
	// 	0x00,        // bInterfaceSubClass
	// 	0x00,        // bInterfaceProtocol
	// 	0x00,        // iInterface (String Index)

	// 	0x09,        // bLength
	// 	0x21,        // bDescriptorType (HID)
	// 	0x11, 0x01,  // bcdHID 1.11
	// 	0x00,        // bCountryCode
	// 	0x01,        // bNumDescriptors
	// 	0x22,        // bDescriptorType[0] (HID)
	// 	0x56, 0x00,  // wDescriptorLength[0] 86

	// 	0x07,        // bLength
	// 	0x05,        // bDescriptorType (Endpoint)
	// 	0x02,        // bEndpointAddress (OUT/H2D)
	// 	0x03,        // bmAttributes (Interrupt)
	// 	0x40, 0x00,  // wMaxPacketSize 64
	// 	0x01,        // bInterval 1 (unit depends on device speed)

	// 	0x07,        // bLength
	// 	0x05,        // bDescriptorType (Endpoint)
	// 	0x81,        // bEndpointAddress (IN/D2H)
	// 	0x03,        // bmAttributes (Interrupt)
	// 	0x40, 0x00,  // wMaxPacketSize 64
	// 	0x01,        // bInterval 1 (unit depends on device speed)
	// };
	
    enum 
    { 
        ITF_NUM_HID1, 
#if MAX_GAMEPADS > 1
        ITF_NUM_HID2,
#endif
#if MAX_GAMEPADS > 2 
        ITF_NUM_HID3,
#endif
#if MAX_GAMEPADS > 3 
        ITF_NUM_HID4,
#endif 
        ITF_NUM_TOTAL 
    };

    #define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + (TUD_HID_DESC_LEN * MAX_GAMEPADS))

    #define EPNUM_HID1 0x81
    #define EPNUM_HID2 0x82
    #define EPNUM_HID3 0x83
    #define EPNUM_HID4 0x84

    uint8_t const CONFIGURATION_DESCRIPTORS[] = {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(  1,
                                ITF_NUM_TOTAL,
                                0,
                                CONFIG_TOTAL_LEN,
                                TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                                500),

        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_DESCRIPTOR( ITF_NUM_HID1,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            EPNUM_HID1,
                            CFG_TUD_HID_EP_BUFSIZE,
                            1),
#if MAX_GAMEPADS > 1
        TUD_HID_DESCRIPTOR( ITF_NUM_HID2,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            EPNUM_HID2,
                            CFG_TUD_HID_EP_BUFSIZE,
                            1),
#endif
#if MAX_GAMEPADS > 2
        TUD_HID_DESCRIPTOR( ITF_NUM_HID3,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            EPNUM_HID3,
                            CFG_TUD_HID_EP_BUFSIZE,
                            1),
#endif
#if MAX_GAMEPADS > 3
        TUD_HID_DESCRIPTOR( ITF_NUM_HID4,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            EPNUM_HID4,
                            CFG_TUD_HID_EP_BUFSIZE,
                            1)
#endif
    };

}; // namespace SwitchWired

#endif // _SWITCH_WIRED_DESCRIPTORS_H_	