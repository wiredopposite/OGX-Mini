#include "utilities/scaling.h"
#include "usbd/ps3/PS3Driver.h"
#include "descriptors/PS3Descriptors.h"
#include "usbd/shared/driverhelper.h"

static const uint8_t ps3_ps_btn_en_bytes[8] = 
{
	0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00
};

static bool ps3_auth = false;
static bool ps3_host = true;
static uint8_t byte_6_ef;
static uint8_t master_b_address[6];

static bool ps3_hid_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
    // if (!ps3_host)
    // {
    //     return hidd_control_xfer_cb(rhport, stage, request);
    // }

    static bool reply_f5 = false;
    static bool reply_ef = false;

	if (request->bmRequestType == 0xA1 && request->bRequest == HID_REQ_CONTROL_GET_REPORT)
    {
        switch(request->wValue)
        {
            // case 0x0300:
            //     return tud_control_xfer(rhport, request, (void *) ps3_ps_btn_en_bytes, sizeof(ps3_ps_btn_en_bytes));
            case 0x0301:
                return tud_control_xfer(rhport, request, (void *) report_01, sizeof(report_01));
            case 0x03f2:
                return tud_control_xfer(rhport, request, (void *) report_f2, sizeof(report_f2));
            case 0x03f5:
                uint8_t buffer[PS3_ENDPOINT_SIZE];
                memcpy(buffer, report_f5, sizeof(report_f5));
                if (!reply_f5)
                {
                    reply_f5 = tud_control_xfer(rhport, request, (void *) buffer, sizeof(report_f5));
                    return reply_f5;
                }
                else
                {
                    memcpy(buffer + 2, master_b_address, sizeof(master_b_address));
                    return tud_control_xfer(rhport, request, (void *) buffer, sizeof(report_f5));
                }
            case 0x03ef:
                uint8_t buffer[PS3_ENDPOINT_SIZE];
                memcpy(buffer, report_ef, sizeof(report_ef));
                buffer[7] = byte_6_ef;
                return tud_control_xfer(rhport, request, (void *) buffer, sizeof(report_ef));
            case 0x03f7:
                return tud_control_xfer(rhport, request, (void *) report_f7, sizeof(report_f7));
            case 0x03f8:
                return tud_control_xfer(rhport, request, (void *) report_f8, sizeof(report_f8));
            default:
                // ps3_auth = true;
                return hidd_control_xfer_cb(rhport, stage, request);
        }
	} 
    else 
    {
        // ps3_auth = true;
		return hidd_control_xfer_cb(rhport, stage, request);
	}
}

void PS3Driver::initialize() 
{
    ds3_report = {0};
    ds3_report.report_id = 0x01;
    ds3_report.ps        = 1;
    ds3_report.left_x    = PS3_JOYSTICK_MID;
    ds3_report.left_y    = PS3_JOYSTICK_MID;
    ds3_report.right_x   = PS3_JOYSTICK_MID;
    ds3_report.right_y   = PS3_JOYSTICK_MID;
    ds3_report.battery   = 0x04;

	class_driver = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "HID",
	#endif
		.init = hidd_init,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = ps3_hid_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void PS3Driver::process(uint8_t idx, Gamepad * gamepad, uint8_t * outBuffer) 
{
    // if (!ps3_auth) return;

    ds3_report.up        = gamepad->buttons.up     ? 1 : 0;
    ds3_report.down      = gamepad->buttons.down   ? 1 : 0;
    ds3_report.left      = gamepad->buttons.left   ? 1 : 0;
    ds3_report.right     = gamepad->buttons.right  ? 1 : 0;

    ds3_report.cross     = gamepad->buttons.a      ? 1 : 0;
    ds3_report.circle    = gamepad->buttons.b      ? 1 : 0;
    ds3_report.square    = gamepad->buttons.x      ? 1 : 0;
    ds3_report.triangle  = gamepad->buttons.y      ? 1 : 0;

    ds3_report.l1        = gamepad->buttons.lb     ? 1 : 0;
    ds3_report.r1        = gamepad->buttons.rb     ? 1 : 0;
    ds3_report.l2        = gamepad->triggers.l > 0 ? 1 : 0;
    ds3_report.r2        = gamepad->triggers.r > 0 ? 1 : 0;
    ds3_report.select    = gamepad->buttons.back   ? 1 : 0;
    ds3_report.start     = gamepad->buttons.start  ? 1 : 0;
    ds3_report.l3        = gamepad->buttons.l3     ? 1 : 0;
    ds3_report.r3        = gamepad->buttons.r3     ? 1 : 0;
    ds3_report.ps        = gamepad->buttons.sys    ? 1 : 0;

    ds3_report.cross_axis    = gamepad->buttons.a      ? 0xFF : 0x00;
    ds3_report.circle_axis   = gamepad->buttons.b      ? 0xFF : 0x00;
    ds3_report.square_axis   = gamepad->buttons.x      ? 0xFF : 0x00;
    ds3_report.triangle_axis = gamepad->buttons.y      ? 0xFF : 0x00;
    ds3_report.l1_axis       = gamepad->buttons.lb     ? 0xFF : 0x00;
    ds3_report.r1_axis       = gamepad->buttons.rb     ? 0xFF : 0x00;
    ds3_report.up_axis       = gamepad->buttons.up     ? 0xFF : 0x00;
    ds3_report.down_axis     = gamepad->buttons.down   ? 0xFF : 0x00;
    ds3_report.left_axis     = gamepad->buttons.left   ? 0xFF : 0x00;
    ds3_report.right_axis    = gamepad->buttons.right  ? 0xFF : 0x00;

    ds3_report.l2_axis = gamepad->triggers.l;
    ds3_report.r2_axis = gamepad->triggers.r;

    ds3_report.left_x = scale_int16_to_uint8(gamepad->joysticks.lx, false);
    ds3_report.left_y = scale_int16_to_uint8(gamepad->joysticks.ly, true);
    ds3_report.right_x = scale_int16_to_uint8(gamepad->joysticks.rx, false);
    ds3_report.right_y = scale_int16_to_uint8(gamepad->joysticks.ry, true);

	if (tud_suspended())
    {  
        tud_remote_wakeup();
    }

	void * report = &ds3_report;
	uint16_t report_size = sizeof(ds3_report);

	if (memcmp(&last_report, report, report_size) != 0) 
	{
		if (tud_hid_ready() && tud_hid_report(0, report, report_size) == true ) 
		{
			memcpy(&last_report, report, report_size);
		}
	}
}

uint16_t PS3Driver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    memcpy(buffer, &ds3_report, sizeof(Dualshock3Report));
    return sizeof(Dualshock3Report);
}

void PS3Driver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{
    if (report_type == HID_REPORT_TYPE_FEATURE)
    {
        switch (report_id)
        {
            case 0xF5:
                memcpy(master_b_address, buffer + 2, 6);
                break;
            case 0xEF:
                byte_6_ef = buffer[6];
                break;
            default:
                break;
        }
    }
    else 
    // if (report_id == 0x01 && report_type == HID_REPORT_TYPE_OUTPUT)
    {
        memcpy(&ds3_out_report, buffer, sizeof(Dualshock3OutReport));
        // ps3_auth = true;
    }
}

bool PS3Driver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * PS3Driver::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = (const char *)ps3_string_descriptors[index];
	return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * PS3Driver::get_descriptor_device_cb() 
{
    return ps3_device_descriptor;
}

const uint8_t * PS3Driver::get_hid_descriptor_report_cb(uint8_t itf) 
{
    // ps3_auth = true;
    // ps3_host = false; // PS3 doesn't ask for an HID report descriptor
    return ps3_report_descriptor;
}

const uint8_t * PS3Driver::get_descriptor_configuration_cb(uint8_t index) 
{
    return ps3_configuration_descriptor;
}

const uint8_t * PS3Driver::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}

uint16_t PS3Driver::GetJoystickMidValue() 
{
	return PS3_JOYSTICK_MID;
}

void PS3Driver::update_rumble(uint8_t idx, Gamepad * gamepad)
{
    gamepad->rumble.l = ds3_out_report.rumble.left_motor_force;
    gamepad->rumble.r = (ds3_out_report.rumble.right_motor_on < 0) ? 0xFF: 0x00;
}