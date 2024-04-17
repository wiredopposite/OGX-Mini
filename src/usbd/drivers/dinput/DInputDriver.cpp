#include "usbd/drivers/dinput/DInputDriver.h"
#include "usbd/descriptors/DInputDescriptors.h"

#include "usbd/drivers/shared/driverhelper.h"
#include "usbd/drivers/shared/scaling.h"

// Magic byte sequence to enable PS button on PS3
static const uint8_t ps3_magic_init_bytes[8] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};

static bool hid_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
	if ( request->bmRequestType == 0xA1 &&
		request->bRequest == HID_REQ_CONTROL_GET_REPORT &&
		request->wValue == 0x0300 ) 
    {
		return tud_control_xfer(rhport, request, (void *) ps3_magic_init_bytes, sizeof(ps3_magic_init_bytes));
	} 
    else 
    {
		return hidd_control_xfer_cb(rhport, stage, request);
	}
}

void DInputDriver::initialize() 
{
	dinput_report = {
        // .report_id = 0,
		.square_btn = 0, .cross_btn = 0, .circle_btn = 0, .triangle_btn = 0,
		.l1_btn = 0, .r1_btn = 0, .l2_btn = 0, .r2_btn = 0,
		.select_btn = 0, .start_btn = 0, .l3_btn = 0, .r3_btn = 0, .ps_btn = 0, .tp_btn = 0,
		.direction = 0x08,
		.l_x_axis = DINPUT_JOYSTICK_MID,
		.l_y_axis = DINPUT_JOYSTICK_MID,
		.r_x_axis = DINPUT_JOYSTICK_MID,
		.r_y_axis = DINPUT_JOYSTICK_MID,
		.right_axis = 0x00, .left_axis = 0x00, .up_axis = 0x00, .down_axis = 0x00,
		.triangle_axis = 0x00, .circle_axis = 0x00, .cross_axis = 0x00, .square_axis = 0x00,
		.l1_axis = 0x00, .r1_axis = 0x00, .l2_axis = 0x00, .r2_axis = 0x00
	};

	class_driver = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "HID",
	#endif
		.init = hidd_init,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hid_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

// Generate HID report from gamepad and send to TUSB Device
void DInputDriver::process(int idx, Gamepad * gamepad, uint8_t * outBuffer) 
{
    if (gamepad->buttons.up) {
        if (gamepad->buttons.right) {
            dinput_report.direction = DINPUT_HAT_UPRIGHT;
        } else if (gamepad->buttons.left) {
            dinput_report.direction = DINPUT_HAT_UPLEFT;
        } else {
            dinput_report.direction = DINPUT_HAT_UP;
        }
    } else if (gamepad->buttons.down) {
        if (gamepad->buttons.right) {
            dinput_report.direction = DINPUT_HAT_DOWNRIGHT;
        } else if (gamepad->buttons.left) {
            dinput_report.direction = DINPUT_HAT_DOWNLEFT;
        } else {
            dinput_report.direction = DINPUT_HAT_DOWN;
        }
    } else if (gamepad->buttons.left) {
        dinput_report.direction = DINPUT_HAT_LEFT;
    } else if (gamepad->buttons.right) {
        dinput_report.direction = DINPUT_HAT_RIGHT;
    } else {
        dinput_report.direction = DINPUT_HAT_NOTHING;
    }

    dinput_report.cross_btn     = gamepad->buttons.a      ? 1 : 0;
    dinput_report.circle_btn    = gamepad->buttons.b      ? 1 : 0;
    dinput_report.square_btn    = gamepad->buttons.x      ? 1 : 0;
    dinput_report.triangle_btn  = gamepad->buttons.y      ? 1 : 0;
    dinput_report.l1_btn        = gamepad->buttons.lb     ? 1 : 0;
    dinput_report.r1_btn        = gamepad->buttons.rb     ? 1 : 0;
    dinput_report.l2_btn        = gamepad->triggers.l > 0 ? 1 : 0;
    dinput_report.r2_btn        = gamepad->triggers.r > 0 ? 1 : 0;
    dinput_report.select_btn    = gamepad->buttons.back   ? 1 : 0;
    dinput_report.start_btn     = gamepad->buttons.start  ? 1 : 0;
    dinput_report.l3_btn        = gamepad->buttons.l3     ? 1 : 0;
    dinput_report.r3_btn        = gamepad->buttons.r3     ? 1 : 0;
    dinput_report.ps_btn        = gamepad->buttons.sys    ? 1 : 0;
    dinput_report.tp_btn        = gamepad->buttons.misc   ? 1 : 0;

    dinput_report.cross_axis    = gamepad->buttons.a  ? 0xFF : 0x00;
    dinput_report.circle_axis   = gamepad->buttons.b  ? 0xFF : 0x00;
    dinput_report.square_axis   = gamepad->buttons.x  ? 0xFF : 0x00;
    dinput_report.triangle_axis = gamepad->buttons.y  ? 0xFF : 0x00;
    dinput_report.l1_axis       = gamepad->buttons.lb ? 0xFF : 0x00;
    dinput_report.r1_axis       = gamepad->buttons.rb ? 0xFF : 0x00;

    dinput_report.l2_axis = gamepad->triggers.l;
    dinput_report.r2_axis = gamepad->triggers.r;

    dinput_report.l_x_axis = scale_int16_to_uint8(gamepad->joysticks.lx, false);
    dinput_report.l_y_axis = scale_int16_to_uint8(gamepad->joysticks.ly, true);
    dinput_report.r_x_axis = scale_int16_to_uint8(gamepad->joysticks.rx, false);
    dinput_report.r_y_axis = scale_int16_to_uint8(gamepad->joysticks.ry, true);

	// Wake up TinyUSB device
	if (tud_suspended())
		tud_remote_wakeup();

	void * report = &dinput_report;
	uint16_t report_size = sizeof(dinput_report);
	// if (memcmp(last_report, report, report_size) != 0)
	// {
		// HID ready + report sent, copy previous report
		// if (tud_hid_ready() && tud_hid_report(0, report, report_size) == true ) {
		// 	memcpy(last_report, report, report_size);
		if (tud_hid_n_ready((uint8_t)idx) && tud_hid_n_report((uint8_t)idx, 0, report, report_size) == true ) 
        {
			memcpy(last_report, report, report_size);
		}
    // }
}

// tud_hid_get_report_cb
uint16_t DInputDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    memcpy(buffer, &dinput_report, sizeof(DInputReport));
    return sizeof(DInputReport);
}

// Only PS4 does anything with set report
void DInputDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{
    // testing things, I'll have a usb sniffer soon so this will get figured out then

    // DInputOutReport dinput_out_report = {0};
    // memcpy(&dinput_out_report, buffer, sizeof(dinput_out_report));

    // // gamepadOut.out_state.rrumble = dinput_out_report.rumble.right_motor_on ? 0xFF : 0x00;
    // // gamepadOut.out_state.lrumble = gamepadOut.out_state.rrumble = dinput_out_report.rumble.left_motor_force;

    // // if (dinput_out_report.rumble.left_motor_force > 0)
    // if (dinput_out_report.led->time_enabled > 0)
    //     gamepadOut.out_state.lrumble = gamepadOut.out_state.rrumble = 0xFF;
    // if (dinput_out_report.led->duty_length > 0)
    //     gamepadOut.out_state.lrumble = gamepadOut.out_state.rrumble = 0xFF;
    // if (dinput_out_report.led->enabled > 0)
    //     gamepadOut.out_state.lrumble = gamepadOut.out_state.rrumble = 0xFF;
    // if (dinput_out_report.led->duty_off > 0)
    //     gamepadOut.out_state.lrumble = gamepadOut.out_state.rrumble = 0xFF;
    // if (dinput_out_report.led->duty_on > 0)
    //     gamepadOut.out_state.lrumble = gamepadOut.out_state.rrumble = 0xFF;
}

// Only XboxOG and Xbox One use vendor control xfer cb
bool DInputDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * DInputDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = (const char *)dinput_string_descriptors[index];
	return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * DInputDriver::get_descriptor_device_cb() 
{
    return dinput_device_descriptor;
}

const uint8_t * DInputDriver::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return dinput_report_descriptor;
}

const uint8_t * DInputDriver::get_descriptor_configuration_cb(uint8_t index) 
{
    return dinput_configuration_descriptor;
}

const uint8_t * DInputDriver::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}

void DInputDriver::update_rumble(int idx, Gamepad * gamepad)
{
    
}