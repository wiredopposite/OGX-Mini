#pragma once

#include <cstdint>
#include <cstring>

namespace SteamPassthrough
{
	static constexpr uint8_t USB_REPORT_SIZE = 64;

	inline uint8_t report[USB_REPORT_SIZE]{};
	inline volatile bool has_report = false;
	/** Input device exposes a DualSense-style touchpad (BT or wired PS5). */
	inline volatile bool input_has_touchpad = false;
	inline uint16_t host_vid = 0x054C;
	inline uint16_t host_pid = 0x0CE6;

	inline void set_host_identity(uint16_t vid, uint16_t pid)
	{
		host_vid = vid;
		host_pid = pid;
	}

	inline void store(const uint8_t* data, uint16_t len)
	{
		std::memset(report, 0, USB_REPORT_SIZE);
		const uint16_t n = (len > USB_REPORT_SIZE) ? USB_REPORT_SIZE : len;
		std::memcpy(report, data, n);
		has_report = true;
	}

	inline void clear()
	{
		has_report = false;
		input_has_touchpad = false;
		std::memset(report, 0, USB_REPORT_SIZE);
	}
}
