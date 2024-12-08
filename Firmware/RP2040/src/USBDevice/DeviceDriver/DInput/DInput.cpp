#include <cstring>

#include "class/hid/hid_device.h"

#include "Descriptors/PS3.h"
#include "USBDevice/DeviceDriver/DInput/DInput.h"

bool DInputDevice::control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
	if (request->bmRequestType == 0xA1 &&
		request->bRequest == HID_REQ_CONTROL_GET_REPORT &&
		request->wValue == 0x0300 ) 
    {
		return tud_control_xfer(rhport, request, const_cast<void*>(static_cast<const void*>(PS3::MAGIC_BYTES)), sizeof(PS3::MAGIC_BYTES));
	} 
    return hidd_control_xfer_cb(rhport, stage, request);
}

void DInputDevice::initialize() 
{
    std::memset(&in_report_, 0, sizeof(DInput::InReport));
    in_report_.dpad = DInput::DPad::CENTER;
    in_report_.joystick_lx = DInput::AXIS_MID;
    in_report_.joystick_ly = DInput::AXIS_MID;
    in_report_.joystick_rx = DInput::AXIS_MID;
    in_report_.joystick_ry = DInput::AXIS_MID;

    for (auto& prev_in_report : prev_in_reports_)
    {
        std::memcpy(&prev_in_report, &in_report_, sizeof(DInput::InReport));
    }

	class_driver_ = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "HID",
	#endif
		.init = hidd_init,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = DInputDevice::control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void DInputDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    switch (gamepad.get_dpad_buttons())
    {
        case Gamepad::DPad::UP:
            in_report_.dpad = DInput::DPad::UP;
            break;
        case Gamepad::DPad::DOWN:
            in_report_.dpad = DInput::DPad::DOWN;
            break;
        case Gamepad::DPad::LEFT:
            in_report_.dpad = DInput::DPad::LEFT;
            break;
        case Gamepad::DPad::RIGHT:
            in_report_.dpad = DInput::DPad::RIGHT;
            break;
        case Gamepad::DPad::UP_LEFT:
            in_report_.dpad = DInput::DPad::UP_LEFT;
            break;
        case Gamepad::DPad::UP_RIGHT:
            in_report_.dpad = DInput::DPad::UP_RIGHT;
            break;
        case Gamepad::DPad::DOWN_LEFT:
            in_report_.dpad = DInput::DPad::DOWN_LEFT;
            break;
        case Gamepad::DPad::DOWN_RIGHT:
            in_report_.dpad = DInput::DPad::DOWN_RIGHT;
            break;
        default:
            in_report_.dpad = DInput::DPad::CENTER;
            break;
    }

    std::memset(in_report_.buttons, 0, sizeof(in_report_.buttons));

    uint16_t gamepad_buttons = gamepad.get_buttons();

    if (gamepad_buttons & Gamepad::Button::A)   in_report_.buttons[0] |= DInput::Buttons0::CROSS;
    if (gamepad_buttons & Gamepad::Button::B)   in_report_.buttons[0] |= DInput::Buttons0::CIRCLE;
    if (gamepad_buttons & Gamepad::Button::X)   in_report_.buttons[0] |= DInput::Buttons0::SQUARE;
    if (gamepad_buttons & Gamepad::Button::Y)   in_report_.buttons[0] |= DInput::Buttons0::TRIANGLE;
    if (gamepad_buttons & Gamepad::Button::LB)  in_report_.buttons[0] |= DInput::Buttons0::L1;
    if (gamepad_buttons & Gamepad::Button::RB)  in_report_.buttons[0] |= DInput::Buttons0::R1;

    if (gamepad_buttons & Gamepad::Button::L3)    in_report_.buttons[1] |= DInput::Buttons1::L3;
    if (gamepad_buttons & Gamepad::Button::R3)    in_report_.buttons[1] |= DInput::Buttons1::R3;
    if (gamepad_buttons & Gamepad::Button::BACK)  in_report_.buttons[1] |= DInput::Buttons1::SELECT;
    if (gamepad_buttons & Gamepad::Button::START) in_report_.buttons[1] |= DInput::Buttons1::START;
    if (gamepad_buttons & Gamepad::Button::SYS)   in_report_.buttons[1] |= DInput::Buttons1::PS;
    if (gamepad_buttons & Gamepad::Button::MISC)  in_report_.buttons[1] |= DInput::Buttons1::TP;

    if (gamepad.analog_enabled())
    {
        in_report_.up_axis = gamepad.get_analog_up();
        in_report_.down_axis = gamepad.get_analog_down();
        in_report_.right_axis = gamepad.get_analog_right();
        in_report_.left_axis = gamepad.get_analog_left();

        in_report_.triangle_axis = gamepad.get_analog_y();
        in_report_.circle_axis = gamepad.get_analog_x();
        in_report_.cross_axis = gamepad.get_analog_b();
        in_report_.square_axis = gamepad.get_analog_a();

        in_report_.r1_axis = gamepad.get_analog_rb();
        in_report_.l1_axis = gamepad.get_analog_lb();
    }

    in_report_.joystick_lx = gamepad.get_joystick_lx().uint8();
    in_report_.joystick_ly = gamepad.get_joystick_ly().uint8();
    in_report_.joystick_rx = gamepad.get_joystick_rx().uint8();
    in_report_.joystick_ry = gamepad.get_joystick_ry().uint8();

    in_report_.l2_axis = gamepad.get_trigger_l().uint8();
    in_report_.r2_axis = gamepad.get_trigger_r().uint8();

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }

    if (tud_hid_n_ready(idx) &&
        // std::memcmp(&in_report_, &prev_in_reports_[idx], sizeof(DInput::InReport)) != 0 && 
        tud_hid_n_report(idx, 0, reinterpret_cast<void*>(&in_report_), sizeof(DInput::InReport)))
    {
        std::memcpy(&prev_in_reports_[idx], &in_report_, sizeof(DInput::InReport));
    }
}

uint16_t DInputDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    std::memcpy(buffer, &in_report_, sizeof(DInput::InReport));
    return sizeof(DInput::InReport);
}

void DInputDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool DInputDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    return false;
}

const uint16_t* DInputDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    const char* value = reinterpret_cast<const char*>(DInput::STRING_DESCRIPTORS[index]);
    return get_string_descriptor(value, index);
}

const uint8_t* DInputDevice::get_descriptor_device_cb()
{
    return DInput::DEVICE_DESCRIPTORS;
}

const uint8_t* DInputDevice::get_hid_descriptor_report_cb(uint8_t itf)
{
    return DInput::REPORT_DESCRIPTORS;
}

const uint8_t* DInputDevice::get_descriptor_configuration_cb(uint8_t index)
{
    return DInput::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* DInputDevice::get_descriptor_device_qualifier_cb()
{
    return nullptr;
}