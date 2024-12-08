#include <cstring>
#include <vector>

#include "USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.h"
#include "USBDevice/DeviceDriver/XboxOG/XboxOG_GP.h"

void XboxOGDevice::initialize() 
{
    tud_xid::initialize(tud_xid::Type::GAMEPAD);
    class_driver_ = *tud_xid::class_driver();

    std::memset(&in_report_, 0, sizeof(XboxOG::GP::InReport));
    in_report_.report_len = sizeof(XboxOG::GP::InReport);

    std::memcpy(&prev_in_report_, &in_report_, sizeof(XboxOG::GP::InReport));
}

void XboxOGDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    std::memset(&in_report_.buttons, 0, 8);

    switch (gamepad.get_dpad_buttons())
    {
        case Gamepad::DPad::UP:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP;
            break;
        case Gamepad::DPad::DOWN:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN;
            break;
        case Gamepad::DPad::LEFT:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_LEFT;
            break;
        case Gamepad::DPad::RIGHT:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_RIGHT;
            break;
        case Gamepad::DPad::UP_LEFT:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP | XboxOG::GP::Buttons::DPAD_LEFT;
            break;
        case Gamepad::DPad::UP_RIGHT:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP | XboxOG::GP::Buttons::DPAD_RIGHT;
            break;
        case Gamepad::DPad::DOWN_LEFT:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN | XboxOG::GP::Buttons::DPAD_LEFT;
            break;
        case Gamepad::DPad::DOWN_RIGHT:
            in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN | XboxOG::GP::Buttons::DPAD_RIGHT;
            break;
        default:
            break;
    }
    
    uint16_t gp_buttons = gamepad.get_buttons();

    if (gp_buttons & Gamepad::Button::BACK)     in_report_.buttons |= XboxOG::GP::Buttons::BACK;
    if (gp_buttons & Gamepad::Button::START)    in_report_.buttons |= XboxOG::GP::Buttons::START;
    if (gp_buttons & Gamepad::Button::L3)       in_report_.buttons |= XboxOG::GP::Buttons::L3;
    if (gp_buttons & Gamepad::Button::R3)       in_report_.buttons |= XboxOG::GP::Buttons::R3;

    if (gamepad.analog_enabled())
    {
        in_report_.a = gamepad.get_analog_a();
        in_report_.b = gamepad.get_analog_b();
        in_report_.x = gamepad.get_analog_x();
        in_report_.y = gamepad.get_analog_y();
        in_report_.white = gamepad.get_analog_lb();
        in_report_.black = gamepad.get_analog_rb();
    }
    else
    {
        if (gp_buttons & Gamepad::Button::X)    in_report_.x = 0xFF;
        if (gp_buttons & Gamepad::Button::A)    in_report_.a = 0xFF;
        if (gp_buttons & Gamepad::Button::Y)    in_report_.y = 0xFF;
        if (gp_buttons & Gamepad::Button::B)    in_report_.b = 0xFF;
        if (gp_buttons & Gamepad::Button::LB)   in_report_.white = 0xFF;
        if (gp_buttons & Gamepad::Button::RB)   in_report_.black = 0xFF;
    }

    in_report_.trigger_l = gamepad.get_trigger_l().uint8();
    in_report_.trigger_r = gamepad.get_trigger_r().uint8();

    in_report_.joystick_lx = gamepad.get_joystick_lx().int16();
    in_report_.joystick_ly = gamepad.get_joystick_ly().int16(true);
    in_report_.joystick_rx = gamepad.get_joystick_rx().int16();
    in_report_.joystick_ry = gamepad.get_joystick_ry().int16(true);

    if (std::memcmp(&prev_in_report_, &in_report_, sizeof(in_report_)) != 0 &&
        tud_xid::send_report(0, reinterpret_cast<uint8_t*>(&in_report_), sizeof(XboxOG::GP::InReport)))
    {
        std::memcpy(&prev_in_report_, &in_report_, sizeof(XboxOG::GP::InReport));
    }

    if (tud_xid::receive_report(0, reinterpret_cast<uint8_t*>(&out_report_), sizeof(XboxOG::GP::OutReport)))
    {
        gamepad.set_rumble_l(out_report_.rumble_l);
        gamepad.set_rumble_r(out_report_.rumble_r);
    }
}

uint16_t XboxOGDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    std::memcpy(buffer, &in_report_, sizeof(in_report_));
	return sizeof(XboxOG::GP::InReport);
}

void XboxOGDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool XboxOGDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return tud_xid::class_driver()->control_xfer_cb(rhport, stage, request);
}

const uint16_t* XboxOGDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(XboxOG::GP::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* XboxOGDevice::get_descriptor_device_cb() 
{
    return reinterpret_cast<const uint8_t*>(&XboxOG::GP::DEVICE_DESCRIPTORS);
}

const uint8_t* XboxOGDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t* XboxOGDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return XboxOG::GP::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* XboxOGDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}