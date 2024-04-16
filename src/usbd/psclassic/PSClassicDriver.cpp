#include "usbd/psclassic/PSClassicDriver.h"
#include "usbd/shared/driverhelper.h"

void PSClassicDriver::initialize() {
	psClassicReport = {
		.buttons = 0x0014
	};

	class_driver = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "PSCLASSIC",
	#endif
		.init = hidd_init,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void PSClassicDriver::process(uint8_t idx, Gamepad * gamepad, uint8_t * outBuffer) {
    psClassicReport.buttons = PSCLASSIC_MASK_CENTER;

    if (gamepad->buttons.up) {
        if (gamepad->buttons.right)
            psClassicReport.buttons = PSCLASSIC_MASK_UP_RIGHT;
        else if (gamepad->buttons.left)
            psClassicReport.buttons = PSCLASSIC_MASK_UP_LEFT;
        else
            psClassicReport.buttons = PSCLASSIC_MASK_UP;
    } else if (gamepad->buttons.down) {
        if (gamepad->buttons.right)
            psClassicReport.buttons = PSCLASSIC_MASK_DOWN_RIGHT;
        else if (gamepad->buttons.left)
            psClassicReport.buttons = PSCLASSIC_MASK_DOWN_LEFT;
        else
            psClassicReport.buttons = PSCLASSIC_MASK_DOWN;
    } else if (gamepad->buttons.left) {
        psClassicReport.buttons = PSCLASSIC_MASK_LEFT;
    } else if (gamepad->buttons.right) {
        psClassicReport.buttons = PSCLASSIC_MASK_RIGHT;
    }

    psClassicReport.buttons |=
        (gamepad->buttons.start ? PSCLASSIC_MASK_START    : 0) |
        (gamepad->buttons.back  ? PSCLASSIC_MASK_SELECT   : 0) |
        (gamepad->buttons.a     ? PSCLASSIC_MASK_CROSS    : 0) |
        (gamepad->buttons.b     ? PSCLASSIC_MASK_CIRCLE   : 0) |
        (gamepad->buttons.x     ? PSCLASSIC_MASK_SQUARE   : 0) |
        (gamepad->buttons.y     ? PSCLASSIC_MASK_TRIANGLE : 0) |
        (gamepad->buttons.lb    ? PSCLASSIC_MASK_L1       : 0) |
        (gamepad->buttons.rb    ? PSCLASSIC_MASK_R1       : 0) |
        (gamepad->triggers.l    ? PSCLASSIC_MASK_L2       : 0) |
        (gamepad->triggers.r    ? PSCLASSIC_MASK_R2       : 0);

	// Wake up TinyUSB device
	if (tud_suspended())
		tud_remote_wakeup();

	void * report = &psClassicReport;
	uint16_t report_size = sizeof(psClassicReport);
	if (memcmp(last_report, report, report_size) != 0) {
		// HID ready + report sent, copy previous report
		if (tud_hid_n_ready(idx) && tud_hid_n_report(idx, 0, report, report_size) == true ) 
        {
			memcpy(last_report, report, report_size);
		}
	}
}

// tud_hid_get_report_cb
uint16_t PSClassicDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    memcpy(buffer, &psClassicReport, sizeof(PSClassicReport));
	return sizeof(PSClassicReport);
}

// Only PS4 does anything with set report
void PSClassicDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

// Only XboxOG and Xbox One use vendor control xfer cb
bool PSClassicDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    return false;
}

const uint16_t * PSClassicDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
	const char *value = (const char *)psclassic_string_descriptors[index];
	return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * PSClassicDriver::get_descriptor_device_cb() {
    return psclassic_device_descriptor;
}

const uint8_t * PSClassicDriver::get_hid_descriptor_report_cb(uint8_t itf) {
    return psclassic_report_descriptor;
}

const uint8_t * PSClassicDriver::get_descriptor_configuration_cb(uint8_t index) {
    return psclassic_configuration_descriptor;
}

const uint8_t * PSClassicDriver::get_descriptor_device_qualifier_cb() {
	return nullptr;
}

uint16_t PSClassicDriver::GetJoystickMidValue() {
	return 0;
}

void PSClassicDriver::update_rumble(uint8_t idx, Gamepad * gamepad)
{
    
}