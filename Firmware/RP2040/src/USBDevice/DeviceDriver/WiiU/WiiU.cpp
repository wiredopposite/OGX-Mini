#include <cstring>

#include "Board/ogxm_log.h"
#include "USBDevice/DeviceDriver/WiiU/WiiU.h"

// Set to 1 to bypass init gate: always send reports for testing (Dolphin/Linux may not send 0x13).
#ifndef WIIU_SKIP_INIT_GATE
#define WIIU_SKIP_INIT_GATE 1
#endif

void WiiUDevice::initialize()
{
	class_driver_ =
    {
		.name = TUD_DRV_NAME("WIIU"),
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};

	in_report_.report_id = WiiU::HID_REPORT_ID;
	std::memset(in_report_.port_data, 0, sizeof(in_report_.port_data));
	init_received_ = false;
}

// GC stick deadzone: values near center map to 128 to avoid spurious LEFT/RIGHT/UP/DOWN
static constexpr int16_t STICK_DEADZONE = 4096;  // ~12.5% of -32768..32767

// Dolphin GameCube test reported max stick range: Left ±111, C-stick ~±110..112 (center 0).
// Our bytes 0–255 map to that via host; Y uses 1..254 to avoid full-up→down misread.

static uint8_t stick_to_gc(int16_t value)
{
	if (value > -STICK_DEADZONE && value < STICK_DEADZONE)
		return WiiU::STICK_CENTER;
	return Scale::int16_to_uint8(value);
}

// Y axis only: map to 1..254 so 0/255 are never sent (some hosts treat extremes as wrong direction).
static uint8_t stick_y_to_gc(int16_t value)
{
	if (value > -STICK_DEADZONE && value < STICK_DEADZONE)
		return WiiU::STICK_CENTER;
	uint8_t u = Scale::int16_to_uint8(value);
	if (u <= 1)   return 1u;
	if (u >= 254) return 254u;
	return u;
}

static void fill_port_block(uint8_t port[9], const Gamepad::PadIn& gp_in, bool stick_y_positive_is_up)
{
	// Byte 0 (type): Bit 4 = Wired. Dolphin: (type & 0x10) != 0 → connected.
	port[0] = WiiU::PORT_WIRED;

	// Byte 1 (b1): A, B, X, Y, D-pad Left, Right, Down, Up — direct mapping (A→A, B→B, etc.)
	uint8_t b1 = 0;
	if (gp_in.buttons & Gamepad::BUTTON_B)     b1 |= WiiU::GCButtons::A;
	if (gp_in.buttons & Gamepad::BUTTON_A)     b1 |= WiiU::GCButtons::B;
	if (gp_in.buttons & Gamepad::BUTTON_Y)     b1 |= WiiU::GCButtons::X;
	if (gp_in.buttons & Gamepad::BUTTON_X)     b1 |= WiiU::GCButtons::Y;
	if (gp_in.dpad & Gamepad::DPAD_LEFT)       b1 |= WiiU::GCButtons::LEFT;
	if (gp_in.dpad & Gamepad::DPAD_RIGHT)      b1 |= WiiU::GCButtons::RIGHT;
	if (gp_in.dpad & Gamepad::DPAD_DOWN)       b1 |= WiiU::GCButtons::DOWN;
	if (gp_in.dpad & Gamepad::DPAD_UP)         b1 |= WiiU::GCButtons::UP;
	port[1] = b1;

	// Byte 2 (b2): Start, Z, R (digital), L (digital)
	// L/R come from triggers at 255; Z from RB; LB maps to nothing
	uint8_t b2 = 0;
	if (gp_in.buttons & Gamepad::BUTTON_START) b2 |= WiiU::GCButtons::START;
	if (gp_in.buttons & Gamepad::BUTTON_RB)    b2 |= WiiU::GCButtons::Z;
	if (gp_in.trigger_l >= 255)                b2 |= WiiU::GCButtons::L;
	if (gp_in.trigger_r >= 255)                b2 |= WiiU::GCButtons::R;
	port[2] = b2;

	// Bytes 3–4: main stick X, Y (0–255, 128 = center).
	// Y: set by host — Xbox uses positive=up (send value), Wii U/Nintendo use negative=up (send 255-value).
	port[3] = stick_to_gc(gp_in.joystick_lx);
	{
		uint8_t ly = stick_y_to_gc(gp_in.joystick_ly);
		port[4] = stick_y_positive_is_up ? ly : static_cast<uint8_t>(255 - ly);
	}
	// Bytes 5–6: C-stick (substick) X, Y
	port[5] = stick_to_gc(gp_in.joystick_rx);
	{
		uint8_t ry = stick_y_to_gc(gp_in.joystick_ry);
		port[6] = stick_y_positive_is_up ? ry : static_cast<uint8_t>(255 - ry);
	}
	// Bytes 7–8: L/R trigger analog (0–255)
	port[7] = gp_in.trigger_l;
	port[8] = gp_in.trigger_r;
}

void WiiUDevice::process(const uint8_t idx, Gamepad& gamepad)
{
	if (idx >= 4)
		return;

	// Always update from current gamepad state so late controller connections are reflected immediately.
	// Dolphin may stop re-checking after seeing initial zeros; filling every frame avoids that.
	// Combo check (check_for_driver_change) also uses get_pad_in(); both see the same current state.
	Gamepad::PadIn gp_in = gamepad.get_pad_in();
	fill_port_block(in_report_.port_data[idx], gp_in, gamepad.stick_y_positive_is_up());

	if (tud_suspended())
		tud_remote_wakeup();

	// Send once per frame after the last port, only when init received (or WIIU_SKIP_INIT_GATE) and endpoint ready.
	// Real adapter does not send reports until host sends Start Polling (0x13).
	constexpr uint8_t last_port = (MAX_GAMEPADS >= 4) ? 3 : (MAX_GAMEPADS - 1);
	if (idx == last_port && (WIIU_SKIP_INIT_GATE || init_received_) && tud_hid_n_ready(0))
	{
		// TinyUSB prepends report_id; pass only port_data (36 bytes) to avoid double report ID
		tud_hid_n_report(0, WiiU::HID_REPORT_ID, in_report_.port_data, sizeof(in_report_.port_data));
#if defined(CONFIG_OGXM_DEBUG)
		static uint32_t send_count = 0;
		if (++send_count % 500 == 0)
			OGXM_LOG("WiiU: IN report sent (count " + std::to_string(send_count) + ")\n");
#endif
	}
}

uint16_t WiiUDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	if (report_type != HID_REPORT_TYPE_INPUT || report_id != WiiU::HID_REPORT_ID || itf != 0)
		return 0;
	if (reqlen < sizeof(WiiU::InReport))
		return 0;
	std::memcpy(buffer, &in_report_, sizeof(WiiU::InReport));
	return sizeof(WiiU::InReport);
}

void WiiUDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
	// Host sends Start Polling (0x13) via Output report. May be [0x13] or [0x21, 0x13] if report ID prepended.
	bool is_init = (bufsize >= 1 && buffer[0] == WiiU::INIT_PAYLOAD) ||
	               (bufsize >= 2 && buffer[1] == WiiU::INIT_PAYLOAD);
	if (is_init)
	{
		init_received_ = true;
#if defined(CONFIG_OGXM_DEBUG)
		OGXM_LOG("WiiU: init 0x13 received from host\n");
#endif
	}
	(void)itf;
	(void)report_id;
	(void)report_type;
}

bool WiiUDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
	(void)rhport;
	(void)stage;
	(void)request;
	return false;
}

const uint16_t* WiiUDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	const char *value = reinterpret_cast<const char*>(WiiU::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* WiiUDevice::get_descriptor_device_cb()
{
	return WiiU::DEVICE_DESCRIPTORS;
}

const uint8_t* WiiUDevice::get_hid_descriptor_report_cb(uint8_t itf)
{
	return WiiU::REPORT_DESCRIPTORS;
}

const uint8_t* WiiUDevice::get_descriptor_configuration_cb(uint8_t index)
{
	return WiiU::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* WiiUDevice::get_descriptor_device_qualifier_cb()
{
	return nullptr;
}
