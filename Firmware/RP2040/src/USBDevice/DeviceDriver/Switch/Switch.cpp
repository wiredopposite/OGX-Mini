#include <cstring>

#include "USBDevice/DeviceDriver/Switch/Switch.h"

void SwitchDevice::initialize() 
{
	class_driver_ = 
    {
		.name = TUD_DRV_NAME("SWITCH"),
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};

    in_report_.fill(SwitchWired::InReport());
}

void SwitchDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    SwitchWired::InReport& in_report = in_report_[idx];

    if (gamepad.new_pad_in())
    {
        Gamepad::PadIn gp_in = gamepad.get_pad_in();
    
        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                in_report.dpad = SwitchWired::DPad::UP;
                break;
            case Gamepad::DPAD_DOWN:
                in_report.dpad = SwitchWired::DPad::DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                in_report.dpad = SwitchWired::DPad::LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                in_report.dpad = SwitchWired::DPad::RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                in_report.dpad = SwitchWired::DPad::UP_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                in_report.dpad = SwitchWired::DPad::UP_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                in_report.dpad = SwitchWired::DPad::DOWN_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                in_report.dpad = SwitchWired::DPad::DOWN_RIGHT;
                break;
            default:
                in_report.dpad = SwitchWired::DPad::CENTER;
                break;
        }

        in_report.buttons = 0;

        if (gp_in.buttons & Gamepad::BUTTON_X)        in_report.buttons |= SwitchWired::Buttons::Y;
        if (gp_in.buttons & Gamepad::BUTTON_A)        in_report.buttons |= SwitchWired::Buttons::B;
        if (gp_in.buttons & Gamepad::BUTTON_Y)        in_report.buttons |= SwitchWired::Buttons::X;
        if (gp_in.buttons & Gamepad::BUTTON_B)        in_report.buttons |= SwitchWired::Buttons::A;
        if (gp_in.buttons & Gamepad::BUTTON_LB)       in_report.buttons |= SwitchWired::Buttons::L;
        if (gp_in.buttons & Gamepad::BUTTON_RB)       in_report.buttons |= SwitchWired::Buttons::R;
        if (gp_in.buttons & Gamepad::BUTTON_BACK)     in_report.buttons |= SwitchWired::Buttons::MINUS;
        if (gp_in.buttons & Gamepad::BUTTON_START)    in_report.buttons |= SwitchWired::Buttons::PLUS;
        if (gp_in.buttons & Gamepad::BUTTON_L3)       in_report.buttons |= SwitchWired::Buttons::L3;
        if (gp_in.buttons & Gamepad::BUTTON_R3)       in_report.buttons |= SwitchWired::Buttons::R3;
        if (gp_in.buttons & Gamepad::BUTTON_SYS)      in_report.buttons |= SwitchWired::Buttons::HOME;
        if (gp_in.buttons & Gamepad::BUTTON_MISC)     in_report.buttons |= SwitchWired::Buttons::CAPTURE;

        if (gp_in.trigger_l) in_report.buttons |= SwitchWired::Buttons::ZL;
        if (gp_in.trigger_r) in_report.buttons |= SwitchWired::Buttons::ZR;

        in_report.joystick_lx = Scale::int16_to_uint8(gp_in.joystick_lx);
        in_report.joystick_ly = Scale::int16_to_uint8(gp_in.joystick_ly);
        in_report.joystick_rx = Scale::int16_to_uint8(gp_in.joystick_rx);
        in_report.joystick_ry = Scale::int16_to_uint8(gp_in.joystick_ry);
    }

	if (tud_suspended())
    {
		tud_remote_wakeup();
    }
	if (tud_hid_n_ready(idx))
    {
        tud_hid_n_report(idx, 0, reinterpret_cast<uint8_t*>(&in_report), sizeof(SwitchWired::InReport));
    }
}

uint16_t SwitchDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    if (report_type != HID_REPORT_TYPE_INPUT || itf >= MAX_GAMEPADS)
    {
        return 0;
    }
    std::memcpy(buffer, &in_report_[itf], sizeof(SwitchWired::InReport));
	return sizeof(SwitchWired::InReport);
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