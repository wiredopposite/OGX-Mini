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
    for (auto& in_report : in_reports_)
    {
        std::memset(reinterpret_cast<void*>(&in_report), 0, sizeof(DInput::InReport));
        in_report.dpad = DInput::DPad::CENTER;
        in_report.joystick_lx = DInput::AXIS_MID;
        in_report.joystick_ly = DInput::AXIS_MID;
        in_report.joystick_rx = DInput::AXIS_MID;
        in_report.joystick_ry = DInput::AXIS_MID;
    }

	class_driver_ = {
		.name = TUD_DRV_NAME("HID"),
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = DInputDevice::control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void DInputDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    DInput::InReport& in_report = in_reports_[idx];

    if (gamepad.new_pad_in())
    {
        Gamepad::PadIn gp_in = gamepad.get_pad_in();

        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                in_report.dpad = DInput::DPad::UP;
                break;
            case Gamepad::DPAD_DOWN:
                in_report.dpad = DInput::DPad::DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                in_report.dpad = DInput::DPad::LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                in_report.dpad = DInput::DPad::RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                in_report.dpad = DInput::DPad::UP_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                in_report.dpad = DInput::DPad::UP_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                in_report.dpad = DInput::DPad::DOWN_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                in_report.dpad = DInput::DPad::DOWN_RIGHT;
                break;
            default:
                in_report.dpad = DInput::DPad::CENTER;
                break;
        }

        std::memset(in_report.buttons, 0, sizeof(in_report.buttons));

        if (gp_in.buttons & Gamepad::BUTTON_A)   in_report.buttons[0] |= DInput::Buttons0::CROSS;
        if (gp_in.buttons & Gamepad::BUTTON_B)   in_report.buttons[0] |= DInput::Buttons0::CIRCLE;
        if (gp_in.buttons & Gamepad::BUTTON_X)   in_report.buttons[0] |= DInput::Buttons0::SQUARE;
        if (gp_in.buttons & Gamepad::BUTTON_Y)   in_report.buttons[0] |= DInput::Buttons0::TRIANGLE;
        if (gp_in.buttons & Gamepad::BUTTON_LB)  in_report.buttons[0] |= DInput::Buttons0::L1;
        if (gp_in.buttons & Gamepad::BUTTON_RB)  in_report.buttons[0] |= DInput::Buttons0::R1;

        if (gp_in.buttons & Gamepad::BUTTON_L3)    in_report.buttons[1] |= DInput::Buttons1::L3;
        if (gp_in.buttons & Gamepad::BUTTON_R3)    in_report.buttons[1] |= DInput::Buttons1::R3;
        if (gp_in.buttons & Gamepad::BUTTON_BACK)  in_report.buttons[1] |= DInput::Buttons1::SELECT;
        if (gp_in.buttons & Gamepad::BUTTON_START) in_report.buttons[1] |= DInput::Buttons1::START;
        if (gp_in.buttons & Gamepad::BUTTON_SYS)   in_report.buttons[1] |= DInput::Buttons1::SYS;
        if (gp_in.buttons & Gamepad::BUTTON_MISC)  in_report.buttons[1] |= DInput::Buttons1::TP;

        if (gamepad.analog_enabled())
        {
            in_report.up_axis    = gp_in.analog[Gamepad::ANALOG_OFF_UP];
            in_report.down_axis  = gp_in.analog[Gamepad::ANALOG_OFF_DOWN];
            in_report.right_axis = gp_in.analog[Gamepad::ANALOG_OFF_RIGHT];
            in_report.left_axis  = gp_in.analog[Gamepad::ANALOG_OFF_LEFT];

            in_report.triangle_axis = gp_in.analog[Gamepad::ANALOG_OFF_Y];
            in_report.circle_axis   = gp_in.analog[Gamepad::ANALOG_OFF_X];
            in_report.cross_axis    = gp_in.analog[Gamepad::ANALOG_OFF_B];
            in_report.square_axis   = gp_in.analog[Gamepad::ANALOG_OFF_A];

            in_report.r1_axis = gp_in.analog[Gamepad::ANALOG_OFF_RB];
            in_report.l1_axis = gp_in.analog[Gamepad::ANALOG_OFF_LB];
        }

        in_report.joystick_lx = Scale::int16_to_uint8(gp_in.joystick_lx);
        in_report.joystick_ly = Scale::int16_to_uint8(gp_in.joystick_ly);
        in_report.joystick_rx = Scale::int16_to_uint8(gp_in.joystick_rx);
        in_report.joystick_ry = Scale::int16_to_uint8(gp_in.joystick_ry);

        in_report.l2_axis = gp_in.trigger_l;
        in_report.r2_axis = gp_in.trigger_r;
    }

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }

    if (tud_hid_n_ready(idx))
    {
        tud_hid_n_report(idx, 0, reinterpret_cast<void*>(&in_report), sizeof(DInput::InReport));
    }
}

uint16_t DInputDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    if (report_type != HID_REPORT_TYPE_INPUT || itf >= MAX_GAMEPADS)
    {
        return 0;
    }
    std::memcpy(buffer, &in_reports_[itf], sizeof(DInput::InReport));
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