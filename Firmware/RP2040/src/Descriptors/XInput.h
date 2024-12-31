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

	static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
	static const uint8_t STRING_MANUFACTURER[] = "Microsoft";
	static const uint8_t STRING_PRODUCT[]      = "XInput STANDARD GAMEPAD";
	static const uint8_t STRING_VERSION[]      = "1.0";

	static const uint8_t *DESC_STRING[] __attribute__((unused)) =
	{
		STRING_LANGUAGE,
		STRING_MANUFACTURER,
		STRING_PRODUCT,
		STRING_VERSION
	};

	static const uint8_t DESC_DEVICE[] =
	{
		0x12,       // bLength
		0x01,       // bDescriptorType (Device)
		0x00, 0x02, // bcdUSB 2.00
		0xFF,	      // bDeviceClass
		0xFF,	      // bDeviceSubClass
		0xFF,	      // bDeviceProtocol
		0x40,	      // bMaxPacketSize0 64
		0x5E, 0x04, // idVendor 0x045E
		0x8E, 0x02, // idProduct 0x028E
		0x14, 0x01, // bcdDevice 2.14
		0x01,       // iManufacturer (String Index)
		0x02,       // iProduct (String Index)
		0x03,       // iSerialNumber (String Index)
		0x01,       // bNumConfigurations 1
	};

	static const uint8_t DESC_CONFIGURATION[] =
	{
		0x09,        // bLength
		0x02,        // bDescriptorType (Configuration)
		0x30, 0x00,  // wTotalLength 48
		0x01,        // bNumInterfaces 1
		0x01,        // bConfigurationValue
		0x00,        // iConfiguration (String Index)
		0x80,        // bmAttributes
		0xFA,        // bMaxPower 500mA

		0x09,        // bLength
		0x04,        // bDescriptorType (Interface)
		0x00,        // bInterfaceNumber 0
		0x00,        // bAlternateSetting
		0x02,        // bNumEndpoints 2
		0xFF,        // bInterfaceClass
		0x5D,        // bInterfaceSubClass
		0x01,        // bInterfaceProtocol
		0x00,        // iInterface (String Index)

		0x10,        // bLength
		0x21,        // bDescriptorType (HID)
		// 0x10, 0x01,  // bcdHID 1.10
		0x00, 0x01,  // bcdHID 1.00
		0x01,        // bCountryCode
		0x24,        // bNumDescriptors
		0x81,        // bDescriptorType[0] (Unknown 0x81)
		0x14, 0x03,  // wDescriptorLength[0] 788
		0x00,        // bDescriptorType[1] (Unknown 0x00)
		0x03, 0x13,  // wDescriptorLength[1] 4867
		0x01,        // bDescriptorType[2] (Unknown 0x02)
		0x00, 0x03,  // wDescriptorLength[2] 768
		0x00,        // bDescriptorType[3] (Unknown 0x00)

		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x81,        // bEndpointAddress (IN/D2H)
		0x03,        // bmAttributes (Interrupt)
		0x20, 0x00,  // wMaxPacketSize 32
		0x01,        // bInterval 1 (unit depends on device speed)

		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x01,        // bEndpointAddress (OUT/H2D)
		0x03,        // bmAttributes (Interrupt)
		0x20, 0x00,  // wMaxPacketSize 32
		0x08,        // bInterval 8 (unit depends on device speed)
	};
};

#endif // _XINPUT_DESCRIPTORS_H_