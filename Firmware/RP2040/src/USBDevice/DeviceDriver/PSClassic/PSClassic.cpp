#include <cstring>

#include "USBDevice/DeviceDriver/PSClassic/PSClassic.h"

void PSClassicDevice::initialize()
{
	class_driver_ = 
    {
		.name = TUD_DRV_NAME("PSClassic"),
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void PSClassicDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    if (gamepad.new_pad_in())
    {
        Gamepad::PadIn gp_in = gamepad.get_pad_in();
        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                in_report_.buttons = PSClassic::Buttons::UP;
                break;
            case Gamepad::DPAD_DOWN:
                in_report_.buttons = PSClassic::Buttons::DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                in_report_.buttons = PSClassic::Buttons::LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                in_report_.buttons = PSClassic::Buttons::RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                in_report_.buttons = PSClassic::Buttons::UP_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                in_report_.buttons = PSClassic::Buttons::UP_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                in_report_.buttons = PSClassic::Buttons::DOWN_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                in_report_.buttons = PSClassic::Buttons::DOWN_RIGHT;
                break;
            default:
                in_report_.buttons = PSClassic::Buttons::CENTER;
                break;
        }

        int16_t joy_lx = gp_in.joystick_lx;
        int16_t joy_ly = Range::invert(gp_in.joystick_ly);
        int16_t joy_rx = gp_in.joystick_rx;
        int16_t joy_ry = Range::invert(gp_in.joystick_ry);

        if (meets_pos_threshold(joy_lx, joy_rx))
        {
            if (meets_neg_45_threshold(joy_ly, joy_ry))
            {
                in_report_.buttons = PSClassic::Buttons::DOWN_RIGHT;
            }
            else if (meets_pos_45_threshold(joy_ly, joy_ry))
            {
                in_report_.buttons = PSClassic::Buttons::UP_RIGHT;
            }
            else
            {
                in_report_.buttons = PSClassic::Buttons::RIGHT;
            }
        }
        else if (meets_neg_threshold(joy_lx, joy_rx))
        {
            if (meets_neg_45_threshold(joy_ly, joy_ry))
            {
                in_report_.buttons = PSClassic::Buttons::DOWN_LEFT;
            }
            else if (meets_pos_45_threshold(joy_ly, joy_ry))
            {
                in_report_.buttons = PSClassic::Buttons::UP_LEFT;
            }
            else
            {
                in_report_.buttons = PSClassic::Buttons::LEFT;
            }
        }
        else if (meets_neg_threshold(joy_ly, joy_ry))
        {
            in_report_.buttons = PSClassic::Buttons::DOWN;
        }
        else if (meets_pos_threshold(joy_ly, joy_ry))
        {
            in_report_.buttons = PSClassic::Buttons::UP;
        }

        if (gp_in.buttons & Gamepad::BUTTON_A) in_report_.buttons |= PSClassic::Buttons::CROSS;
        if (gp_in.buttons & Gamepad::BUTTON_B) in_report_.buttons |= PSClassic::Buttons::CIRCLE;
        if (gp_in.buttons & Gamepad::BUTTON_X) in_report_.buttons |= PSClassic::Buttons::SQUARE;
        if (gp_in.buttons & Gamepad::BUTTON_Y) in_report_.buttons |= PSClassic::Buttons::TRIANGLE;
        if (gp_in.buttons & Gamepad::BUTTON_LB)    in_report_.buttons |= PSClassic::Buttons::L1;
        if (gp_in.buttons & Gamepad::BUTTON_RB)    in_report_.buttons |= PSClassic::Buttons::R1;
        if (gp_in.buttons & Gamepad::BUTTON_BACK)  in_report_.buttons |= PSClassic::Buttons::SELECT;
        if (gp_in.buttons & Gamepad::BUTTON_START) in_report_.buttons |= PSClassic::Buttons::START;
        
        if (gp_in.trigger_l) in_report_.buttons |= PSClassic::Buttons::L2;
        if (gp_in.trigger_r) in_report_.buttons |= PSClassic::Buttons::R2;
    }

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }
    if (tud_hid_n_ready(idx))
    {
        tud_hid_n_report(idx, 0, reinterpret_cast<uint8_t*>(&in_report_), sizeof(PSClassic::InReport));
    }
}

uint16_t PSClassicDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    std::memcpy(buffer, &in_report_, sizeof(PSClassic::InReport));
    return sizeof(PSClassic::InReport);
}

void PSClassicDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool PSClassicDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    return false;
}

const uint16_t* PSClassicDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    const char* value = reinterpret_cast<const char*>(PSClassic::STRING_DESCRIPTORS[index]);
    return get_string_descriptor(value, index);
}

const uint8_t* PSClassicDevice::get_descriptor_device_cb()
{
    return PSClassic::DEVICE_DESCRIPTORS;
}

const uint8_t* PSClassicDevice::get_hid_descriptor_report_cb(uint8_t itf)
{
    return PSClassic::REPORT_DESCRIPTORS;
}

const uint8_t* PSClassicDevice::get_descriptor_configuration_cb(uint8_t index)
{
    return PSClassic::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* PSClassicDevice::get_descriptor_device_qualifier_cb()
{
    return nullptr;
}