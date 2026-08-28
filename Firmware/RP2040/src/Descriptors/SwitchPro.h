#ifndef _SWITCH_PRO_DESCRIPTORS_H_
#define _SWITCH_PRO_DESCRIPTORS_H_

#include <stdint.h>

/**
 * Shared Switch Pro Controller types for both USB Host (parsing a real Pro)
 * and USB Device (emulating a Pro). Host uses InReport, OutReport, CMD, Buttons*.
 * Device uses the same InReport (as SwitchReport), Btn, HAT_*, and descriptors from SwitchProDevice.h.
 */
namespace SwitchPro
{
#pragma pack(push, 1)
	struct InReport
	{
		uint8_t batteryConnection;
		uint8_t buttons[3];
		union
		{
			uint8_t joysticks[6];
			struct { uint8_t l[3]; uint8_t r[3]; };
		};
	};
	static_assert(sizeof(InReport) == 10, "InReport must be 10 bytes");

		struct OutReport
		{
			uint8_t command;
			uint8_t sequence_counter;
			uint8_t rumble_l[4];
			uint8_t rumble_r[4];
			uint8_t sub_command;
			uint8_t sub_command_args[5];
		};
#pragma pack(pop)

	namespace CMD
	{
		static constexpr uint8_t RUMBLE_ONLY       = 0x10;
		static constexpr uint8_t HID               = 0x11;
		static constexpr uint8_t HANDSHAKE        = 0x00;
		static constexpr uint8_t DISABLE_TIMEOUT  = 0x01;
		static constexpr uint8_t AND_RUMBLE       = 0x12;
		static constexpr uint8_t LED              = 0x30;
		static constexpr uint8_t LED_HOME         = 0x38;
		static constexpr uint8_t MODE             = 0x03;
		static constexpr uint8_t FULL_REPORT_MODE = 0x30;
		static constexpr uint8_t GYRO             = 0x40;
		/** Unrecognized subcommand; sent after 0x80 0x04 so init gets a 0x21 ack. */
		static constexpr uint8_t USB_PROBE        = 0x33;
		/** Wired USB output report ID and subcommands (not Bluetooth 0x11 path). */
		static constexpr uint8_t USB_OUT              = 0x80;
		static constexpr uint8_t USB_SUB_MAC            = 0x01;
		static constexpr uint8_t USB_SUB_HANDSHAKE      = 0x02;
		static constexpr uint8_t USB_SUB_BAUD           = 0x03;
		static constexpr uint8_t USB_SUB_DISABLE_TIMEOUT = 0x04;
	}

	// First report byte (buttons_right): bit layout per real hardware / Bluepad32 parse_report_30_pro_controller
	namespace Buttons0
	{
		static constexpr uint8_t Y  = (1U << 0);
		static constexpr uint8_t X  = (1U << 1);
		static constexpr uint8_t B  = (1U << 2);
		static constexpr uint8_t A  = (1U << 3);
		static constexpr uint8_t R  = (1U << 6);
		static constexpr uint8_t ZR = (1U << 7);
	}
	namespace Buttons1
	{
		static constexpr uint8_t MINUS   = (1U << 0);
		static constexpr uint8_t PLUS    = (1U << 1);
		static constexpr uint8_t L3      = (1U << 2);
		static constexpr uint8_t R3      = (1U << 3);
		static constexpr uint8_t HOME    = (1U << 4);
		static constexpr uint8_t CAPTURE = (1U << 5);
	}
	namespace Buttons2
	{
		static constexpr uint8_t DPAD_UP    = (1U << 0);
		static constexpr uint8_t DPAD_DOWN  = (1U << 1);
		static constexpr uint8_t DPAD_LEFT = (1U << 2);
		static constexpr uint8_t DPAD_RIGHT= (1U << 3);
		static constexpr uint8_t L         = (1U << 6);
		static constexpr uint8_t ZL        = (1U << 7);
	}

	using SwitchReport = InReport;

	namespace Btn
	{
		static constexpr uint8_t Y       = Buttons0::Y;
		static constexpr uint8_t B       = Buttons0::B;
		static constexpr uint8_t A       = Buttons0::A;
		static constexpr uint8_t X       = Buttons0::X;
		static constexpr uint8_t R       = Buttons0::R;
		static constexpr uint8_t ZR      = Buttons0::ZR;
		static constexpr uint8_t MINUS   = Buttons1::MINUS;
		static constexpr uint8_t PLUS    = Buttons1::PLUS;
		static constexpr uint8_t L3      = Buttons1::L3;
		static constexpr uint8_t R3      = Buttons1::R3;
		static constexpr uint8_t HOME    = Buttons1::HOME;
		static constexpr uint8_t CAPTURE = Buttons1::CAPTURE;
		static constexpr uint8_t L       = Buttons2::L;
		static constexpr uint8_t ZL      = Buttons2::ZL;
	}

	static constexpr uint8_t HAT_NEUTRAL    = 0x00;
	static constexpr uint8_t HAT_DOWN       = 0x01;
	static constexpr uint8_t HAT_UP         = 0x02;
	static constexpr uint8_t HAT_RIGHT      = 0x04;
	static constexpr uint8_t HAT_LEFT       = 0x08;
	static constexpr uint8_t HAT_DOWN_RIGHT = 0x05;
	static constexpr uint8_t HAT_DOWN_LEFT  = 0x09;
	static constexpr uint8_t HAT_UP_RIGHT   = 0x06;
	static constexpr uint8_t HAT_UP_LEFT    = 0x0A;

	static constexpr uint16_t STICK_MIN = 0x000;
	static constexpr uint16_t STICK_MID = 0x7FF;
	static constexpr uint16_t STICK_MAX = 0xFFF;

	static constexpr uint8_t REPORT_ID_OUTPUT_SUBCMD = 0x01;
	static constexpr uint8_t REPORT_ID_OUTPUT_RUMBLE = 0x10;
	static constexpr uint8_t REPORT_ID_STANDARD = 0x30;
	/** Switch 2 Pro (USB) full input reports use 0x09; layout matches 0x30 after id + timer. */
	static constexpr uint8_t REPORT_ID_SWITCH2_FULL = 0x09;
	/** Alternate full-report id seen on some Switch-family USB stacks. */
	static constexpr uint8_t REPORT_ID_FULL_ALT   = 0x31;
	static constexpr uint8_t REPORT_ID_SUBCMD   = 0x21;
	static constexpr uint8_t REPORT_ID_USB_INIT = 0x81;
	static constexpr uint8_t REPORT_ID_USB_OUT  = 0x80;
	static constexpr int     REPORT_SIZE        = 64;
	static constexpr int     USB_INIT_REPORT_SIZE = 64;

	static constexpr uint8_t SUBCMD_BT_PAIR     = 0x01;
	static constexpr uint8_t SUBCMD_DEVICE_INFO = 0x02;
	static constexpr uint8_t SUBCMD_SET_MODE    = 0x03;
	static constexpr uint8_t SUBCMD_TRIGGER     = 0x04;
	static constexpr uint8_t SUBCMD_SHIPMENT    = 0x08;
	static constexpr uint8_t SUBCMD_SPI_READ    = 0x10;
	static constexpr uint8_t SUBCMD_NFC_IR_CFG  = 0x21;
	static constexpr uint8_t SUBCMD_NFC_IR_STATE= 0x22;
	static constexpr uint8_t SUBCMD_SET_PLAYER  = 0x30;
	static constexpr uint8_t SUBCMD_IMU         = 0x40;
	static constexpr uint8_t SUBCMD_IMU_SENS   = 0x41;
	static constexpr uint8_t SUBCMD_VIBRATION   = 0x48;
}

#endif
