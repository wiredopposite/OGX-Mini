#pragma once

#include <atomic>

/** True while the USB output driver is SteamDevice (touchpad → HID mouse). */
namespace SteamActive
{
	inline std::atomic<bool> enabled{false};

	inline void set(bool on)
	{
		enabled.store(on, std::memory_order_release);
	}

	inline bool is_enabled()
	{
		return enabled.load(std::memory_order_acquire);
	}
}
