#pragma once

#include <cstdint>
#include <cstring>

/** Latest wired DualSense input for Steam mode touchpad → USB mouse (raw 64-byte wire layout). */
namespace SteamInput
{
	static constexpr uint8_t USB_REPORT_SIZE = 64;

	inline uint8_t report[USB_REPORT_SIZE]{};
	inline volatile bool has_wired_report = false;

	inline void store_wired(const uint8_t* data, uint16_t len)
	{
		std::memset(report, 0, USB_REPORT_SIZE);
		const uint16_t n = (len > USB_REPORT_SIZE) ? USB_REPORT_SIZE : len;
		std::memcpy(report, data, n);
		has_wired_report = true;
	}

	inline void clear()
	{
		has_wired_report = false;
		std::memset(report, 0, USB_REPORT_SIZE);
	}
}
