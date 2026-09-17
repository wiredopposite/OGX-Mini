#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "Descriptors/PS5.h"
#include "Gamepad/Gamepad.h"
#include "USBDevice/DeviceDriver/Steam/SteamPassthrough.h"

#if defined(CONFIG_EN_BLUETOOTH)
#include "controller/uni_gamepad.h"
#endif

namespace SteamBtReport
{

#if defined(CONFIG_EN_BLUETOOTH)

inline uint8_t ps5_stick_byte(int32_t axis)
{
	int32_t b = axis / 4 + 127;
	if (b < 0) {
		return 0;
	}
	if (b > 255) {
		return 255;
	}
	return static_cast<uint8_t>(b);
}

inline uint8_t ps5_trigger_byte(int32_t value)
{
	int32_t b = value >> 2;
	if (b < 0) {
		return 0;
	}
	if (b > 255) {
		return 255;
	}
	return static_cast<uint8_t>(b);
}

inline void update_from_uni_gamepad(const uni_gamepad_t* gp)
{
	PS5::InReport rep{};
	rep.report_id = 0x01;
	rep.joystick_lx = ps5_stick_byte(gp->axis_x);
	rep.joystick_ly = ps5_stick_byte(gp->axis_y);
	rep.joystick_rx = ps5_stick_byte(gp->axis_rx);
	rep.joystick_ry = ps5_stick_byte(gp->axis_ry);
	rep.trigger_l = ps5_trigger_byte(gp->brake);
	rep.trigger_r = ps5_trigger_byte(gp->throttle);

	uint8_t b0 = PS5::Buttons0::DPAD_CENTER;
	switch (gp->dpad) {
		case DPAD_UP:
			b0 = PS5::Buttons0::DPAD_UP;
			break;
		case DPAD_DOWN:
			b0 = PS5::Buttons0::DPAD_DOWN;
			break;
		case DPAD_LEFT:
			b0 = PS5::Buttons0::DPAD_LEFT;
			break;
		case DPAD_RIGHT:
			b0 = PS5::Buttons0::DPAD_RIGHT;
			break;
		case DPAD_UP | DPAD_RIGHT:
			b0 = PS5::Buttons0::DPAD_UP_RIGHT;
			break;
		case DPAD_DOWN | DPAD_RIGHT:
			b0 = PS5::Buttons0::DPAD_RIGHT_DOWN;
			break;
		case DPAD_DOWN | DPAD_LEFT:
			b0 = PS5::Buttons0::DPAD_DOWN_LEFT;
			break;
		case DPAD_UP | DPAD_LEFT:
			b0 = PS5::Buttons0::DPAD_LEFT_UP;
			break;
		default:
			break;
	}
	if (gp->buttons & BUTTON_X) {
		b0 |= PS5::Buttons0::SQUARE;
	}
	if (gp->buttons & BUTTON_A) {
		b0 |= PS5::Buttons0::CROSS;
	}
	if (gp->buttons & BUTTON_B) {
		b0 |= PS5::Buttons0::CIRCLE;
	}
	if (gp->buttons & BUTTON_Y) {
		b0 |= PS5::Buttons0::TRIANGLE;
	}
	rep.buttons[0] = b0;

	uint8_t b1 = 0;
	if (gp->buttons & BUTTON_SHOULDER_L) {
		b1 |= PS5::Buttons1::L1;
	}
	if (gp->buttons & BUTTON_SHOULDER_R) {
		b1 |= PS5::Buttons1::R1;
	}
	if (gp->misc_buttons & MISC_BUTTON_BACK) {
		b1 |= PS5::Buttons1::SHARE;
	}
	if (gp->misc_buttons & MISC_BUTTON_START) {
		b1 |= PS5::Buttons1::OPTIONS;
	}
	if (gp->buttons & BUTTON_THUMB_L) {
		b1 |= PS5::Buttons1::L3;
	}
	if (gp->buttons & BUTTON_THUMB_R) {
		b1 |= PS5::Buttons1::R3;
	}
	rep.buttons[1] = b1;

	uint8_t b2 = 0;
	if (gp->misc_buttons & MISC_BUTTON_SYSTEM) {
		b2 |= PS5::Buttons2::PS;
	}
	if (gp->misc_buttons & MISC_BUTTON_CAPTURE) {
		b2 |= PS5::Buttons2::MUTE;
	}
	rep.buttons[2] = b2;

	uint8_t buf[SteamPassthrough::USB_REPORT_SIZE]{};
	std::memcpy(buf, &rep, sizeof(rep));
	SteamPassthrough::store(buf, sizeof(buf));
}

#endif /* CONFIG_EN_BLUETOOTH */

inline uint8_t pad_stick_byte(int16_t axis)
{
	int32_t scaled = (static_cast<int32_t>(axis) + 32768) * 255 / 65535;
	if (scaled < 0) {
		return 0;
	}
	if (scaled > 255) {
		return 255;
	}
	return static_cast<uint8_t>(scaled);
}

inline void fill_report_from_pad(PS5::InReport& rep, const Gamepad::PadIn& gp_in, const Gamepad& gamepad)
{
	rep.report_id = 0x01;
	rep.joystick_lx = pad_stick_byte(gp_in.joystick_lx);
	rep.joystick_ly = pad_stick_byte(gp_in.joystick_ly);
	rep.joystick_rx = pad_stick_byte(gp_in.joystick_rx);
	rep.joystick_ry = pad_stick_byte(gp_in.joystick_ry);
	rep.trigger_l = gp_in.trigger_l;
	rep.trigger_r = gp_in.trigger_r;

	uint8_t b0 = PS5::Buttons0::DPAD_CENTER;
	if (gp_in.dpad & gamepad.MAP_DPAD_UP) {
		if (gp_in.dpad & gamepad.MAP_DPAD_LEFT) {
			b0 = PS5::Buttons0::DPAD_LEFT_UP;
		} else if (gp_in.dpad & gamepad.MAP_DPAD_RIGHT) {
			b0 = PS5::Buttons0::DPAD_UP_RIGHT;
		} else {
			b0 = PS5::Buttons0::DPAD_UP;
		}
	} else if (gp_in.dpad & gamepad.MAP_DPAD_DOWN) {
		if (gp_in.dpad & gamepad.MAP_DPAD_LEFT) {
			b0 = PS5::Buttons0::DPAD_DOWN_LEFT;
		} else if (gp_in.dpad & gamepad.MAP_DPAD_RIGHT) {
			b0 = PS5::Buttons0::DPAD_RIGHT_DOWN;
		} else {
			b0 = PS5::Buttons0::DPAD_DOWN;
		}
	} else if (gp_in.dpad & gamepad.MAP_DPAD_LEFT) {
		b0 = PS5::Buttons0::DPAD_LEFT;
	} else if (gp_in.dpad & gamepad.MAP_DPAD_RIGHT) {
		b0 = PS5::Buttons0::DPAD_RIGHT;
	}

	if (gp_in.buttons & gamepad.MAP_BUTTON_X) {
		b0 |= PS5::Buttons0::SQUARE;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_A) {
		b0 |= PS5::Buttons0::CROSS;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_B) {
		b0 |= PS5::Buttons0::CIRCLE;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_Y) {
		b0 |= PS5::Buttons0::TRIANGLE;
	}
	rep.buttons[0] = b0;

	uint8_t b1 = 0;
	if (gp_in.buttons & gamepad.MAP_BUTTON_LB) {
		b1 |= PS5::Buttons1::L1;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_RB) {
		b1 |= PS5::Buttons1::R1;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_BACK) {
		b1 |= PS5::Buttons1::SHARE;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_START) {
		b1 |= PS5::Buttons1::OPTIONS;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_L3) {
		b1 |= PS5::Buttons1::L3;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_R3) {
		b1 |= PS5::Buttons1::R3;
	}
	rep.buttons[1] = b1;

	uint8_t b2 = 0;
	if (gp_in.buttons & gamepad.MAP_BUTTON_SYS) {
		b2 |= PS5::Buttons2::PS;
	}
	if (gp_in.buttons & gamepad.MAP_BUTTON_MISC) {
		b2 |= PS5::Buttons2::MUTE;
	}
	rep.buttons[2] = b2;
}

inline void update_from_gamepad(Gamepad& gamepad)
{
	PS5::InReport rep{};
	fill_report_from_pad(rep, gamepad.get_pad_in(), gamepad);
	uint8_t buf[SteamPassthrough::USB_REPORT_SIZE]{};
	std::memcpy(buf, &rep, sizeof(rep));
	SteamPassthrough::store(buf, sizeof(buf));
}

} // namespace SteamBtReport
