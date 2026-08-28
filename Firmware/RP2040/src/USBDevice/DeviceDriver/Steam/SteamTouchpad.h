#pragma once

#include <cstddef>
#include <cstdint>

namespace SteamTouchpad
{

/** First touch point in a 64-byte DualSense USB input report (report ID at [0]). */
constexpr size_t kReportTouchPointsOffset = 33;

void touchpoint_decode(const uint8_t raw[4], int& x, int& y, bool& touching);

/** Encode one DualSense touch point (inverse of touchpoint_decode). */
void touchpoint_encode(uint8_t raw[4], int x, int y, bool touching, uint8_t counter);

/** Bluetooth: raw 8-byte touch region + physical click. */
void send_mouse_from_touch(const uint8_t touch_points[8], bool touchpad_click);

/** Wired USB: full 64-byte DualSense input report. */
void send_mouse_from_report(const uint8_t report[64]);

/** Merge touch bytes into the Steam passthrough USB input report. */
void apply_to_passthrough(const uint8_t touch_points[8], bool touchpad_click);

/** Last relative mouse HID packet sent (for GET_REPORT on interface 1). */
extern uint8_t last_mouse_report[3];

/** Send a relative mouse HID report on the Steam mouse interface. */
bool send_mouse_packet(uint8_t buttons, int8_t dx, int8_t dy);

} // namespace SteamTouchpad
