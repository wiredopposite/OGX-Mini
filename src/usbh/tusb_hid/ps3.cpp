#include <stdint.h>

#include "pico/stdlib.h"

#include "tusb.h"
#include "class/hid/hid_host.h"

#include "utilities/scaling.h"

#include "usbh/tusb_hid/ps3.h"

#include "Gamepad.h"

#include "utilities/log.h"

/* ---------------------------- */
/* this does not work currently */
/* ---------------------------- */

void Dualshock3::init(uint8_t dev_addr, uint8_t instance)
{
    dualshock3.dev_addr = dev_addr;
    dualshock3.instance = instance;
        
    // tuh_hid_set_protocol(dualshock3.dev_addr, dualshock3.instance, HID_PROTOCOL_REPORT);
    // sleep_ms(200);
    if (enable_reports())
    {
        log("reports enabled, addr: %02X inst: %02X", dev_addr, instance);
    }
    else
    {
        log("reports enable failed");
    }
}

bool Dualshock3::enable_reports()
{
    // uint8_t cmd_buf[4];
    // cmd_buf[0] = 0x42;
    // cmd_buf[1] = 0x0c;
    // cmd_buf[2] = 0x00;
    // cmd_buf[3] = 0x00;

    // // if (tuh_hid_send_ready)
    // // {
    //     dualshock3.report_enabled = tuh_hid_set_report(dualshock3.dev_addr, dualshock3.instance, 0xF4, HID_REPORT_TYPE_FEATURE, &cmd_buf, sizeof(cmd_buf));
    // // }

    static uint8_t buffer[5] = {};
    buffer[0] = 0xF4;
    buffer[1] = 0x42;
    buffer[2] = 0x03;
    buffer[3] = 0x00;
    buffer[4] = 0x00;

    dualshock3.report_enabled = tuh_hid_set_report(dualshock3.dev_addr, dualshock3.instance, 0, HID_REPORT_TYPE_FEATURE, buffer, sizeof(buffer));

    tuh_hid_send_report(dualshock3.dev_addr, dualshock3.instance, 0, buffer, sizeof(buffer));

    if (dualshock3.report_enabled)
    {
        tuh_hid_receive_report(dualshock3.dev_addr, dualshock3.instance);
    }

    return dualshock3.report_enabled;
}

// void Dualshock3::reset_state()
// {
//     dualshock3.report_enabled = false;
//     dualshock3.dev_addr = 0;
//     dualshock3.instance = 0;
// }

void Dualshock3::update_gamepad(const DualShock3Report* ds3_data)
{
    gamepad.reset_state();

    if (ds3_data->up)       gamepad.state.up =true;
    if (ds3_data->down)     gamepad.state.down =true;
    if (ds3_data->left)     gamepad.state.left =true;
    if (ds3_data->right)    gamepad.state.right =true;

    if (ds3_data->square)   gamepad.state.x = true;
    if (ds3_data->triangle) gamepad.state.y = true;
    if (ds3_data->cross)    gamepad.state.a = true;
    if (ds3_data->circle)   gamepad.state.b = true;

    if (ds3_data->select)   gamepad.state.back = true;
    if (ds3_data->start)    gamepad.state.start = true;
    if (ds3_data->ps)       gamepad.state.sys = true;

    if (ds3_data->l3) gamepad.state.l3 = true;
    if (ds3_data->r3) gamepad.state.r3 = true;

    if (ds3_data->l1) gamepad.state.lb = true;
    if (ds3_data->r1) gamepad.state.rb = true;

    if (ds3_data->l2) gamepad.state.lt = 0xFF;
    if (ds3_data->r2) gamepad.state.rt = 0xFF;

    gamepad.state.lx = scale_uint8_to_int16(ds3_data->leftX, false);
    gamepad.state.ly = scale_uint8_to_int16(ds3_data->leftY, true);
    gamepad.state.rx = scale_uint8_to_int16(ds3_data->rightX, false);
    gamepad.state.ry = scale_uint8_to_int16(ds3_data->rightY, true);
}

void Dualshock3::process_report(uint8_t const* report, uint16_t len)
{
    // if (report[0] != 0x01) return;

    // print hex values
    char hex_buffer[len * 3];
    for (int i = 0; i < len; i++) 
    {
        sprintf(hex_buffer + (i * 3), "%02X ", report[i]); // Convert byte to hexadecimal string
    }
    log(hex_buffer);

    static DualShock3Report prev_report = { 0 };

    DualShock3Report ds3_report;
    memcpy(&ds3_report, report, sizeof(ds3_report));

    // if (memcmp(&ds3_report, &prev_report, sizeof(ds3_report)) != 0)
    // {
        update_gamepad(&ds3_report);
        prev_report = ds3_report;
    // }
}

bool Dualshock3::send_fb_data()
{
    uint8_t default_report[] = {
  			0x01, 0xff, 0x00, 0xff, 0x00,
  			0x00, 0x00, 0x00, 0x00, 0x00,
  			0xff, 0x27, 0x10, 0x00, 0x32,
  			0xff, 0x27, 0x10, 0x00, 0x32,
  			0xff, 0x27, 0x10, 0x00, 0x32,
  			0xff, 0x27, 0x10, 0x00, 0x32,
  			0x00, 0x00, 0x00, 0x00, 0x00
    };

    struct sixaxis_output_report default_output_report;

    memcpy(&default_output_report, default_report, sizeof(default_output_report));

    default_output_report.leds_bitmap |= 0x1 << (dualshock3.instance+1);

    bool rumble_sent = tuh_hid_send_report(dualshock3.dev_addr, dualshock3.instance, 0x1, &default_output_report, sizeof(default_output_report));

    return rumble_sent;
    // return true;
}