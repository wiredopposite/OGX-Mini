#include "USBDevice/DeviceDriver/Steam/SteamTouchpad.h"

#include <algorithm>
#include <cstring>

#include "Descriptors/PS5.h"
#include "Descriptors/Steam.h"
#include "USBDevice/DeviceDriver/Steam/SteamPassthrough.h"
#include "tusb.h"

namespace SteamTouchpad
{

uint8_t last_mouse_report[3]{};

namespace
{

constexpr int kDeltaDivisor = 1;
constexpr int kMaxDelta = 127;

int clamp_delta(int v)
{
	if (v > kMaxDelta) {
		return kMaxDelta;
	}
	if (v < -kMaxDelta) {
		return -kMaxDelta;
	}
	return v;
}

const uint8_t* touch_raw(const uint8_t report[64])
{
	return &report[kReportTouchPointsOffset];
}

bool send_mouse_hid(uint8_t buttons, int8_t dx, int8_t dy)
{
	if (!tud_mounted() || !tud_hid_n_ready(Steam::ITF_MOUSE)) {
		return false;
	}
	const uint8_t rep[3] = {
		buttons,
		static_cast<uint8_t>(dx),
		static_cast<uint8_t>(dy),
	};
	if (tud_hid_n_report(Steam::ITF_MOUSE, 0, rep, sizeof(rep))) {
		std::memcpy(last_mouse_report, rep, sizeof(rep));
		return true;
	}
	return false;
}

void send_mouse_deltas(uint8_t buttons, int cx, int cy, int& last_x, int& last_y, bool touching,
                       bool& was_touching, uint8_t& prev_buttons)
{
	int8_t dx = 0;
	int8_t dy = 0;

	if (touching && was_touching && last_x >= 0) {
		dx = static_cast<int8_t>(clamp_delta((cx - last_x) / kDeltaDivisor));
		dy = static_cast<int8_t>(clamp_delta((cy - last_y) / kDeltaDivisor));
	}

	if (dx != 0 || dy != 0 || buttons != prev_buttons) {
		if (send_mouse_hid(buttons, dx, dy)) {
			prev_buttons = buttons;
			if (touching) {
				last_x = cx;
				last_y = cy;
			} else {
				last_x = -1;
				last_y = -1;
			}
			was_touching = touching;
			return;
		}
	}

	if (!touching) {
		last_x = -1;
		last_y = -1;
	}
	was_touching = touching;
}

void decode_active_finger(const uint8_t touch_points[8], int& cx, int& cy, bool& touching)
{
	int x0 = 0;
	int y0 = 0;
	bool touch0 = false;
	int x1 = 0;
	int y1 = 0;
	bool touch1 = false;
	touchpoint_decode(&touch_points[0], x0, y0, touch0);
	touchpoint_decode(&touch_points[4], x1, y1, touch1);

	touching = touch0 || touch1;
	cx = touch0 ? x0 : x1;
	cy = touch0 ? y0 : y1;
}

} // namespace

void touchpoint_decode(const uint8_t raw[4], int& x, int& y, bool& touching)
{
	touching = (raw[0] & 0x80) == 0;
	const uint8_t x_lo = raw[1];
	const uint8_t x_hi = raw[2] & 0x0F;
	const uint8_t y_lo = (raw[2] >> 4) & 0x0F;
	const uint8_t y_hi = raw[3];
	x = (x_hi << 8) | x_lo;
	y = (y_hi << 4) | y_lo;
}

void touchpoint_encode(uint8_t raw[4], int x, int y, bool touching, uint8_t counter)
{
	if (x < 0) {
		x = 0;
	}
	if (x > 1919) {
		x = 1919;
	}
	if (y < 0) {
		y = 0;
	}
	if (y > 1079) {
		y = 1079;
	}
	raw[0] = static_cast<uint8_t>((counter & 0x7F) | (touching ? 0 : 0x80));
	raw[1] = static_cast<uint8_t>(x & 0xFF);
	raw[2] = static_cast<uint8_t>((x >> 8) & 0x0F) | static_cast<uint8_t>((y & 0x0F) << 4);
	raw[3] = static_cast<uint8_t>((y >> 4) & 0xFF);
}

void apply_to_passthrough(const uint8_t touch_points[8], bool touchpad_click)
{
	if (!SteamPassthrough::has_report) {
		return;
	}

	std::memcpy(&SteamPassthrough::report[kReportTouchPointsOffset], touch_points, 8);

	auto* in = reinterpret_cast<PS5::InReport*>(SteamPassthrough::report);
	if (touchpad_click) {
		in->buttons[2] |= static_cast<uint8_t>(PS5::Buttons2::TP);
	} else {
		in->buttons[2] &= static_cast<uint8_t>(~static_cast<uint8_t>(PS5::Buttons2::TP));
	}
}

void send_mouse_from_touch(const uint8_t touch_points[8], bool touchpad_click)
{
	static int last_x = -1;
	static int last_y = -1;
	static bool was_touching = false;
	static uint8_t prev_buttons = 0;

	int cx = 0;
	int cy = 0;
	bool touching = false;
	decode_active_finger(touch_points, cx, cy, touching);

	uint8_t buttons = touchpad_click ? 0x01u : 0x00u;
	send_mouse_deltas(buttons, cx, cy, last_x, last_y, touching, was_touching, prev_buttons);
}

void send_mouse_from_report(const uint8_t report[64])
{
	const uint8_t* tp = touch_raw(report);

	int x0 = 0;
	int y0 = 0;
	bool touch0 = false;
	int x1 = 0;
	int y1 = 0;
	bool touch1 = false;
	touchpoint_decode(&tp[0], x0, y0, touch0);
	touchpoint_decode(&tp[4], x1, y1, touch1);

	static int last_x = -1;
	static int last_y = -1;
	static bool was_touching = false;
	static uint8_t prev_buttons = 0;

	const bool touching = touch0 || touch1;
	const int cx = touch0 ? x0 : x1;
	const int cy = touch0 ? y0 : y1;

	uint8_t buttons = 0;
	const auto* in = reinterpret_cast<const PS5::InReport*>(report);
	if (in->buttons[2] & static_cast<uint8_t>(PS5::Buttons2::TP)) {
		buttons |= 0x01u;
	}

	send_mouse_deltas(buttons, cx, cy, last_x, last_y, touching, was_touching, prev_buttons);
}

bool send_mouse_packet(uint8_t buttons, int8_t dx, int8_t dy)
{
	return send_mouse_hid(buttons, dx, dy);
}

} // namespace SteamTouchpad
