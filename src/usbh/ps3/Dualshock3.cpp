#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "class/hid/hid_host.h"

#include "usbh/ps3/Dualshock3.h"

#include "utilities/scaling.h"
#include "utilities/log.h"

#define PS3_REPORT_BUFFER_SIZE 48

/* -------------------------------------------- */
/* this only works for DInput currently, no DS3 */
/* -------------------------------------------- */

void Dualshock3::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    dualshock3.player_id = player_id;
    
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    if (vid == 0x054C && pid == 0x0268) 
    {
        dualshock3.sixaxis = true;
    }

    enable_reports(dev_addr, instance);
    tuh_hid_receive_report(dev_addr, instance);
}

void Dualshock3::enable_reports(uint8_t dev_addr, uint8_t instance)
{
    uint8_t cmd_buf[4];
    cmd_buf[0] = 0x42;
    // cmd_buf[1] = 0x0c;
    cmd_buf[1] = 0x03;
    cmd_buf[2] = 0x00;
    cmd_buf[3] = 0x00;

    if (tuh_hid_send_ready)
    {
        dualshock3.reports_enabled = tuh_hid_set_report(dev_addr, instance, 0xF4, HID_REPORT_TYPE_FEATURE, &cmd_buf, sizeof(cmd_buf));
    }

    // static uint8_t buffer[5] = {};
    // buffer[0] = 0xF4;
    // buffer[1] = 0x42;
    // buffer[2] = 0x03;
    // buffer[3] = 0x00;
    // buffer[4] = 0x00;

    // dualshock3.reports_enabled = tuh_hid_set_report(dev_addr, instance, 0, HID_REPORT_TYPE_FEATURE, buffer, sizeof(buffer));

    // dualshock3.reports_enabled = tuh_hid_send_report(dev_addr, instance, 0, cmd_buf, sizeof(cmd_buf));

    // if (dualshock3.reports_enabled)
    // {
    //     tuh_hid_receive_report(dev_addr, instance);
    // }
}

void Dualshock3::update_gamepad_from_dinput(Gamepad& gp, const DInputReport* dinput_report)
{
    gp.reset_state();

    switch (dinput_report->direction)
    {
        case DINPUT_HAT_UP:
            gp.state.up = true;
            break;
        case DINPUT_HAT_UPRIGHT:
            gp.state.up = true;
            gp.state.right = true;
            break;
        case DINPUT_HAT_RIGHT:
            gp.state.right = true;
            break;
        case DINPUT_HAT_DOWNRIGHT:
            gp.state.right = true;
            gp.state.down = true;
            break;
        case DINPUT_HAT_DOWN:
            gp.state.down = true;
            break;
        case DINPUT_HAT_DOWNLEFT:
            gp.state.down = true;
            gp.state.left = true;
            break;
        case DINPUT_HAT_LEFT:
            gp.state.left = true;
            break;
        case DINPUT_HAT_UPLEFT:
            gp.state.up = true;
            gp.state.left = true;
            break;
    }

    if (dinput_report->square_btn)   gp.state.x = true;
    if (dinput_report->triangle_btn) gp.state.y = true;
    if (dinput_report->cross_btn)    gp.state.a = true;
    if (dinput_report->circle_btn)   gp.state.b = true;

    if (dinput_report->select_btn)   gp.state.back = true;
    if (dinput_report->start_btn)    gp.state.start = true;
    if (dinput_report->ps_btn)       gp.state.sys = true;

    if (dinput_report->l3_btn)       gp.state.l3 = true;
    if (dinput_report->r3_btn)       gp.state.r3 = true;
         
    if (dinput_report->l1_btn)       gp.state.lb = true;
    if (dinput_report->r1_btn)       gp.state.rb = true;
         
    if (dinput_report->l2_btn)       gp.state.lt = 0xFF;
    if (dinput_report->r2_btn)       gp.state.rt = 0xFF;

    gp.state.lx = scale_uint8_to_int16(dinput_report->l_x_axis, false);
    gp.state.ly = scale_uint8_to_int16(dinput_report->l_y_axis, true);
    gp.state.rx = scale_uint8_to_int16(dinput_report->r_x_axis, false);
    gp.state.ry = scale_uint8_to_int16(dinput_report->r_y_axis, true);
}

void Dualshock3::update_gamepad_from_ds3(Gamepad& gp, const Dualshock3Report* ds3_data)
{
    gp.reset_state();

    if (ds3_data->up)       gp.state.up =true;
    if (ds3_data->down)     gp.state.down =true;
    if (ds3_data->left)     gp.state.left =true;
    if (ds3_data->right)    gp.state.right =true;

    if (ds3_data->square)   gp.state.x = true;
    if (ds3_data->triangle) gp.state.y = true;
    if (ds3_data->cross)    gp.state.a = true;
    if (ds3_data->circle)   gp.state.b = true;

    if (ds3_data->select)   gp.state.back = true;
    if (ds3_data->start)    gp.state.start = true;
    if (ds3_data->ps)       gp.state.sys = true;

    if (ds3_data->l3)       gp.state.l3 = true;
    if (ds3_data->r3)       gp.state.r3 = true;

    if (ds3_data->l1)       gp.state.lb = true;
    if (ds3_data->r1)       gp.state.rb = true;

    if (ds3_data->l2)       gp.state.lt = 0xFF;
    if (ds3_data->r2)       gp.state.rt = 0xFF;

    gp.state.lx = scale_uint8_to_int16(ds3_data->left_x, false);
    gp.state.ly = scale_uint8_to_int16(ds3_data->left_y, true);
    gp.state.rx = scale_uint8_to_int16(ds3_data->right_x, false);
    gp.state.ry = scale_uint8_to_int16(ds3_data->right_y, true);
}

void Dualshock3::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    if (dualshock3.sixaxis && !dualshock3.reports_enabled)
    {
        enable_reports(dev_addr, instance);
        return;
    }

    // if (!dualshock3.sixaxis)
    // {
    //     static DInputReport prev_report = {0};
    //     DInputReport dinput_report;
    //     memcpy(&dinput_report, report, sizeof(dinput_report));
    //     update_gamepad_from_dinput(gp, &dinput_report);
    //     prev_report = dinput_report;
    // }
    // else
    // {
        static Dualshock3Report prev_report = { 0 };
        Dualshock3Report ds3_report;
        memcpy(&ds3_report, report, sizeof(ds3_report));
        update_gamepad_from_ds3(gp, &ds3_report);
        prev_report = ds3_report;
    // }

    tuh_hid_receive_report(dev_addr, instance);
}

void Dualshock3::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool Dualshock3::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
{
    if (dualshock3.sixaxis && !dualshock3.reports_enabled)
    {
        enable_reports(dev_addr, instance);
        return false;
    }
    else if (!dualshock3.sixaxis)
    {
        return true;
    }

    uint8_t default_report[] = 
    {
        0x01, 0xff, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0xff, 0x27, 0x10, 0x00, 0x32,
        0x00, 0x00, 0x00, 0x00, 0x00
    };

    struct sixaxis_output_report out_report;

    memcpy(&out_report, default_report, sizeof(out_report));

    out_report.leds_bitmap |= 0x1 << (instance+1);
    out_report.leds_bitmap = 0x02;
    out_report.led->time_enabled = 0xFF;
    out_report.led->duty_on = 0xFF;

    out_report.rumble.right_duration = UINT8_MAX / 2;
    if (gp_out.state.rrumble > 0) out_report.rumble.right_motor_on = 1;

    out_report.rumble.left_duration = UINT8_MAX / 2;
    out_report.rumble.left_motor_force = gp_out.state.lrumble;

    return tuh_hid_send_report(dev_addr, instance, 0x1, &out_report, sizeof(out_report));
}