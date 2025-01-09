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
}

void XboxOGDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    if (gamepad.new_pad_in())
    {
        std::memset(&in_report_.buttons, 0, 8);
        Gamepad::PadIn gp_in = gamepad.get_pad_in();

        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP;
                break;
            case Gamepad::DPAD_DOWN:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP | XboxOG::GP::Buttons::DPAD_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP | XboxOG::GP::Buttons::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN | XboxOG::GP::Buttons::DPAD_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN | XboxOG::GP::Buttons::DPAD_RIGHT;
                break;
            default:
                break;
        }
        
        if (gp_in.buttons & Gamepad::BUTTON_BACK)     in_report_.buttons |= XboxOG::GP::Buttons::BACK;
        if (gp_in.buttons & Gamepad::BUTTON_START)    in_report_.buttons |= XboxOG::GP::Buttons::START;
        if (gp_in.buttons & Gamepad::BUTTON_L3)       in_report_.buttons |= XboxOG::GP::Buttons::L3;
        if (gp_in.buttons & Gamepad::BUTTON_R3)       in_report_.buttons |= XboxOG::GP::Buttons::R3;

        if (gamepad.analog_enabled())
        {
            in_report_.a = gp_in.analog[Gamepad::ANALOG_OFF_A];
            in_report_.b = gp_in.analog[Gamepad::ANALOG_OFF_B];
            in_report_.x = gp_in.analog[Gamepad::ANALOG_OFF_X];
            in_report_.y = gp_in.analog[Gamepad::ANALOG_OFF_Y];
            in_report_.white = gp_in.analog[Gamepad::ANALOG_OFF_LB];
            in_report_.black = gp_in.analog[Gamepad::ANALOG_OFF_RB];
        }
        else
        {
            if (gp_in.buttons & Gamepad::BUTTON_X)    in_report_.x = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_A)    in_report_.a = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_Y)    in_report_.y = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_B)    in_report_.b = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_LB)   in_report_.white = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_RB)   in_report_.black = 0xFF;
        }

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
        if (tud_xid::send_report_ready(0))
        {
            tud_xid::send_report(0, reinterpret_cast<uint8_t*>(&in_report_), sizeof(XboxOG::GP::InReport));
        }
    }

    if (tud_xid::receive_report(0, reinterpret_cast<uint8_t*>(&out_report_), sizeof(XboxOG::GP::OutReport)))
    {
        Gamepad::PadOut gp_out;
        gp_out.rumble_l = Scale::uint16_to_uint8(out_report_.rumble_l);
        gp_out.rumble_r = Scale::uint16_to_uint8(out_report_.rumble_r);
        gamepad.set_pad_out(gp_out);
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