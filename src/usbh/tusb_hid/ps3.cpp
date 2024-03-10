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

    uint16_t vid, pid;
    tuh_vid_pid_get(dualshock3.dev_addr, &vid, &pid);
    if (vid == 0x054C && pid == 0x0268) dualshock3.sixaxis = true;
        
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

void Dualshock3::update_gamepad_dinput(const DInputReport* dinput_report)
{
    gamepad.reset_state();

    switch (dinput_report->direction)
    {
        case DINPUT_HAT_UP:
            gamepad.state.up = true;
            break;
        case DINPUT_HAT_UPRIGHT:
            gamepad.state.up = true;
            gamepad.state.right = true;
            break;
        case DINPUT_HAT_RIGHT:
            gamepad.state.right = true;
            break;
        case DINPUT_HAT_DOWNRIGHT:
            gamepad.state.right = true;
            gamepad.state.down = true;
            break;
        case DINPUT_HAT_DOWN:
            gamepad.state.down = true;
            break;
        case DINPUT_HAT_DOWNLEFT:
            gamepad.state.down = true;
            gamepad.state.left = true;
            break;
        case DINPUT_HAT_LEFT:
            gamepad.state.left = true;
            break;
        case DINPUT_HAT_UPLEFT:
            gamepad.state.up = true;
            gamepad.state.left = true;
            break;
    }

    if (dinput_report->square_btn)   gamepad.state.x = true;
    if (dinput_report->triangle_btn) gamepad.state.y = true;
    if (dinput_report->cross_btn)    gamepad.state.a = true;
    if (dinput_report->circle_btn)   gamepad.state.b = true;

    if (dinput_report->select_btn)   gamepad.state.back = true;
    if (dinput_report->start_btn)    gamepad.state.start = true;
    if (dinput_report->ps_btn)       gamepad.state.sys = true;

    if (dinput_report->l3_btn) gamepad.state.l3 = true;
    if (dinput_report->r3_btn) gamepad.state.r3 = true;

    if (dinput_report->l1_btn) gamepad.state.lb = true;
    if (dinput_report->r1_btn) gamepad.state.rb = true;

    if (dinput_report->l2_btn) gamepad.state.lt = 0xFF;
    if (dinput_report->r2_btn) gamepad.state.rt = 0xFF;

    gamepad.state.lx = scale_uint8_to_int16(dinput_report->l_x_axis, false);
    gamepad.state.ly = scale_uint8_to_int16(dinput_report->l_y_axis, true);
    gamepad.state.rx = scale_uint8_to_int16(dinput_report->r_x_axis, false);
    gamepad.state.ry = scale_uint8_to_int16(dinput_report->r_y_axis, true);
}

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

    gamepad.state.lx = scale_uint8_to_int16(ds3_data->left_x, false);
    gamepad.state.ly = scale_uint8_to_int16(ds3_data->left_y, true);
    gamepad.state.rx = scale_uint8_to_int16(ds3_data->right_x, false);
    gamepad.state.ry = scale_uint8_to_int16(ds3_data->right_y, true);
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

    if (!dualshock3.sixaxis)
    {
        static DInputReport prev_report = {0};
        DInputReport dinput_report;
        memcpy(&dinput_report, report, sizeof(dinput_report));
        update_gamepad_dinput(&dinput_report);
        prev_report = dinput_report;
    }
    else
    {
        static DualShock3Report prev_report = { 0 };
        DualShock3Report ds3_report;
        memcpy(&ds3_report, report, sizeof(ds3_report));
        update_gamepad(&ds3_report);
        prev_report = ds3_report;
    }
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

    struct sixaxis_output_report output_report;

    memcpy(&output_report, default_report, sizeof(output_report));

    // default_output_report.leds_bitmap |= 0x1 << (dualshock3.instance+1);
    output_report.leds_bitmap = 0x02;
    // output_report.led->time_enabled = 0xFF;
    // output_report.led->duty_on = 0xFF;

    output_report.rumble.right_duration = UINT8_MAX / 2;
    if (gamepadOut.out_state.rrumble > 0) output_report.rumble.right_motor_on = 1;

    output_report.rumble.left_duration = UINT8_MAX / 2;
    output_report.rumble.left_motor_force = gamepadOut.out_state.lrumble;

    bool rumble_sent = tuh_hid_send_report(dualshock3.dev_addr, dualshock3.instance, 1, &output_report, sizeof(output_report));

    return rumble_sent;
    // return true;
}