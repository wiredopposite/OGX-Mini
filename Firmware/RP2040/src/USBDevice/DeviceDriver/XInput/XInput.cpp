#include <cstring>

#include "USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h"
#include "USBDevice/DeviceDriver/XInput/XInput.h"

void XInputDevice::initialize() 
{
    class_driver_ = *tud_xinput::class_driver();
}

void XInputDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    if (gamepad.new_pad_in())
    {
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

        if (tud_suspended())
        {
            tud_remote_wakeup();
        }

        tud_xinput::send_report((uint8_t*)&in_report_, sizeof(XInput::InReport));
    }

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
    return false;
}

const uint16_t * XInputDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
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
    return XInput::DESC_CONFIGURATION;
}

const uint8_t * XInputDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}