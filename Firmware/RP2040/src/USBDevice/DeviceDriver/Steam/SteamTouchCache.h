#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>

#include "USBDevice/DeviceDriver/Steam/SteamTouchpad.h"

/** Core1 (Bluepad32) writes, Core0 (SteamDevice) reads — DualSense touchpad samples. */
namespace SteamTouchCache
{
	inline uint8_t touch_points[8]{};
	inline uint8_t usb_report[64]{};
	inline bool touchpad_click = false;
	inline std::atomic<uint32_t> generation{0};

	inline void store(const uint8_t points[8], bool click)
	{
		std::memcpy(touch_points, points, 8);
		touchpad_click = click;

		std::memset(usb_report, 0, sizeof(usb_report));
		usb_report[0] = 0x01;
		std::memcpy(&usb_report[SteamTouchpad::kReportTouchPointsOffset], points, 8);
		if (click) {
			usb_report[10] |= 0x02; /* PS5::Buttons2::TP */
		}

		generation.fetch_add(1, std::memory_order_release);
	}

	/** Decode touch from a raw DualSense 0x31 Bluetooth input report. */
	inline void store_from_bt_report(const uint8_t* report, uint16_t len)
	{
		if (!report || len < 42 || report[0] != 0x31) {
			return;
		}
		enum { kTouchOffset = 34 };
		if (len < kTouchOffset + 8) {
			return;
		}
		const bool click = (len >= 12) ? ((report[11] & 0x02) != 0) : false;
		store(&report[kTouchOffset], click);
	}

	inline void clear()
	{
		touch_points[0] = 0x80;
		touch_points[1] = touch_points[2] = touch_points[3] = 0;
		touch_points[4] = 0x80;
		touch_points[5] = touch_points[6] = touch_points[7] = 0;
		touchpad_click = false;
		std::memset(usb_report, 0, sizeof(usb_report));
		generation.store(0, std::memory_order_release);
	}

	/** Returns false if no sample has been stored yet. */
	inline bool load(uint8_t out_points[8], bool& click)
	{
		for (int attempt = 0; attempt < 4; ++attempt) {
			const uint32_t gen_before = generation.load(std::memory_order_acquire);
			if (gen_before == 0) {
				return false;
			}
			std::memcpy(out_points, touch_points, 8);
			click = touchpad_click;
			const uint32_t gen_after = generation.load(std::memory_order_acquire);
			if (gen_before == gen_after) {
				return true;
			}
		}
		std::memcpy(out_points, touch_points, 8);
		click = touchpad_click;
		return generation.load(std::memory_order_acquire) != 0;
	}
}
