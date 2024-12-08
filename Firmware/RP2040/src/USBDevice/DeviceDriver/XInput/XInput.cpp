#include <cstring>
#include <vector>

#include "USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h"
#include "USBDevice/DeviceDriver/XInput/XInput.h"

void XInputDevice::initialize() 
{
    class_driver_ = *tud_xinput::class_driver();
    std::memset(&in_report_, 0, sizeof(XInput::InReport));
    std::memset(&prev_in_report_, 0, sizeof(XInput::InReport));
    in_report_.report_size = XInput::ENDPOINT_IN_SIZE;
}

void XInputDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    std::memset(&in_report_.buttons, 0, sizeof(in_report_.buttons));

    switch (gamepad.get_dpad_buttons())
    {
        case Gamepad::DPad::UP:
            in_report_.buttons[0] = XInput::Buttons0::DPAD_UP;
            break;
        case Gamepad::DPad::DOWN:
            in_report_.buttons[0] = XInput::Buttons0::DPAD_DOWN;
            break;
        case Gamepad::DPad::LEFT:
            in_report_.buttons[0] = XInput::Buttons0::DPAD_LEFT;
            break;
        case Gamepad::DPad::RIGHT:
            in_report_.buttons[0] = XInput::Buttons0::DPAD_RIGHT;
            break;
        case Gamepad::DPad::UP_LEFT:
            in_report_.buttons[0] = (XInput::Buttons0::DPAD_UP | XInput::Buttons0::DPAD_LEFT);
            break;
        case Gamepad::DPad::UP_RIGHT:
            in_report_.buttons[0] = XInput::Buttons0::DPAD_UP | XInput::Buttons0::DPAD_RIGHT;
            break;
        case Gamepad::DPad::DOWN_LEFT:
            in_report_.buttons[0] = XInput::Buttons0::DPAD_DOWN | XInput::Buttons0::DPAD_LEFT;
            break;
        case Gamepad::DPad::DOWN_RIGHT:
            in_report_.buttons[0] = XInput::Buttons0::DPAD_DOWN | XInput::Buttons0::DPAD_RIGHT;
            break;
        default:
            break;
    }
    
    uint16_t gp_buttons = gamepad.get_buttons();

    if (gp_buttons & Gamepad::Button::BACK)     in_report_.buttons[0] |= XInput::Buttons0::BACK;
    if (gp_buttons & Gamepad::Button::START)    in_report_.buttons[0] |= XInput::Buttons0::START;
    if (gp_buttons & Gamepad::Button::L3)       in_report_.buttons[0] |= XInput::Buttons0::L3;
    if (gp_buttons & Gamepad::Button::R3)       in_report_.buttons[0] |= XInput::Buttons0::R3;

    if (gp_buttons & Gamepad::Button::X)        in_report_.buttons[1] |= XInput::Buttons1::X;
    if (gp_buttons & Gamepad::Button::A)        in_report_.buttons[1] |= XInput::Buttons1::A;
    if (gp_buttons & Gamepad::Button::Y)        in_report_.buttons[1] |= XInput::Buttons1::Y;
    if (gp_buttons & Gamepad::Button::B)        in_report_.buttons[1] |= XInput::Buttons1::B;
    if (gp_buttons & Gamepad::Button::LB)       in_report_.buttons[1] |= XInput::Buttons1::LB;
    if (gp_buttons & Gamepad::Button::RB)       in_report_.buttons[1] |= XInput::Buttons1::RB;
    if (gp_buttons & Gamepad::Button::SYS)      in_report_.buttons[1] |= XInput::Buttons1::HOME;

    in_report_.trigger_l = gamepad.get_trigger_l().uint8();
    in_report_.trigger_r = gamepad.get_trigger_r().uint8();

    in_report_.joystick_lx = gamepad.get_joystick_lx().int16();
    in_report_.joystick_ly = gamepad.get_joystick_ly().int16(true);
    in_report_.joystick_rx = gamepad.get_joystick_rx().int16();
    in_report_.joystick_ry = gamepad.get_joystick_ry().int16(true);

    if (std::memcmp(&prev_in_report_, &in_report_, sizeof(in_report_)) != 0 &&
        tud_xinput::send_report(reinterpret_cast<uint8_t*>(&in_report_), sizeof(XInput::InReport)))
    {
        std::memcpy(&prev_in_report_, &in_report_, sizeof(XInput::InReport));
    }

    if (tud_xinput::receive_report(reinterpret_cast<uint8_t*>(&out_report_), sizeof(XInput::OutReport)) &&
        out_report_.report_id == XInput::OutReportID::RUMBLE)
    {
        gamepad.set_rumble_l(out_report_.rumble_l);
        gamepad.set_rumble_r(out_report_.rumble_r);
    }
}

uint16_t XInputDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    std::memcpy(buffer, &in_report_, sizeof(in_report_));
	return sizeof(XInput::InReport);
}

void XInputDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool XInputDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * XInputDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = (const char *)XInput::STRING_DESCRIPTORS[index];
	return get_string_descriptor(value, index);
}

const uint8_t * XInputDevice::get_descriptor_device_cb() 
{
    return XInput::DEVICE_DESCRIPTORS;
}

const uint8_t * XInputDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * XInputDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return XInput::CONFIGURATION_DESCRIPTORS;
}

const uint8_t * XInputDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}