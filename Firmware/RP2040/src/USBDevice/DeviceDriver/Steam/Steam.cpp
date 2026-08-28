#include <algorithm>
#include <cstring>

#include "USBDevice/DeviceDriver/Steam/Steam.h"
#include "USBDevice/DeviceDriver/Steam/SteamPassthrough.h"
#include "USBDevice/DeviceDriver/Steam/SteamTouchpad.h"
#include "USBDevice/DeviceDriver/Steam/SteamBtReport.h"
#include "Descriptors/PS5Usb.h"
#include "Descriptors/Steam.h"

namespace {

constexpr uint8_t kReportIdIn = 0x01;

void init_neutral_report(std::array<uint8_t, SteamPassthrough::USB_REPORT_SIZE>& rep)
{
	std::memset(rep.data(), 0, rep.size());
	rep[0] = kReportIdIn;
	rep[1] = rep[2] = rep[3] = rep[4] = PS5::JOYSTICK_MID;
}

} // namespace

void SteamDevice::initialize()
{
	class_driver_ = {
		.name = TUD_DRV_NAME("STEAM"),
		.init = hidd_init,
		.deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};

	std::memcpy(device_descriptor_, PS5Usb::DEVICE_DESCRIPTORS, sizeof(device_descriptor_));
	init_neutral_report(report_in_);
	report_out_.report_id = PS5::OutReportID::RUMBLE;
}

void SteamDevice::process(const uint8_t idx, Gamepad& gamepad)
{
	(void)idx;

	const Gamepad::PadIn gp_in = gamepad.get_pad_in();

	if (!SteamPassthrough::input_has_touchpad) {
		PS5::InReport rep{};
		SteamBtReport::fill_report_from_pad(rep, gp_in, gamepad);
		std::memcpy(report_in_.data(), &rep, sizeof(rep));
		if (report_in_.size() > sizeof(rep)) {
			std::memset(report_in_.data() + sizeof(rep), 0, report_in_.size() - sizeof(rep));
		}
		/* Core1 passthrough uses raw host axes (no profile axis-restrict) for sticks/triggers. */
		if (SteamPassthrough::has_report) {
			auto* out = reinterpret_cast<PS5::InReport*>(report_in_.data());
			const auto* raw = reinterpret_cast<const PS5::InReport*>(SteamPassthrough::report);
			out->joystick_rx = raw->joystick_rx;
			out->joystick_ry = raw->joystick_ry;
			out->trigger_l = raw->trigger_l;
			out->trigger_r = raw->trigger_r;
		}
	} else if (SteamPassthrough::has_report) {
		std::memcpy(report_in_.data(), SteamPassthrough::report, SteamPassthrough::USB_REPORT_SIZE);
	} else {
		init_neutral_report(report_in_);
	}

	/* Mouse only from DualSense (or other) touchpad — no right-stick mouse fallback. */
	if (SteamPassthrough::input_has_touchpad) {
		SteamTouchpad::send_mouse_from_report(report_in_.data());
	}

	if (tud_hid_n_ready(Steam::ITF_GAMEPAD)) {
		tud_hid_n_report(Steam::ITF_GAMEPAD, 0, report_in_.data(), static_cast<uint16_t>(report_in_.size()));
	}

	if (new_report_out_) {
		Gamepad::PadOut gp_out;
		gp_out.rumble_l = report_out_.motor_left;
		gp_out.rumble_r = report_out_.motor_right;
		gamepad.set_pad_out(gp_out);
		new_report_out_ = false;
	}
}

uint16_t SteamDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	if (report_type == HID_REPORT_TYPE_INPUT) {
		if (itf == Steam::ITF_MOUSE) {
			const uint16_t n = static_cast<uint16_t>(std::min<uint16_t>(reqlen, 4));
			std::memset(buffer, 0, n);
			const uint16_t copy = static_cast<uint16_t>(std::min<size_t>(n, sizeof(SteamTouchpad::last_mouse_report)));
			std::memcpy(buffer, SteamTouchpad::last_mouse_report, copy);
			return n;
		}
		if (itf == Steam::ITF_GAMEPAD && (report_id == 0 || report_id == kReportIdIn)) {
			const uint16_t n = static_cast<uint16_t>(std::min<size_t>(reqlen, report_in_.size()));
			std::memcpy(buffer, report_in_.data(), n);
			return n;
		}
	} else if (report_type == HID_REPORT_TYPE_FEATURE) {
		std::memset(buffer, 0, reqlen);
		return reqlen;
	}
	return 0;
}

void SteamDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
	if (itf != Steam::ITF_GAMEPAD || report_type != HID_REPORT_TYPE_OUTPUT) {
		return;
	}

	uint8_t rid = report_id;
	const uint8_t *buf = buffer;
	uint16_t len = bufsize;
	if (rid == 0 && len > 0) {
		rid = buffer[0];
		len = static_cast<uint16_t>(len - 1u);
		buf = &buffer[1];
	}

	if ((rid == PS5::OutReportID::RUMBLE || rid == PS5::OutReportID::CONTROL) &&
	    len >= sizeof(PS5::OutReport)) {
		std::memcpy(&report_out_, buf, sizeof(PS5::OutReport));
		new_report_out_ = true;
	}
}

bool SteamDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
	(void)rhport;
	(void)stage;
	(void)request;
	return false;
}

const uint16_t* SteamDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	const char *value = reinterpret_cast<const char*>(PS5Usb::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* SteamDevice::get_descriptor_device_cb()
{
	device_descriptor_[8] = static_cast<uint8_t>(SteamPassthrough::host_vid & 0xFF);
	device_descriptor_[9] = static_cast<uint8_t>((SteamPassthrough::host_vid >> 8) & 0xFF);
	device_descriptor_[10] = static_cast<uint8_t>(SteamPassthrough::host_pid & 0xFF);
	device_descriptor_[11] = static_cast<uint8_t>((SteamPassthrough::host_pid >> 8) & 0xFF);
	return device_descriptor_;
}

const uint8_t* SteamDevice::get_hid_descriptor_report_cb(uint8_t itf)
{
	if (itf == Steam::ITF_MOUSE) {
		return Steam::MOUSE_REPORT_DESCRIPTORS;
	}
	return PS5Usb::REPORT_DESCRIPTORS;
}

const uint8_t* SteamDevice::get_descriptor_configuration_cb(uint8_t index)
{
	(void)index;
	return Steam::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* SteamDevice::get_descriptor_device_qualifier_cb()
{
	return nullptr;
}
