#include <cstdio>
#include <cstring>

#include "pico/time.h"
#include "tusb.h"
#include "USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h"
#include "USBDevice/DeviceDriver/XInput/XInput.h"

extern "C" {
#include "xsm3.h"
}

// XSM3 state and buffers — match joypad-os: init at driver init, defer crypto to process() loop
namespace {
	enum class Xsm3AuthState : uint8_t {
		Idle = 0,
		InitReceived = 1,   // 0x82 data received, pending xsm3_do_challenge_init
		Responded = 2,      // init response ready for 0x83
		VerifyReceived = 3, // 0x87 data received, pending xsm3_do_challenge_verify
		Authenticated = 4,
	};
	static Xsm3AuthState xsm3_auth_state = Xsm3AuthState::Idle;
	static uint8_t xsm3_buf_82[0x22];
	static uint8_t xsm3_buf_87[0x16];
	// joypad-os: state 1 = processing, state 2 = response ready
	static constexpr uint8_t XSM3_STATE_PROCESSING = 1;
	static constexpr uint8_t XSM3_STATE_READY = 2;
	// joypad-os: init response is 46 bytes (0x2E), not full 0x30
	static constexpr uint16_t XSM3_RESPONSE_INIT_LEN = 46u;
	static constexpr uint16_t XSM3_RESPONSE_VERIFY_LEN = 0x16u;

	static void xsm3_process_pending(void) {
		if (xsm3_auth_state == Xsm3AuthState::InitReceived) {
			xsm3_do_challenge_init(xsm3_buf_82);
			xsm3_auth_state = Xsm3AuthState::Responded;
		} else if (xsm3_auth_state == Xsm3AuthState::VerifyReceived) {
			xsm3_do_challenge_verify(xsm3_buf_87);
			xsm3_auth_state = Xsm3AuthState::Authenticated;
		}
	}
}

void XInputDevice::initialize()
{
	class_driver_ = *tud_xinput::class_driver();
	// joypad-os: init XSM3 at mode init so 0x81 can send ID without doing init in callback
	xsm3_initialise_state();
	xsm3_set_identification_data(xsm3_id_data_ms_controller);
	xsm3_auth_state = Xsm3AuthState::Idle;
}

void XInputDevice::process(const uint8_t idx, Gamepad& gamepad)
{
	// joypad-os: run XSM3 crypto in task loop, not in USB callback
	xsm3_process_pending();

	// Always read latest state and build report every loop so get_report_cb and IN send both see current state (minimal latency; only delay is BT radio when wireless).
	in_report_.buttons[0] = 0;
	in_report_.buttons[1] = 0;
	Gamepad::PadIn gp_in = gamepad.get_pad_in();

	switch (gp_in.dpad)
	{
		case Gamepad::DPAD_UP:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_UP;
			break;
		case Gamepad::DPAD_DOWN:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_DOWN;
			break;
		case Gamepad::DPAD_LEFT:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_LEFT;
			break;
		case Gamepad::DPAD_RIGHT:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_RIGHT;
			break;
		case Gamepad::DPAD_UP_LEFT:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_UP | XInput::Buttons0::DPAD_LEFT;
			break;
		case Gamepad::DPAD_UP_RIGHT:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_UP | XInput::Buttons0::DPAD_RIGHT;
			break;
		case Gamepad::DPAD_DOWN_LEFT:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_DOWN | XInput::Buttons0::DPAD_LEFT;
			break;
		case Gamepad::DPAD_DOWN_RIGHT:
			in_report_.buttons[0] = XInput::Buttons0::DPAD_DOWN | XInput::Buttons0::DPAD_RIGHT;
			break;
		default:
			break;
	}

	if (gp_in.buttons & Gamepad::BUTTON_BACK)  in_report_.buttons[0] |= XInput::Buttons0::BACK;
	if (gp_in.buttons & Gamepad::BUTTON_START) in_report_.buttons[0] |= XInput::Buttons0::START;
	if (gp_in.buttons & Gamepad::BUTTON_L3)    in_report_.buttons[0] |= XInput::Buttons0::L3;
	if (gp_in.buttons & Gamepad::BUTTON_R3)    in_report_.buttons[0] |= XInput::Buttons0::R3;

	if (gp_in.buttons & Gamepad::BUTTON_X)     in_report_.buttons[1] |= XInput::Buttons1::X;
	if (gp_in.buttons & Gamepad::BUTTON_A)     in_report_.buttons[1] |= XInput::Buttons1::A;
	if (gp_in.buttons & Gamepad::BUTTON_Y)     in_report_.buttons[1] |= XInput::Buttons1::Y;
	if (gp_in.buttons & Gamepad::BUTTON_B)     in_report_.buttons[1] |= XInput::Buttons1::B;
	if (gp_in.buttons & Gamepad::BUTTON_LB)    in_report_.buttons[1] |= XInput::Buttons1::LB;
	if (gp_in.buttons & Gamepad::BUTTON_RB)    in_report_.buttons[1] |= XInput::Buttons1::RB;
	if (gp_in.buttons & Gamepad::BUTTON_SYS)   in_report_.buttons[1] |= XInput::Buttons1::HOME;

	in_report_.trigger_l = gp_in.trigger_l;
	in_report_.trigger_r = gp_in.trigger_r;

	in_report_.joystick_lx = gp_in.joystick_lx;
	in_report_.joystick_ly = Range::invert(gp_in.joystick_ly);
	in_report_.joystick_rx = gp_in.joystick_rx;
	in_report_.joystick_ry = Range::invert(gp_in.joystick_ry);

	// Remote wake when host has suspended the bus (e.g. 360 "off" with USB power kept):
	// - Guide (Home) press, or
	// - Start held for 3 seconds (avoids holding Guide on Xbox One/PS5 pads, which can turn the controller off).
	{
		static bool start_wake_sent = false;
		static bool start_held = false;
		static absolute_time_t start_hold_begin = { 0 };
		bool start_pressed = (gp_in.buttons & Gamepad::BUTTON_START) != 0;
		if (start_pressed)
		{
			if (!start_held)
			{
				start_held = true;
				start_hold_begin = get_absolute_time();
			}
			else
			{
				uint64_t hold_ms = to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(start_hold_begin);
				if (hold_ms >= 3000 && tud_suspended() && !start_wake_sent)
				{
					tud_remote_wakeup();
					start_wake_sent = true;
				}
			}
		}
		else
		{
			start_held = false;
			start_wake_sent = false;
		}
		if (tud_suspended() && (gp_in.buttons & Gamepad::BUTTON_SYS))
			tud_remote_wakeup();
	}

	// send_report() only transmits when endpoint is free; otherwise we keep latest in_report_ for get_report_cb
	tud_xinput::send_report((uint8_t*)&in_report_, sizeof(XInput::InReport));

    if (tud_xinput::receive_report(reinterpret_cast<uint8_t*>(&out_report_), sizeof(XInput::OutReport)) &&
        out_report_.report_id == XInput::OutReportID::RUMBLE)
    {
        Gamepad::PadOut gp_out;
        gp_out.rumble_l = out_report_.rumble_l;
        gp_out.rumble_r = out_report_.rumble_r;
        gamepad.set_pad_out(gp_out);
    }
}

uint16_t XInputDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    std::memcpy(buffer, &in_report_, sizeof(XInput::InReport));
	return sizeof(XInput::InReport);
}

void XInputDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool XInputDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
	printf("XSM3 cb bReq 0x%02x stage %u bmReqType 0x%02x wIdx 0x%04x\n",
		(unsigned)request->bRequest, (unsigned)stage, (unsigned)request->bmRequestType, (unsigned)request->wIndex);
	// 360 can send XSM3 as Vendor (0x40/0xC0) or Class (0x20/0xA0) to the security interface.
	// Accept Vendor (bits 6-5 == 0x40) or Class (bits 6-5 == 0x20).
	uint8_t type_bits = request->bmRequestType & 0x60;
	if (type_bits != 0x40 && type_bits != 0x20)
		return false;

	switch (request->bRequest)
	{
	// 0x81: Host GET — send controller identification (0x1D bytes). XSM3 already inited in initialize().
	case 0x81:
		if (stage == CONTROL_STAGE_SETUP) printf("XSM3: 0x81 GET_SERIAL\n");
		if (stage == CONTROL_STAGE_SETUP && request->wLength >= 0x1D)
			return tud_control_xfer(rhport, request, const_cast<uint8_t*>(xsm3_id_data_ms_controller), 0x1D);
		return true;

	// 0x82: Host OUT — receive challenge init; defer xsm3_do_challenge_init to process() (joypad-os)
	case 0x82:
		if (stage == CONTROL_STAGE_SETUP && request->wLength >= 0x22)
			return tud_control_xfer(rhport, request, xsm3_buf_82, 0x22);
		if (stage == CONTROL_STAGE_DATA)
		{
			printf("XSM3: 0x82 challenge init received (defer process)\n");
			xsm3_auth_state = Xsm3AuthState::InitReceived;
		}
		return true;

	// 0x83: Host GET — send challenge response. Joypad: 46 bytes init, 22 bytes verify.
	case 0x83:
		if (stage == CONTROL_STAGE_SETUP)
		{
			if (xsm3_auth_state == Xsm3AuthState::Responded)
			{
				printf("XSM3: 0x83 sending init response (46 bytes)\n");
				uint16_t len = (request->wLength < XSM3_RESPONSE_INIT_LEN) ? request->wLength : XSM3_RESPONSE_INIT_LEN;
				bool ok = tud_control_xfer(rhport, request, xsm3_challenge_response, len);
				if (ok)
					xsm3_auth_state = Xsm3AuthState::Idle;
				return ok;
			}
			if (xsm3_auth_state == Xsm3AuthState::Authenticated)
			{
				printf("XSM3: 0x83 sending verify response (0x16 bytes)\n");
				uint16_t len = (request->wLength < XSM3_RESPONSE_VERIFY_LEN) ? request->wLength : XSM3_RESPONSE_VERIFY_LEN;
				bool ok = tud_control_xfer(rhport, request, xsm3_challenge_response, len);
				if (ok)
					xsm3_auth_state = Xsm3AuthState::Idle;
				return ok;
			}
			return false;
		}
		return true;

	// 0x84: Host GET — keepalive, zero-length response (joypad-os)
	case 0x84:
		if (stage == CONTROL_STAGE_SETUP) printf("XSM3: 0x84 KEEPALIVE\n");
		if (stage == CONTROL_STAGE_SETUP)
			return tud_control_xfer(rhport, request, nullptr, 0);
		return true;

	// 0x86: Host GET — state 1 = processing, 2 = response ready (joypad-os)
	case 0x86:
		if (stage == CONTROL_STAGE_SETUP && request->wLength >= 2)
		{
			uint8_t state_val = (xsm3_auth_state == Xsm3AuthState::Responded || xsm3_auth_state == Xsm3AuthState::Authenticated)
				? XSM3_STATE_READY : XSM3_STATE_PROCESSING;
			printf("XSM3: 0x86 GET_STATE %u\n", (unsigned)state_val);
			uint8_t state_data[] = { state_val, 0x00 };
			return tud_control_xfer(rhport, request, state_data, 2);
		}
		return true;

	// 0x87: Host OUT — receive verify; defer xsm3_do_challenge_verify to process() (joypad-os)
	case 0x87:
		if (stage == CONTROL_STAGE_SETUP && request->wLength >= 0x16)
			return tud_control_xfer(rhport, request, xsm3_buf_87, 0x16);
		if (stage == CONTROL_STAGE_DATA)
		{
			printf("XSM3: 0x87 verify received (defer process)\n");
			xsm3_auth_state = Xsm3AuthState::VerifyReceived;
		}
		return true;

	default:
		return false;
	}
}

const uint16_t * XInputDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	// joypad-os: XInput string index 4 (XSM3 security) uses a 96-char buffer and full length
	if (index == 4)
	{
		static uint16_t xsm3_str[96];
		const char *str = reinterpret_cast<const char*>(XInput::STRING_XSM3);
		size_t len = std::strlen(str);
		if (len > 95) len = 95;
		for (size_t i = 0; i < len; i++)
			xsm3_str[1 + i] = static_cast<uint8_t>(str[i]);
		xsm3_str[0] = (0x03u << 8) | static_cast<uint16_t>(2 * len + 2);
		return xsm3_str;
	}
	const char *value = reinterpret_cast<const char*>(XInput::DESC_STRING[index]);
	return get_string_descriptor(value, index);
}

const uint8_t * XInputDevice::get_descriptor_device_cb() 
{
    return XInput::DESC_DEVICE;
}

const uint8_t * XInputDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * XInputDevice::get_descriptor_configuration_cb(uint8_t index)
{
	// joypad-os: single config; return nullptr for index != 0 (bNumConfigurations == 1)
	if (index != 0)
		return nullptr;
	return XInput::DESC_CONFIGURATION;
}

const uint8_t * XInputDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}