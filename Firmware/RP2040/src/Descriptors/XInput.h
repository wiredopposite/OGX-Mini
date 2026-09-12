#ifndef _XINPUT_DESCRIPTORS_H_
#define _XINPUT_DESCRIPTORS_H_

#include <cstdint>
#include <cstring>

namespace XInput
{
	static constexpr size_t ENDPOINT_IN_SIZE = 20;
	static constexpr size_t ENDPOINT_OUT_SIZE = 32;

	namespace Chatpad
	{
		static constexpr uint8_t CODE_1 = 23 ;
		static constexpr uint8_t CODE_2 = 22 ;
		static constexpr uint8_t CODE_3 = 21 ;
		static constexpr uint8_t CODE_4 = 20 ;
		static constexpr uint8_t CODE_5 = 19 ;
		static constexpr uint8_t CODE_6 = 18 ;
		static constexpr uint8_t CODE_7 = 17 ;
		static constexpr uint8_t CODE_8 = 103; 
		static constexpr uint8_t CODE_9 = 102; 
		static constexpr uint8_t CODE_0 = 101; 

		static constexpr uint8_t CODE_Q = 39 ;
		static constexpr uint8_t CODE_W = 38 ;
		static constexpr uint8_t CODE_E = 37 ;
		static constexpr uint8_t CODE_R = 36 ;
		static constexpr uint8_t CODE_T = 35 ;
		static constexpr uint8_t CODE_Y = 34 ;
		static constexpr uint8_t CODE_U = 33 ;
		static constexpr uint8_t CODE_I = 118; 
		static constexpr uint8_t CODE_O = 117; 
		static constexpr uint8_t CODE_P = 100; 

		static constexpr uint8_t CODE_A = 55;
		static constexpr uint8_t CODE_S = 54;
		static constexpr uint8_t CODE_D = 53;
		static constexpr uint8_t CODE_F = 52;
		static constexpr uint8_t CODE_G = 51;
		static constexpr uint8_t CODE_H = 50;
		static constexpr uint8_t CODE_J = 49;
		static constexpr uint8_t CODE_K = 119;
		static constexpr uint8_t CODE_L = 114;
		static constexpr uint8_t CODE_COMMA = 98;

		static constexpr uint8_t CODE_Z = 70; 
		static constexpr uint8_t CODE_X = 69; 
		static constexpr uint8_t CODE_C = 68; 
		static constexpr uint8_t CODE_V = 67; 
		static constexpr uint8_t CODE_B = 66; 
		static constexpr uint8_t CODE_N = 65; 
		static constexpr uint8_t CODE_M = 82; 
		static constexpr uint8_t CODE_PERIOD = 83;
		static constexpr uint8_t CODE_ENTER  = 99;

		static constexpr uint8_t CODE_LEFT  = 85;
		static constexpr uint8_t CODE_SPACE = 84;
		static constexpr uint8_t CODE_RIGHT = 81;
		static constexpr uint8_t CODE_BACK  = 113;

		//Offset byte 25 
		static constexpr uint8_t CODE_SHIFT = 1; 
		static constexpr uint8_t CODE_GREEN = 2;
		static constexpr uint8_t CODE_ORANGE = 4; 
		static constexpr uint8_t CODE_MESSENGER = 8; 
	};

	namespace OutReportID
	{
		static constexpr uint8_t RUMBLE = 0x00;
		static constexpr uint8_t LED = 0x01;
	};

	namespace Buttons0
	{
		static constexpr uint8_t DPAD_UP    = (1U << 0);
		static constexpr uint8_t DPAD_DOWN  = (1U << 1);
		static constexpr uint8_t DPAD_LEFT  = (1U << 2);
		static constexpr uint8_t DPAD_RIGHT = (1U << 3);
		static constexpr uint8_t START = (1U << 4);
		static constexpr uint8_t BACK  = (1U << 5);
		static constexpr uint8_t L3    = (1U << 6);
		static constexpr uint8_t R3    = (1U << 7);
	};

	namespace Buttons1
	{
		static constexpr uint8_t LB    = (1U << 0);
		static constexpr uint8_t RB    = (1U << 1);
		static constexpr uint8_t HOME  = (1U << 2);
		static constexpr uint8_t A     = (1U << 4);
		static constexpr uint8_t B     = (1U << 5);
		static constexpr uint8_t X     = (1U << 6);
		static constexpr uint8_t Y     = (1U << 7);
	};

	#pragma pack(push, 1)
	struct InReport
	{
		uint8_t report_id;
		uint8_t report_size;
		uint8_t buttons[2];
		uint8_t trigger_l;
		uint8_t trigger_r;
		int16_t joystick_lx;
		int16_t joystick_ly;
		int16_t joystick_rx;
		int16_t joystick_ry;
		uint8_t reserved[6];

		InReport()
		{
			std::memset(this, 0, sizeof(InReport));
			report_size = sizeof(InReport);
		}
	};
	static_assert(sizeof(InReport) == 20, "XInput::InReport is misaligned");

	struct WiredChatpadReport
	{
		uint8_t report_id;
		uint8_t chatpad[3];

		WiredChatpadReport()
		{
			std::memset(this, 0, sizeof(WiredChatpadReport));
		}
	};
	static_assert(sizeof(WiredChatpadReport) == 4, "XInput::WiredChatpadReport is misaligned");

	struct InReportWireless
	{
		uint8_t command[4];
		uint8_t report_id;
		uint8_t report_size;
		uint8_t buttons[2];
		uint8_t trigger_l;
		uint8_t trigger_r;
		int16_t joystick_lx;
		int16_t joystick_ly;
		int16_t joystick_rx;
		int16_t joystick_ry; // 18
		uint8_t reserved[6];
		uint8_t chatpad_status;
		uint8_t chatpad[3];

		InReportWireless()
		{
			std::memset(this, 0, sizeof(InReportWireless));
			report_size = sizeof(InReportWireless);
		}
	};
	static_assert(sizeof(InReportWireless) == 28, "XInput::InReportWireless is misaligned");

	struct OutReport
	{
		uint8_t report_id;
		uint8_t report_size;
		uint8_t led;
		uint8_t rumble_l;
		uint8_t rumble_r;
		uint8_t reserved[3];

		OutReport()
		{
			std::memset(this, 0, sizeof(OutReport));
		}
	};
	static_assert(sizeof(OutReport) == 8, "XInput::OutReport is misaligned");
	#pragma pack(pop)

	// String descriptors — from joypad-os xinput_descriptors.h
	static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
	static const uint8_t STRING_MANUFACTURER[] = "\xa9Microsoft Corporation";
	static const uint8_t STRING_PRODUCT[]      = "Xbox 360 Controller";
	static const uint8_t STRING_VERSION[]      = "1.0";
	// XSM3 security string (iInterface=4), joypad-os: XINPUT_SECURITY_STRING
	static const uint8_t STRING_XSM3[]         = "Xbox Security Method 3, Version 1.00, \xa9 2005 Microsoft Corporation. All rights reserved.";

	static const uint8_t *DESC_STRING[] __attribute__((unused)) =
	{
		STRING_LANGUAGE,
		STRING_MANUFACTURER,
		STRING_PRODUCT,
		STRING_VERSION,
		STRING_XSM3,
	};

	// Device descriptor — from joypad-os (XINPUT_VID 0x045E, XINPUT_PID 0x028E, XINPUT_BCD_DEVICE 0x0114)
	static const uint8_t DESC_DEVICE[] =
	{
		0x12,       // bLength
		0x01,       // bDescriptorType (Device)
		0x00, 0x02, // bcdUSB 2.00
		0xFF,       // bDeviceClass
		0xFF,       // bDeviceSubClass
		0xFF,       // bDeviceProtocol
		0x40,       // bMaxPacketSize0 64
		0x5E, 0x04, // idVendor 0x045E (Microsoft)
		0x8E, 0x02, // idProduct 0x028E (Xbox 360 Controller)
		0x14, 0x01, // bcdDevice 0x0114 (v1.14)
		0x01,       // iManufacturer (String Index)
		0x02,       // iProduct (String Index)
		0x03,       // iSerialNumber (String Index)
		0x01,       // bNumConfigurations 1
	};

	// Configuration descriptor (153 bytes) — from joypad-os xinput_config_descriptor[]
	// https://github.com/joypad-ai/joypad-os src/usb/usbd/descriptors/xinput_descriptors.h
	static constexpr uint16_t CONFIG_TOTAL_LENGTH = 153;

	static const uint8_t DESC_CONFIGURATION[] =
	{
		// Configuration descriptor (9 bytes)
		0x09, 0x02,
		0x99, 0x00,  // wTotalLength 153 (LE)
		0x04,        // bNumInterfaces
		0x01,        // bConfigurationValue
		0x00,        // iConfiguration
		0xA0,        // bmAttributes (bus powered, remote wakeup)
		0xFA,        // bMaxPower (500mA)

		// ---- Interface 0: Gamepad (SubClass 0x5D, Protocol 0x01) ----
		0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x5D, 0x01, 0x00,
		0x11, 0x21, 0x00, 0x01, 0x01, 0x25, 0x81, 0x14,
		0x00, 0x00, 0x00, 0x00, 0x13, 0x02, 0x08, 0x00, 0x00,
		0x07, 0x05, 0x81, 0x03, 0x20, 0x00, 0x04,
		0x07, 0x05, 0x02, 0x03, 0x20, 0x00, 0x08,

		// ---- Interface 1: Audio (SubClass 0x5D, Protocol 0x03) ----
		0x09, 0x04, 0x01, 0x00, 0x04, 0xFF, 0x5D, 0x03, 0x00,
		0x1B, 0x21, 0x00, 0x01, 0x01, 0x01, 0x83, 0x40,
		0x01, 0x04, 0x20, 0x16, 0x85, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x16, 0x06, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x07, 0x05, 0x83, 0x03, 0x20, 0x00, 0x02,
		0x07, 0x05, 0x04, 0x03, 0x20, 0x00, 0x04,
		0x07, 0x05, 0x85, 0x03, 0x20, 0x00, 0x40,
		0x07, 0x05, 0x06, 0x03, 0x20, 0x00, 0x10,

		// ---- Interface 2: Plugin Module (SubClass 0x5D, Protocol 0x02) ----
		0x09, 0x04, 0x02, 0x00, 0x01, 0xFF, 0x5D, 0x02, 0x00,
		0x09, 0x21, 0x00, 0x01, 0x01, 0x22, 0x86, 0x03, 0x00,
		0x07, 0x05, 0x86, 0x03, 0x20, 0x00, 0x10,

		// ---- Interface 3: Security (SubClass 0xFD, Protocol 0x13), iInterface=4 ----
		0x09, 0x04, 0x03, 0x00, 0x00, 0xFF, 0xFD, 0x13, 0x04,
		0x06, 0x41, 0x00, 0x01, 0x01, 0x03,
	};
	static_assert(sizeof(DESC_CONFIGURATION) == CONFIG_TOTAL_LENGTH, "XInput config descriptor size");
};

#endif // _XINPUT_DESCRIPTORS_H_