#include "usbd/drivers/switch/SwitchDriver.h"
#include "usbd/drivers/shared/driverhelper.h"

#include "usbd/drivers/shared/scaling.h"

void SwitchDriver::initialize() {
	switchReport = {
		.buttons = 0,
		.hat = SWITCH_HAT_NOTHING,
		.lx = SWITCH_JOYSTICK_MID,
		.ly = SWITCH_JOYSTICK_MID,
		.rx = SWITCH_JOYSTICK_MID,
		.ry = SWITCH_JOYSTICK_MID,
		.vendor = 0,
	};

	class_driver = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "SWITCH",
	#endif
		.init = hidd_init,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void SwitchDriver::process(int idx, Gamepad * gamepad, uint8_t * outBuffer) 
{
	switchReport.hat = SWITCH_HAT_NOTHING;

    if (gamepad->buttons.up) 
	{
        if (gamepad->buttons.right) 
		{
            switchReport.hat = SWITCH_HAT_UPRIGHT;
        } 
		else if (gamepad->buttons.left) 
		{
            switchReport.hat = SWITCH_HAT_UPLEFT;
        } 
		else 
		{
            switchReport.hat = SWITCH_HAT_UP;
        }
    } 
	else if (gamepad->buttons.down) 
	{
        if (gamepad->buttons.right) 
		{
            switchReport.hat = SWITCH_HAT_DOWNRIGHT;
        } 
		else if (gamepad->buttons.left) 
		{
            switchReport.hat = SWITCH_HAT_DOWNLEFT;
        } 
		else 
		{
            switchReport.hat = SWITCH_HAT_DOWN;
        }
    } 
	else if (gamepad->buttons.left) 
	{
        switchReport.hat = SWITCH_HAT_LEFT;
    } 
	else if (gamepad->buttons.right) 
	{
        switchReport.hat = SWITCH_HAT_RIGHT;
    }

	switchReport.buttons = 0
		| (gamepad->buttons.a 	? SWITCH_MASK_B    		: 0)
		| (gamepad->buttons.b 	? SWITCH_MASK_A    		: 0)
		| (gamepad->buttons.x 	? SWITCH_MASK_Y    		: 0)
		| (gamepad->buttons.y 	? SWITCH_MASK_X    		: 0)
		| (gamepad->buttons.lb 	? SWITCH_MASK_L    		: 0)
		| (gamepad->buttons.rb 	? SWITCH_MASK_R    		: 0)
		| (gamepad->triggers.l 	? SWITCH_MASK_ZL   		: 0)
		| (gamepad->triggers.r 	? SWITCH_MASK_ZR   		: 0)
		| (gamepad->buttons.back 	? SWITCH_MASK_MINUS		: 0)
		| (gamepad->buttons.start ? SWITCH_MASK_PLUS 		: 0)
		| (gamepad->buttons.l3 	? SWITCH_MASK_L3   		: 0)
		| (gamepad->buttons.r3 	? SWITCH_MASK_R3   		: 0)
		| (gamepad->buttons.sys 	? SWITCH_MASK_HOME 		: 0)
		| (gamepad->buttons.misc  ? SWITCH_MASK_CAPTURE 	: 0)
	;

	switchReport.lx = scale_int16_to_uint8(gamepad->joysticks.lx, false);
	switchReport.ly = scale_int16_to_uint8(gamepad->joysticks.ly, true);
	switchReport.rx = scale_int16_to_uint8(gamepad->joysticks.rx, false);
	switchReport.ry = scale_int16_to_uint8(gamepad->joysticks.ry, true);

	// Wake up TinyUSB device
	if (tud_suspended())
		tud_remote_wakeup();

	void * report = &switchReport;
	uint16_t report_size = sizeof(switchReport);
	if (memcmp(last_report, report, report_size) != 0) 
	{
		// HID ready + report sent, copy previous report
		if (tud_hid_n_ready(idx) && tud_hid_n_report(idx, 0, report, report_size) == true ) 
		{
			memcpy(last_report, report, report_size);
		}
	}
}

// tud_hid_get_report_cb
uint16_t SwitchDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    memcpy(buffer, &switchReport, sizeof(SwitchReport));
	return sizeof(SwitchReport);
}

// Only PS4 does anything with set report
void SwitchDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

// Only XboxOG and Xbox One use vendor control xfer cb
bool SwitchDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    return false;
}

const uint16_t * SwitchDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
	const char *value = (const char *)switch_string_descriptors[index];
	return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * SwitchDriver::get_descriptor_device_cb() {
    return switch_device_descriptor;
}

const uint8_t * SwitchDriver::get_hid_descriptor_report_cb(uint8_t itf) {
    return switch_report_descriptor;
}

const uint8_t * SwitchDriver::get_descriptor_configuration_cb(uint8_t index) {
    return switch_configuration_descriptor;
}

const uint8_t * SwitchDriver::get_descriptor_device_qualifier_cb() {
	return nullptr;
}

void SwitchDriver::update_rumble(int idx, Gamepad * gamepad)
{
    
}