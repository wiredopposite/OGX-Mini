#include "usbd/drivers/xboxog/XboxOriginalDriver.h"
#include "usbd/drivers/xboxog/xid/xid.h"
#include "usbd/drivers/shared/driverhelper.h"

// #include "Gamepad.h"

struct XIDRumble
{
	uint16_t left_motor = {0};
	uint16_t right_motor = {0};
};

XIDRumble xid_rumble;

void XboxOriginalDriver::initialize() {
    xboxOriginalReport = {
        .dButtons = 0,
        .A = 0,
        .B = 0,
        .X = 0,
        .Y = 0,
        .BLACK = 0,
        .WHITE = 0,
        .L = 0,
        .R = 0,
        .leftStickX = 0,
        .leftStickY = 0,
        .rightStickX = 0,
        .rightStickY = 0,
    };

    // Copy XID driver to local class driver
    memcpy(&class_driver, xid_get_driver(), sizeof(usbd_class_driver_t));
}

void XboxOriginalDriver::process(int idx, Gamepad * gamepad, uint8_t * outBuffer) {
	// digital buttons
	xboxOriginalReport.dButtons = 0
		| (gamepad->buttons.up    ? XID_DUP    : 0)
		| (gamepad->buttons.down  ? XID_DDOWN  : 0)
		| (gamepad->buttons.left  ? XID_DLEFT  : 0)
		| (gamepad->buttons.right ? XID_DRIGHT : 0)
		| (gamepad->buttons.start ? XID_START  : 0)
		| (gamepad->buttons.back  ? XID_BACK   : 0)
		| (gamepad->buttons.l3    ? XID_LS     : 0)
		| (gamepad->buttons.r3    ? XID_RS     : 0)
	;

    // analog buttons - convert to digital
    xboxOriginalReport.A     = (gamepad->buttons.a  ? 0xFF : 0);
    xboxOriginalReport.B     = (gamepad->buttons.b  ? 0xFF : 0);
    xboxOriginalReport.X     = (gamepad->buttons.x  ? 0xFF : 0);
    xboxOriginalReport.Y     = (gamepad->buttons.y  ? 0xFF : 0);
    xboxOriginalReport.BLACK = (gamepad->buttons.rb ? 0xFF : 0);
    xboxOriginalReport.WHITE = (gamepad->buttons.lb ? 0xFF : 0);

    // analog triggers
    xboxOriginalReport.L = gamepad->triggers.l;
    xboxOriginalReport.R = gamepad->triggers.r;

    // analog sticks
	xboxOriginalReport.leftStickX = gamepad->joysticks.lx;
	xboxOriginalReport.leftStickY = gamepad->joysticks.ly;
	xboxOriginalReport.rightStickX = gamepad->joysticks.rx;
	xboxOriginalReport.rightStickY = gamepad->joysticks.ry;

	if (tud_suspended())
		tud_remote_wakeup();

    uint8_t xIndex = xid_get_index_by_type(0, XID_TYPE_GAMECONTROLLER);
	if (memcmp(last_report, &xboxOriginalReport, sizeof(XboxOriginalReport)) != 0) {
        if ( xid_send_report(xIndex, &xboxOriginalReport, sizeof(XboxOriginalReport)) == true ) {
            memcpy(last_report, &xboxOriginalReport, sizeof(XboxOriginalReport));
        }
    }

    USB_XboxGamepad_OutReport_t xpad_rumble_data;

    if (xid_get_report(xIndex, &xpad_rumble_data, sizeof(xpad_rumble_data)))
    {
        xid_rumble.left_motor = xpad_rumble_data.lValue;
        xid_rumble.right_motor = xpad_rumble_data.rValue;
    }
}

// tud_hid_get_report_cb
uint16_t XboxOriginalDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    memcpy(buffer, &xboxOriginalReport, sizeof(XboxOriginalReport));
	return sizeof(XboxOriginalReport);
}

// Only PS4 does anything with set report
void XboxOriginalDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

// Only XboxOG and Xbox One use vendor control xfer cb
bool XboxOriginalDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    return class_driver.control_xfer_cb(rhport, stage, request);
}

const uint16_t * XboxOriginalDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
	const char *value = (const char *)xboxoriginal_string_descriptors[index];
	return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * XboxOriginalDriver::get_descriptor_device_cb() {
    return xboxoriginal_device_descriptor;
}

const uint8_t * XboxOriginalDriver::get_hid_descriptor_report_cb(uint8_t itf) {
    return nullptr;
}

const uint8_t * XboxOriginalDriver::get_descriptor_configuration_cb(uint8_t index) {
    return xboxoriginal_configuration_descriptor;
}

const uint8_t * XboxOriginalDriver::get_descriptor_device_qualifier_cb() {
	return nullptr;
}

void XboxOriginalDriver::update_rumble(int idx, Gamepad * gamepad)
{
    gamepad->rumble.l = xid_rumble.left_motor >> 8;
    gamepad->rumble.r = xid_rumble.right_motor >> 8;
}