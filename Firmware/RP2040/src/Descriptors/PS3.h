#ifndef _PS3_DESCRIPTORS_H_
#define _PS3_DESCRIPTORS_H_

#include <stdint.h>
#include <cstring>
#include <random>

namespace PS3 
{
	static constexpr uint8_t MAGIC_BYTES[8] = { 0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00 };
	static constexpr uint8_t JOYSTICK_MID = 0x7F;
	static constexpr uint16_t SIXAXIS_MID = 0xFF01;

	namespace ReportID 
	{
		static constexpr uint8_t FEATURE_01 = 0x01;
		static constexpr uint8_t FEATURE_EF = 0xEF;
		static constexpr uint8_t GET_PAIRING_INFO = 0xF2;
		static constexpr uint8_t FEATURE_F4 = 0xF4;
		static constexpr uint8_t FEATURE_F5 = 0xF5;
		static constexpr uint8_t FEATURE_F7 = 0xF7;
		static constexpr uint8_t FEATURE_F8 = 0xF8;
	};

	namespace PlugState
	{
		static constexpr uint8_t PLUGGED = 0x02;
		static constexpr uint8_t UNPLUGGED = 0x03;
	};

	namespace PowerState 
	{
		static constexpr uint8_t CHARGING = 0xEE;
		static constexpr uint8_t NOT_CHARGING = 0xF1;
		static constexpr uint8_t SHUTDOWN = 0x01;
		static constexpr uint8_t DISCHARGING = 0x02;
		static constexpr uint8_t LOW = 0x03;
		static constexpr uint8_t HIGH = 0x04;
		static constexpr uint8_t FULL = 0x05;
	};

	namespace RumbleState 
	{
		static constexpr uint8_t WIRED_RUMBLE = 0x10;
		static constexpr uint8_t WIRED_NO_RUMBLE = 0x12;
		static constexpr uint8_t WIRELESS_RUMBLE = 0x14;
		static constexpr uint8_t WIRELESS_NO_RUMBLE = 0x16;
	};

	namespace Buttons0
	{
		static constexpr uint8_t SELECT = 0x01;
		static constexpr uint8_t L3     = 0x02;
		static constexpr uint8_t R3     = 0x04;
		static constexpr uint8_t START  = 0x08;
		static constexpr uint8_t DPAD_UP     = 0x10;
		static constexpr uint8_t DPAD_RIGHT  = 0x20;
		static constexpr uint8_t DPAD_DOWN   = 0x40;
		static constexpr uint8_t DPAD_LEFT   = 0x80;
	};

	namespace Buttons1
	{
		static constexpr uint8_t L2       = 0x01;
		static constexpr uint8_t R2       = 0x02;
		static constexpr uint8_t L1       = 0x04;
		static constexpr uint8_t R1       = 0x08;
		static constexpr uint8_t TRIANGLE = 0x10;
		static constexpr uint8_t CIRCLE   = 0x20;
		static constexpr uint8_t CROSS    = 0x40;
		static constexpr uint8_t SQUARE   = 0x80;
	};

	namespace Buttons2
	{
		static constexpr uint8_t SYS = 0x01;
		static constexpr uint8_t TP = 0x02;
	};

	const uint8_t DEFAULT_OUT_REPORT[] =
	{
		0x01, 0xff, 0x00, 0xff, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	#pragma pack(push, 1)
	struct InReport
	{
		uint8_t report_id;
		uint8_t unk0;

		uint8_t buttons[3];
		uint8_t unk1;

		uint8_t joystick_lx;
		uint8_t joystick_ly;
		uint8_t joystick_rx;
		uint8_t joystick_ry;

		uint8_t unk2[2];
		uint8_t move_power_status;
		uint8_t unk3;

		uint8_t up_axis;
		uint8_t right_axis;
		uint8_t down_axis;
		uint8_t left_axis;

		uint8_t l2_axis;
		uint8_t r2_axis;
		uint8_t l1_axis;
		uint8_t r1_axis;

		uint8_t triangle_axis;
		uint8_t circle_axis;
		uint8_t cross_axis;
		uint8_t square_axis;

		uint8_t unk4[3];

		uint8_t plugged;
		uint8_t power_status;
		uint8_t rumble_status;

		uint8_t reserved5[9];

		uint16_t acceler_x;
		uint16_t acceler_y;
		uint16_t acceler_z;
		uint16_t gyro_z;

		InReport()
		{
			std::memset(this, 0, sizeof(InReport));
			report_id = 0x01;
			joystick_lx = JOYSTICK_MID;
			joystick_ly = JOYSTICK_MID;
			joystick_rx = JOYSTICK_MID;
			joystick_ry = JOYSTICK_MID;
			plugged = PlugState::PLUGGED;
			power_status = PowerState::FULL;
			rumble_status = RumbleState::WIRED_RUMBLE;
			acceler_x = acceler_y = acceler_z = gyro_z = SIXAXIS_MID;
		}
	};
	static_assert(sizeof(InReport) == 49, "PS3::InReport size mismatch");

	struct OutReport 
	{
		//uint8_t report_id;
		uint8_t reserved0;
		struct Rumble 
		{
			uint8_t right_duration;   /* Right motor duration (0xff means forever) */
			uint8_t right_motor_on;   /* Right (small) motor on/off, only supports values of 0 or 1 (off/on) */
			uint8_t left_duration;    /* Left motor duration (0xff means forever) */
			uint8_t left_motor_force; /* left (large) motor, supports force values from 0 to 255 */
		} rumble;
		uint8_t reserved1[4];
		uint8_t leds_bitmap;      /* bitmap of enabled LEDs: LED_1 = 0x02, LED_2 = 0x04, ... */
		struct LEDs 
		{
			uint8_t time_enabled; /* the total time the led is active (0xff means forever) */
			uint8_t duty_length;  /* how long a cycle is in deciseconds (0 means "really fast") */
			uint8_t enabled;
			uint8_t duty_off;     /* % of duty_length the led is off (0xff means 100%) */
			uint8_t duty_on;      /* % of duty_length the led is on (0xff mean 100%) */
		} leds[4];                /* LEDx at (4 - x) */
		struct LEDs unused;       /* LED5, not actually soldered */
		uint8_t reserved2[13];

		OutReport()
		{
			std::memset(this, 0, sizeof(OutReport));
			std::memcpy(this, DEFAULT_OUT_REPORT, sizeof(DEFAULT_OUT_REPORT));
		}
	};
	static_assert(sizeof(OutReport) == 48, "PS3::OutReport size mismatch");
	static_assert(sizeof(OutReport) >= sizeof(DEFAULT_OUT_REPORT));

	static constexpr uint8_t DEFAULT_BT_INFO_HEADER[] =
	{
		0xFF, 0xFF,
		0x00, 0x20, 0x40, 0xCE, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};

	struct BTInfo
	{
		uint8_t reserved0[2];
		uint8_t device_address[7]; // leading zero followed by address
		uint8_t host_address[7]; // leading zero followed by address
		uint8_t reserved1;

		BTInfo()
		{
			std::memcpy(device_address, DEFAULT_BT_INFO_HEADER, sizeof(DEFAULT_BT_INFO_HEADER));

            std::mt19937 gen(12345);
            std::uniform_int_distribution<uint8_t> dist(0, 0xFF);

			for (uint8_t i = 4; i < sizeof(device_address); i++) 
			{
				device_address[i] = dist(gen);
			}
			for (uint8_t i = 1; i < sizeof(host_address); i++) 
			{
				host_address[i] = dist(gen);
			}
		}
	};
	static_assert(sizeof(BTInfo) == 17, "PS3::BTInfo size mismatch");
	static_assert(sizeof(BTInfo) >= sizeof(DEFAULT_BT_INFO_HEADER));
	#pragma pack(pop)

	static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
	static const uint8_t STRING_MANUFACTURER[] = "Sony";
	static const uint8_t STRING_PRODUCT[]      = "PLAYSTATION(R)3 Controller";
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
		0x4C, 0x05,  // idVendor 0x054C
		0x68, 0x02,  // idProduct 0x0268
		0x00, 0x01,  // bcdDevice 2.00
		0x01,        // iManufacturer (String Index)
		0x02,        // iProduct (String Index)
		0x00,        // iSerialNumber (String Index)
		0x01,        // bNumConfigurations 1
	};

	static const uint8_t REPORT_DESCRIPTORS[] =
	{
		0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
		0x09, 0x04,        // Usage (Joystick)
		0xA1, 0x01,        // Collection (Physical)
		0xA1, 0x02,        //   Collection (Application)
		0x85, 0x01,        //     Report ID (1)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x01,        //     Report Count (1)
		0x15, 0x00,        //     Logical Minimum (0)
		0x26, 0xFF, 0x00,  //     Logical Maximum (255)
		0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
						//     NOTE: reserved byte
		0x75, 0x01,        //     Report Size (1)
		0x95, 0x13,        //     Report Count (19)
		0x15, 0x00,        //     Logical Minimum (0)
		0x25, 0x01,        //     Logical Maximum (1)
		0x35, 0x00,        //     Physical Minimum (0)
		0x45, 0x01,        //     Physical Maximum (1)
		0x05, 0x09,        //     Usage Page (Button)
		0x19, 0x01,        //     Usage Minimum (0x01)
		0x29, 0x13,        //     Usage Maximum (0x13)
		0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x75, 0x01,        //     Report Size (1)
		0x95, 0x0D,        //     Report Count (13)
		0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
		0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
						//     NOTE: 32 bit integer, where 0:18 are buttons and 19:31 are reserved
		0x15, 0x00,        //     Logical Minimum (0)
		0x26, 0xFF, 0x00,  //     Logical Maximum (255)
		0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
		0x09, 0x01,        //     Usage (Pointer)
		0xA1, 0x00,        //     Collection (Undefined)
		0x75, 0x08,        //       Report Size (8)
		0x95, 0x04,        //       Report Count (4)
		0x35, 0x00,        //       Physical Minimum (0)
		0x46, 0xFF, 0x00,  //       Physical Maximum (255)
		0x09, 0x30,        //       Usage (X)
		0x09, 0x31,        //       Usage (Y)
		0x09, 0x32,        //       Usage (Z)
		0x09, 0x35,        //       Usage (Rz)
		0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
						//       NOTE: four joysticks
		0xC0,              //     End Collection
		0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x27,        //     Report Count (39)
		0x09, 0x01,        //     Usage (Pointer)
		0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x30,        //     Report Count (48)
		0x09, 0x01,        //     Usage (Pointer)
		0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x30,        //     Report Count (48)
		0x09, 0x01,        //     Usage (Pointer)
		0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0xC0,              //   End Collection
		0xA1, 0x02,        //   Collection (Application)
		0x85, 0x02,        //     Report ID (2)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x30,        //     Report Count (48)
		0x09, 0x01,        //     Usage (Pointer)
		0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0xC0,              //   End Collection
		0xA1, 0x02,        //   Collection (Application)
		0x85, 0xEE,        //     Report ID (238)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x30,        //     Report Count (48)
		0x09, 0x01,        //     Usage (Pointer)
		0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0xC0,              //   End Collection
		0xA1, 0x02,        //   Collection (Application)
		0x85, 0xEF,        //     Report ID (239)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x30,        //     Report Count (48)
		0x09, 0x01,        //     Usage (Pointer)
		0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
		0xC0,              //   End Collection
		0xC0,              // End Collection
	};

	static const uint8_t CONFIGURATION_DESCRIPTORS[] =
	{
		0x09,        // bLength
		0x02,        // bDescriptorType (Configuration)
		0x29, 0x00,  // wTotalLength 41
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
		0x03,        // bInterfaceClass
		0x00,        // bInterfaceSubClass
		0x00,        // bInterfaceProtocol
		0x00,        // iInterface (String Index)

		0x09,        // bLength
		0x21,        // bDescriptorType (HID)
		0x11, 0x01,  // bcdHID 1.17
		0x00,        // bCountryCode
		0x01,        // bNumDescriptors
		0x22,        // bDescriptorType[0] (HID)
		0x94, 0x00,  // wDescriptorLength[0] 148

		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x02,        // bEndpointAddress (OUT/H2D)
		0x03,        // bmAttributes (Interrupt)
		0x40, 0x00,  // wMaxPacketSize 64
		0x01,        // bInterval 1 (unit depends on device speed)

		0x07,        // bLength
		0x05,        // bDescriptorType (Endpoint)
		0x81,        // bEndpointAddress (IN/D2H)
		0x03,        // bmAttributes (Interrupt)
		0x40, 0x00,  // wMaxPacketSize 64
		0x01,        // bInterval 1 (unit depends on device speed)
	};

	static constexpr uint8_t OUTPUT_0x01[] = 
	{
		0x01, 0x04, 0x00, 0x0b, 0x0c, 0x01, 0x02, 0x18, 
		0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11, 0x12,
		0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02,
		0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04, 0x04,
		0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01, 0x02,
		0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	// calibration data
	static constexpr uint8_t OUTPUT_0xEF[] = 
	{
		0xef, 0x04, 0x00, 0x0b, 0x03, 0x01, 0xa0, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff,
		0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
	};

	// unknown
	static constexpr uint8_t OUTPUT_0xF5[] = 
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // host address - must match 0xf2
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	// unknown
	static constexpr uint8_t OUTPUT_0xF7[] = 
	{
		0x02, 0x01, 0xf8, 0x02, 0xe2, 0x01, 0x05, 0xff,
		0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	// unknown
	static constexpr uint8_t OUTPUT_0xF8[] = 
	{
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
} // namespace PS3

#endif // _PS3_DESCRIPTORS_H_