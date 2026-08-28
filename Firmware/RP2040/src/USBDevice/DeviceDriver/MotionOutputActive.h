#pragma once

#include <atomic>

/** True while USB output is PS3 or PS4 (Sixaxis / DS4 IMU gadget). Switch motion disabled for now. */
namespace MotionOutputActive
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
