#include <cstring>
#include <algorithm>

#include "USBDevice/DeviceDriver/PS3/PS3.h"

void PS3Device::initialize() 
{
	class_driver_ = 
    {
		.name = TUD_DRV_NAME("PS3"),
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void PS3Device::process(const uint8_t idx, Gamepad& gamepad) 
{
    if (gamepad.new_pad_in())
    {
        Gamepad::PadIn gp_in = gamepad.get_pad_in();
        report_in_ = PS3::InReport();

        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_UP;
                break;
            case Gamepad::DPAD_DOWN:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_UP | PS3::Buttons0::DPAD_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_UP | PS3::Buttons0::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_DOWN | PS3::Buttons0::DPAD_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_DOWN | PS3::Buttons0::DPAD_RIGHT;
                break;
            default:
                break;
        }

        if (gp_in.buttons & Gamepad::BUTTON_X)        report_in_.buttons[1] |= PS3::Buttons1::SQUARE;
        if (gp_in.buttons & Gamepad::BUTTON_A)        report_in_.buttons[1] |= PS3::Buttons1::CROSS;
        if (gp_in.buttons & Gamepad::BUTTON_Y)        report_in_.buttons[1] |= PS3::Buttons1::TRIANGLE;
        if (gp_in.buttons & Gamepad::BUTTON_B)        report_in_.buttons[1] |= PS3::Buttons1::CIRCLE;
        if (gp_in.buttons & Gamepad::BUTTON_LB)       report_in_.buttons[1] |= PS3::Buttons1::L1;
        if (gp_in.buttons & Gamepad::BUTTON_RB)       report_in_.buttons[1] |= PS3::Buttons1::R1;
        if (gp_in.buttons & Gamepad::BUTTON_BACK)     report_in_.buttons[0] |= PS3::Buttons0::SELECT;
        if (gp_in.buttons & Gamepad::BUTTON_START)    report_in_.buttons[0] |= PS3::Buttons0::START;
        if (gp_in.buttons & Gamepad::BUTTON_L3)       report_in_.buttons[0] |= PS3::Buttons0::L3;
        if (gp_in.buttons & Gamepad::BUTTON_R3)       report_in_.buttons[0] |= PS3::Buttons0::R3;
        if (gp_in.buttons & Gamepad::BUTTON_SYS)      report_in_.buttons[2] |= PS3::Buttons2::SYS;
        if (gp_in.buttons & Gamepad::BUTTON_MISC)     report_in_.buttons[2] |= PS3::Buttons2::TP;

        if (gp_in.trigger_l) report_in_.buttons[1] |= PS3::Buttons1::L2;
        if (gp_in.trigger_r) report_in_.buttons[1] |= PS3::Buttons1::R2;

        report_in_.joystick_lx = Scale::int16_to_uint8(gp_in.joystick_lx);
        report_in_.joystick_ly = Scale::int16_to_uint8(gp_in.joystick_ly);
        report_in_.joystick_rx = Scale::int16_to_uint8(gp_in.joystick_rx);
        report_in_.joystick_ry = Scale::int16_to_uint8(gp_in.joystick_ry);

        if (gamepad.analog_enabled())
        {
            report_in_.up_axis      = gp_in.analog[Gamepad::ANALOG_OFF_UP];
            report_in_.down_axis    = gp_in.analog[Gamepad::ANALOG_OFF_DOWN];
            report_in_.right_axis   = gp_in.analog[Gamepad::ANALOG_OFF_RIGHT];
            report_in_.left_axis    = gp_in.analog[Gamepad::ANALOG_OFF_LEFT];

            report_in_.triangle_axis = gp_in.analog[Gamepad::ANALOG_OFF_Y];
            report_in_.circle_axis   = gp_in.analog[Gamepad::ANALOG_OFF_B];
            report_in_.cross_axis    = gp_in.analog[Gamepad::ANALOG_OFF_A];
            report_in_.square_axis   = gp_in.analog[Gamepad::ANALOG_OFF_X];

            report_in_.r1_axis = gp_in.analog[Gamepad::ANALOG_OFF_RB];
            report_in_.l1_axis = gp_in.analog[Gamepad::ANALOG_OFF_LB];
        }
        else
        {
            report_in_.up_axis       = (gp_in.dpad & Gamepad::DPAD_UP)    ? 0xFF : 0;
            report_in_.down_axis     = (gp_in.dpad & Gamepad::DPAD_DOWN)  ? 0xFF : 0;
            report_in_.right_axis    = (gp_in.dpad & Gamepad::DPAD_RIGHT) ? 0xFF : 0;
            report_in_.left_axis     = (gp_in.dpad & Gamepad::DPAD_LEFT)  ? 0xFF : 0;

            report_in_.triangle_axis = (gp_in.buttons & Gamepad::BUTTON_Y) ? 0xFF : 0;
            report_in_.circle_axis   = (gp_in.buttons & Gamepad::BUTTON_X) ? 0xFF : 0;
            report_in_.cross_axis    = (gp_in.buttons & Gamepad::BUTTON_B) ? 0xFF : 0;
            report_in_.square_axis   = (gp_in.buttons & Gamepad::BUTTON_A) ? 0xFF : 0;

            report_in_.r1_axis = (gp_in.buttons & Gamepad::BUTTON_RB) ? 0xFF : 0;
            report_in_.l1_axis = (gp_in.buttons & Gamepad::BUTTON_LB) ? 0xFF : 0;
        }
    }

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }

    if (tud_hid_ready())
    {
        //PS3 seems to start using stale data if a report isn't sent every frame
        tud_hid_report(0, reinterpret_cast<uint8_t*>(&report_in_), sizeof(PS3::InReport));
    }

    if (new_report_out_)
    {
        Gamepad::PadOut gp_out;
        gp_out.rumble_l = report_out_.rumble.left_motor_force;
        gp_out.rumble_r = report_out_.rumble.right_motor_on ? Range::MAX<uint8_t> : 0;
        gamepad.set_pad_out(gp_out);
        new_report_out_ = false;
    }
}

uint16_t PS3Device::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    if (report_type == HID_REPORT_TYPE_INPUT) 
    {
        std::memcpy(buffer, &report_in_, sizeof(PS3::InReport));
        return sizeof(PS3::InReport);
    } 
    else if (report_type == HID_REPORT_TYPE_FEATURE) 
    {
        uint16_t resp_len = 0;
        uint8_t ctr = 0;

        switch(report_id) 
        {
            case PS3::ReportID::FEATURE_01:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0x01, resp_len);
                return resp_len;
            case PS3::ReportID::FEATURE_EF:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xEF, resp_len);
                buffer[6] = ef_byte_;
                return resp_len;
            case PS3::ReportID::GET_PAIRING_INFO:
                resp_len = reqlen;
                std::memcpy(buffer, &bt_info_, resp_len);
                return resp_len;
            case PS3::ReportID::FEATURE_F5:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xF5, resp_len);
                for (ctr = 0; ctr < 6; ctr++) 
                {
                    buffer[1+ctr] = bt_info_.host_address[ctr];
                }
                return resp_len;
            case PS3::ReportID::FEATURE_F7:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xF7, resp_len);
                return resp_len;
            case PS3::ReportID::FEATURE_F8:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xF8, resp_len);
                buffer[6] = ef_byte_;
                return resp_len;
        }
    }
    return 0;
}

void PS3Device::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{
    if (report_type == HID_REPORT_TYPE_FEATURE) 
    {
        switch(report_id) 
        {
            case PS3::ReportID::FEATURE_EF:
                ef_byte_ = buffer[6];
                break;
        }
    } 
    else if (report_type == HID_REPORT_TYPE_OUTPUT) 
    {
        // DS3 command
        uint8_t const *buf = buffer;
        if (report_id == 0 && bufsize > 0) 
        {
            report_id = buffer[0];
            bufsize = bufsize - 1;
            buf = &buffer[1];
        }
        switch(report_id) 
        {
            case PS3::ReportID::FEATURE_01:
                new_report_out_ = true;
                std::memcpy(&report_out_, buf, std::min(bufsize, static_cast<uint16_t>(sizeof(PS3::OutReport))));
                break;
        }
    }
}

bool PS3Device::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t* PS3Device::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(PS3::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* PS3Device::get_descriptor_device_cb() 
{
    return PS3::DEVICE_DESCRIPTORS;
}

const uint8_t* PS3Device::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return PS3::REPORT_DESCRIPTORS;
}

const uint8_t* PS3Device::get_descriptor_configuration_cb(uint8_t index) 
{
    return PS3::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* PS3Device::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}