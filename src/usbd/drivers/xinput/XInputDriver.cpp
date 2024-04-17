/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "usbd/drivers/xinput/XInputDriver.h"
#include "usbd/drivers/shared/driverhelper.h"

// #include "Gamepad.h"

#define XINPUT_OUT_SIZE 32

uint8_t endpoint_in = 0;
uint8_t endpoint_out = 0;
uint8_t xinput_out_buffer[XINPUT_OUT_SIZE] = {};

struct XInputRumble
{
	uint8_t left_motor = {0};
	uint8_t right_motor = {0};
};

XInputRumble xinput_rumble;

void xinput_process_rumble_data(const uint8_t* outBuffer)
{
	XInputOutReport out_report;

	memcpy(&out_report, outBuffer, sizeof(XInputOutReport));

	if (out_report.report_type == XBOX_REPORT_TYPE_RUMBLE)
	{
		xinput_rumble.left_motor = out_report.lrumble;
		xinput_rumble.right_motor = out_report.rrumble;
	}
}

static void xinput_init(void)
{
}

static void xinput_reset(uint8_t rhport)
{
	(void)rhport;
}

static uint16_t xinput_open(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length)
{
	uint16_t driver_length = sizeof(tusb_desc_interface_t) + (itf_descriptor->bNumEndpoints * sizeof(tusb_desc_endpoint_t)) + 16;

	TU_VERIFY(max_length >= driver_length, 0);

	uint8_t const *current_descriptor = tu_desc_next(itf_descriptor);
	uint8_t found_endpoints = 0;
	while ((found_endpoints < itf_descriptor->bNumEndpoints) && (driver_length <= max_length))
	{
		tusb_desc_endpoint_t const *endpoint_descriptor = (tusb_desc_endpoint_t const *)current_descriptor;
		if (TUSB_DESC_ENDPOINT == tu_desc_type(endpoint_descriptor))
		{
			TU_ASSERT(usbd_edpt_open(rhport, endpoint_descriptor));

			if (tu_edpt_dir(endpoint_descriptor->bEndpointAddress) == TUSB_DIR_IN)
				endpoint_in = endpoint_descriptor->bEndpointAddress;
			else
				endpoint_out = endpoint_descriptor->bEndpointAddress;

			++found_endpoints;
		}

		current_descriptor = tu_desc_next(current_descriptor);
	}
	return driver_length;
}

static bool xinput_device_control_request(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
	(void)rhport;
	(void)stage;
	(void)request;

	return true;
}

static bool xinput_control_complete(uint8_t rhport, tusb_control_request_t const *request)
{
	(void)rhport;
	(void)request;

	return true;
}

static bool xinput_xfer_callback(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
	(void)rhport;
	(void)result;
	(void)xferred_bytes;

	if (ep_addr == endpoint_out) usbd_edpt_xfer(0, endpoint_out, xinput_out_buffer, XINPUT_OUT_SIZE);

	xinput_process_rumble_data(xinput_out_buffer);

	return true;
}

void XInputDriver::initialize() 
{
	xinputReport = {
		.report_id = 0,
		.report_size = XINPUT_ENDPOINT_SIZE,
		.buttons1 = 0,
		.buttons2 = 0,
		.lt = 0,
		.rt = 0,
		.lx = 0,
		.ly = 0,
		.rx = 0,
		.ry = 0,
		._reserved = { },
	};

	class_driver = {
	#if CFG_TUSB_DEBUG >= 2
		.name = "XINPUT",
	#endif
		.init = xinput_init,
		.reset = xinput_reset,
		.open = xinput_open,
		.control_xfer_cb = xinput_device_control_request,
		.xfer_cb = xinput_xfer_callback,
		.sof = NULL
	};
}

void XInputDriver::process(int idx, Gamepad * gamepad, uint8_t * outBuffer) 
{
	xinputReport.buttons1 = 0
		| (gamepad->buttons.up    ? XBOX_MASK_UP    : 0)
		| (gamepad->buttons.down  ? XBOX_MASK_DOWN  : 0)
		| (gamepad->buttons.left  ? XBOX_MASK_LEFT  : 0)
		| (gamepad->buttons.right ? XBOX_MASK_RIGHT : 0)
		| (gamepad->buttons.start ? XBOX_MASK_START : 0)
		| (gamepad->buttons.back  ? XBOX_MASK_BACK  : 0)
		| (gamepad->buttons.l3    ? XBOX_MASK_LS    : 0)
		| (gamepad->buttons.r3    ? XBOX_MASK_RS    : 0)
	;

	xinputReport.buttons2 = 0
		| (gamepad->buttons.rb	? XBOX_MASK_RB   : 0)
		| (gamepad->buttons.lb	? XBOX_MASK_LB   : 0)
		| (gamepad->buttons.sys ? XBOX_MASK_HOME : 0)
		| (gamepad->buttons.a 	? XBOX_MASK_A    : 0)
		| (gamepad->buttons.b 	? XBOX_MASK_B    : 0)
		| (gamepad->buttons.x 	? XBOX_MASK_X    : 0)
		| (gamepad->buttons.y 	? XBOX_MASK_Y    : 0)
	;

	xinputReport.lt = gamepad->triggers.l;
	xinputReport.rt = gamepad->triggers.r;

	xinputReport.lx = gamepad->joysticks.lx;
	xinputReport.ly = gamepad->joysticks.ly;
	xinputReport.rx = gamepad->joysticks.rx;
	xinputReport.ry = gamepad->joysticks.ry;
	
	// printf("processing rumble\n");

	// compare against previous report and send new
	if ( memcmp(last_report, &xinputReport, sizeof(XInputReport)) != 0) 
	{
		if ( tud_ready() &&											// Is the device ready?
			(endpoint_in != 0) && (!usbd_edpt_busy(0, endpoint_in)) ) // Is the IN endpoint available?
		{
			usbd_edpt_claim(0, endpoint_in);								// Take control of IN endpoint
			usbd_edpt_xfer(0, endpoint_in, (uint8_t *)&xinputReport, sizeof(XInputReport)); // Send report buffer
			usbd_edpt_release(0, endpoint_in);								// Release control of IN endpoint
			memcpy(last_report, &xinputReport, sizeof(XInputReport)); // save if we sent it
		}
	}

	// check for player LEDs
	if (tud_ready() &&
		(endpoint_out != 0) && (!usbd_edpt_busy(0, endpoint_out)))
	{
		usbd_edpt_claim(0, endpoint_out);									 // Take control of OUT endpoint
		usbd_edpt_xfer(0, endpoint_out, outBuffer, XINPUT_OUT_SIZE); 		 // Retrieve report buffer
		usbd_edpt_release(0, endpoint_out);									 // Release control of OUT endpoint
	}
}

// tud_hid_get_report_cb
uint16_t XInputDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
	memcpy(buffer, &xinputReport, sizeof(XInputReport));
	return sizeof(XInputReport);
}

// Only PS4 does anything with set report
void XInputDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

// Only XboxOG and Xbox One use vendor control xfer cb
bool XInputDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * XInputDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = (const char *)xinput_string_descriptors[index];
	return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * XInputDriver::get_descriptor_device_cb() 
{
    return xinput_device_descriptor;
}

const uint8_t * XInputDriver::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * XInputDriver::get_descriptor_configuration_cb(uint8_t index) 
{
    return xinput_configuration_descriptor;
}

const uint8_t * XInputDriver::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}

void XInputDriver::update_rumble(int idx, Gamepad * gamepad)
{
	gamepad->rumble.l = xinput_rumble.left_motor;
	gamepad->rumble.r = xinput_rumble.right_motor; 
}