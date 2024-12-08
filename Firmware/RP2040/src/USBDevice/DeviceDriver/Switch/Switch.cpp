#include <cstring>

#include "USBDevice/DeviceDriver/Switch/Switch.h"

void SwitchDevice::initialize() 
{
	class_driver_ = 
    {
	#if CFG_TUSB_DEBUG >= 2
		.name = "SWITCH",
	#endif
		.init = hidd_init,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};

    in_report_ = SwitchWired::InReport();
    for (auto& prev_in_report : prev_in_reports_)
    {
        prev_in_report = SwitchWired::InReport();
    }
}

void SwitchDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    switch (gamepad.get_dpad_buttons())
    {
        case Gamepad::DPad::UP:
            in_report_.dpad = SwitchWired::DPad::UP;
            break;
        case Gamepad::DPad::DOWN:
            in_report_.dpad = SwitchWired::DPad::DOWN;
            break;
        case Gamepad::DPad::LEFT:
            in_report_.dpad = SwitchWired::DPad::LEFT;
            break;
        case Gamepad::DPad::RIGHT:
            in_report_.dpad = SwitchWired::DPad::RIGHT;
            break;
        case Gamepad::DPad::UP_LEFT:
            in_report_.dpad = SwitchWired::DPad::UP_LEFT;
            break;
        case Gamepad::DPad::UP_RIGHT:
            in_report_.dpad = SwitchWired::DPad::UP_RIGHT;
            break;
        case Gamepad::DPad::DOWN_LEFT:
            in_report_.dpad = SwitchWired::DPad::DOWN_LEFT;
            break;
        case Gamepad::DPad::DOWN_RIGHT:
            in_report_.dpad = SwitchWired::DPad::DOWN_RIGHT;
            break;
        default:
            in_report_.dpad = SwitchWired::DPad::CENTER;
            break;
    }

    in_report_.buttons = 0;
    
    uint16_t gp_buttons = gamepad.get_buttons();

    if (gp_buttons & Gamepad::Button::X)        in_report_.buttons |= SwitchWired::Buttons::Y;
    if (gp_buttons & Gamepad::Button::A)        in_report_.buttons |= SwitchWired::Buttons::B;
    if (gp_buttons & Gamepad::Button::Y)        in_report_.buttons |= SwitchWired::Buttons::X;
    if (gp_buttons & Gamepad::Button::B)        in_report_.buttons |= SwitchWired::Buttons::A;
    if (gp_buttons & Gamepad::Button::LB)       in_report_.buttons |= SwitchWired::Buttons::L;
    if (gp_buttons & Gamepad::Button::RB)       in_report_.buttons |= SwitchWired::Buttons::R;
    if (gp_buttons & Gamepad::Button::BACK)     in_report_.buttons |= SwitchWired::Buttons::MINUS;
    if (gp_buttons & Gamepad::Button::START)    in_report_.buttons |= SwitchWired::Buttons::PLUS;
    if (gp_buttons & Gamepad::Button::L3)       in_report_.buttons |= SwitchWired::Buttons::L3;
    if (gp_buttons & Gamepad::Button::R3)       in_report_.buttons |= SwitchWired::Buttons::R3;
    if (gp_buttons & Gamepad::Button::SYS)      in_report_.buttons |= SwitchWired::Buttons::HOME;
    if (gp_buttons & Gamepad::Button::MISC)     in_report_.buttons |= SwitchWired::Buttons::CAPTURE;

    if (gamepad.get_trigger_l().uint8()) in_report_.buttons |= SwitchWired::Buttons::ZL;
    if (gamepad.get_trigger_r().uint8()) in_report_.buttons |= SwitchWired::Buttons::ZR;

    in_report_.joystick_lx = gamepad.get_joystick_lx().uint8();
    in_report_.joystick_ly = gamepad.get_joystick_ly().uint8();
    in_report_.joystick_rx = gamepad.get_joystick_rx().uint8();
    in_report_.joystick_ry = gamepad.get_joystick_ry().uint8();

	if (tud_suspended())
    {
		tud_remote_wakeup();
    }

	if (std::memcmp(&prev_in_reports_[idx], &in_report_, sizeof(SwitchWired::InReport)) != 0 &&
        tud_hid_n_ready(idx) &&
        tud_hid_n_report(idx, 0, reinterpret_cast<uint8_t*>(&in_report_), sizeof(SwitchWired::InReport)))
	{
		std::memcpy(&prev_in_reports_[idx], &in_report_, sizeof(SwitchWired::InReport));
	}
}

uint16_t SwitchDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    // std::memcpy(buffer, &in_report_, sizeof(SwitchWired::InReport));
	// return sizeof(SwitchWired::InReport);
    return 0;
}

void SwitchDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool SwitchDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t* SwitchDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(SwitchWired::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* SwitchDevice::get_descriptor_device_cb() 
{
    return SwitchWired::DEVICE_DESCRIPTORS;
}

const uint8_t* SwitchDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return SwitchWired::REPORT_DESCRIPTORS;
}

const uint8_t* SwitchDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return SwitchWired::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* SwitchDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}