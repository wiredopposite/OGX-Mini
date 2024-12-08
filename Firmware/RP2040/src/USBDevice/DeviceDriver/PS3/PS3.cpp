#include <cstring>

#include "USBDevice/DeviceDriver/PS3/PS3.h"

void PS3Device::initialize() 
{
	class_driver_ = 
    {
	#if CFG_TUSB_DEBUG >= 2
		.name = "PS3",
	#endif
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};

    in_report_ = PS3::InReport();

    // bt_info_ = 
    // {
    //     .reserved0 = {0xFF,0xFF},
    //     .device_address = { 0x00, 0x20, 0x40, 0xCE, 0x00, 0x00, 0x00 },
    //     .host_address = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
    // };

    // for (uint8_t addr = 0; addr < 3; addr++) 
    // {
    //     bt_info_.device_address[4 + addr] = static_cast<uint8_t>(get_rand_32() % 0xFF);
    // }

    // for (uint8_t addr = 0; addr < 6; addr++) 
    // {
    //     bt_info_.host_address[1 + addr] = static_cast<uint8_t>(get_rand_32() % 0xFF);
    // }
}

void PS3Device::process(const uint8_t idx, Gamepad& gamepad) 
{
    uint8_t gp_dpad = gamepad.get_dpad_buttons();
    uint16_t gp_buttons = gamepad.get_buttons();
    uint8_t gp_trigger_l = gamepad.get_trigger_l().uint8();
    uint8_t gp_trigger_r = gamepad.get_trigger_r().uint8();

    std::memset(in_report_.buttons, 0, sizeof(in_report_.buttons));

    switch (gp_dpad)
    {
        case Gamepad::DPad::UP:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_UP;
            break;
        case Gamepad::DPad::DOWN:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_DOWN;
            break;
        case Gamepad::DPad::LEFT:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_LEFT;
            break;
        case Gamepad::DPad::RIGHT:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_RIGHT;
            break;
        case Gamepad::DPad::UP_LEFT:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_UP | PS3::Buttons0::DPAD_LEFT;
            break;
        case Gamepad::DPad::UP_RIGHT:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_UP | PS3::Buttons0::DPAD_RIGHT;
            break;
        case Gamepad::DPad::DOWN_LEFT:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_DOWN | PS3::Buttons0::DPAD_LEFT;
            break;
        case Gamepad::DPad::DOWN_RIGHT:
            in_report_.buttons[0] = PS3::Buttons0::DPAD_DOWN | PS3::Buttons0::DPAD_RIGHT;
            break;
        default:
            break;
    }

    if (gp_buttons & Gamepad::Button::X)        in_report_.buttons[1] |= PS3::Buttons1::SQUARE;
    if (gp_buttons & Gamepad::Button::A)        in_report_.buttons[1] |= PS3::Buttons1::CROSS;
    if (gp_buttons & Gamepad::Button::Y)        in_report_.buttons[1] |= PS3::Buttons1::TRIANGLE;
    if (gp_buttons & Gamepad::Button::B)        in_report_.buttons[1] |= PS3::Buttons1::CIRCLE;
    if (gp_buttons & Gamepad::Button::LB)       in_report_.buttons[1] |= PS3::Buttons1::L1;
    if (gp_buttons & Gamepad::Button::RB)       in_report_.buttons[1] |= PS3::Buttons1::R1;
    if (gp_buttons & Gamepad::Button::BACK)     in_report_.buttons[0] |= PS3::Buttons0::SELECT;
    if (gp_buttons & Gamepad::Button::START)    in_report_.buttons[0] |= PS3::Buttons0::START;
    if (gp_buttons & Gamepad::Button::L3)       in_report_.buttons[0] |= PS3::Buttons0::L3;
    if (gp_buttons & Gamepad::Button::R3)       in_report_.buttons[0] |= PS3::Buttons0::R3;
    if (gp_buttons & Gamepad::Button::SYS)      in_report_.buttons[2] |= PS3::Buttons2::PS;
    if (gp_buttons & Gamepad::Button::MISC)     in_report_.buttons[2] |= PS3::Buttons2::TP;

    if (gp_trigger_l > 0) in_report_.buttons[1] |= PS3::Buttons1::L2;
    if (gp_trigger_r > 0) in_report_.buttons[1] |= PS3::Buttons1::R2;

    in_report_.joystick_lx = gamepad.get_joystick_lx().uint8();
    in_report_.joystick_ly = gamepad.get_joystick_ly().uint8();
    in_report_.joystick_rx = gamepad.get_joystick_rx().uint8();
    in_report_.joystick_ry = gamepad.get_joystick_ry().uint8();

    if (gamepad.analog_enabled())
    {
        in_report_.up_axis      = gamepad.get_analog_up();
        in_report_.down_axis    = gamepad.get_analog_down();
        in_report_.right_axis   = gamepad.get_analog_right();
        in_report_.left_axis    = gamepad.get_analog_left();

        in_report_.triangle_axis = gamepad.get_analog_y();
        in_report_.circle_axis   = gamepad.get_analog_x();
        in_report_.cross_axis    = gamepad.get_analog_b();
        in_report_.square_axis   = gamepad.get_analog_a();

        in_report_.r1_axis = gamepad.get_analog_rb();
        in_report_.l1_axis = gamepad.get_analog_lb();
    }
    else
    {
        in_report_.up_axis       = (gp_dpad & Gamepad::DPad::UP)    ? 0xFF : 0;
        in_report_.down_axis     = (gp_dpad & Gamepad::DPad::DOWN)  ? 0xFF : 0;
        in_report_.right_axis    = (gp_dpad & Gamepad::DPad::RIGHT) ? 0xFF : 0;
        in_report_.left_axis     = (gp_dpad & Gamepad::DPad::LEFT)  ? 0xFF : 0;

        in_report_.triangle_axis = (gp_buttons & Gamepad::Button::Y) ? 0xFF : 0;
        in_report_.circle_axis   = (gp_buttons & Gamepad::Button::X) ? 0xFF : 0;
        in_report_.cross_axis    = (gp_buttons & Gamepad::Button::B) ? 0xFF : 0;
        in_report_.square_axis   = (gp_buttons & Gamepad::Button::A) ? 0xFF : 0;

        in_report_.r1_axis = (gp_buttons & Gamepad::Button::RB) ? 0xFF : 0;
        in_report_.l1_axis = (gp_buttons & Gamepad::Button::LB) ? 0xFF : 0;
    }

	if (tud_suspended())
    {
		tud_remote_wakeup();
    }

	if (std::memcmp(&prev_in_report_, &in_report_, sizeof(PS3::InReport)) != 0 &&
        tud_hid_ready() &&
        tud_hid_report(0, reinterpret_cast<uint8_t*>(&in_report_), sizeof(PS3::InReport)))
	{
		std::memcpy(&prev_in_report_, &in_report_, sizeof(PS3::InReport));
	}

    if (new_out_report_)
    {
        gamepad.set_rumble_l(out_report_.rumble.left_motor_force);
        gamepad.set_rumble_r(out_report_.rumble.right_motor_on);
        new_out_report_ = false;
    }
}

static constexpr uint8_t output_ps3_0x01[] = 
{
    0x01, 0x04, 0x00, 0x0b, 0x0c, 0x01, 0x02, 0x18, 
    0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11, 0x12,
    0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02,
    0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04, 0x04,
    0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01, 0x02,
    0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// calibration data
static constexpr uint8_t output_ps3_0xef[] = 
{
    0xef, 0x04, 0x00, 0x0b, 0x03, 0x01, 0xa0, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff,
    0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
};

// unknown
static constexpr uint8_t output_ps3_0xf5[] = 
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // host address - must match 0xf2
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// unknown
static constexpr uint8_t output_ps3_0xf7[] = 
{
    0x02, 0x01, 0xf8, 0x02, 0xe2, 0x01, 0x05, 0xff,
    0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// unknown
static constexpr uint8_t output_ps3_0xf8[] = 
{
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Based on: https://github.com/OpenStickCommunity/GP2040-CE/blob/main/src/drivers/ps3/PS3Driver.cpp */

uint16_t PS3Device::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    if (report_type == HID_REPORT_TYPE_INPUT) 
    {
        std::memcpy(buffer, &in_report_, sizeof(PS3::InReport));
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
                std::memcpy(buffer, output_ps3_0x01, resp_len);
                return resp_len;

            case PS3::ReportID::FEATURE_EF:
                resp_len = reqlen;
                std::memcpy(buffer, output_ps3_0xef, resp_len);
                buffer[6] = ef_byte_;
                return resp_len;

            case PS3::ReportID::GET_PAIRING_INFO:
                resp_len = reqlen;
                std::memcpy(buffer, &bt_info_, resp_len);
                return resp_len;

            case PS3::ReportID::FEATURE_F5:
                resp_len = reqlen;
                std::memcpy(buffer, output_ps3_0xf5, resp_len);

                for (ctr = 0; ctr < 6; ctr++) 
                {
                    buffer[1 + ctr] = bt_info_.host_address[ctr];
                }
                return resp_len;

            case PS3::ReportID::FEATURE_F7:
                resp_len = reqlen;
                std::memcpy(buffer, output_ps3_0xf7, resp_len);
                return resp_len;

            case PS3::ReportID::FEATURE_F8:
                resp_len = reqlen;
                std::memcpy(buffer, output_ps3_0xf8, resp_len);
                buffer[6] = ef_byte_;
                return resp_len;
        }
    }
    return -1;
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
    else if (report_type == HID_REPORT_TYPE_OUTPUT ) 
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
                std::memcpy(&out_report_, buf, std::min(bufsize, static_cast<uint16_t>(sizeof(PS3::OutReport))));
                new_out_report_ = true;
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