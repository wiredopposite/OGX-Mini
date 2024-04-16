#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "class/hid/hid_host.h"

#include "usbh/ps3/Dualshock3.h"

#include "utilities/scaling.h"
#include "utilities/log.h"

#define PS3_REPORT_BUFFER_SIZE 48

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

void Dualshock3::update_gamepad_from_dinput(Gamepad& gamepad, const DInputReport* dinput_report)
{
    // gamepad.reset_pad();

    // switch (dinput_report->direction)
    // {
    //     case DINPUT_HAT_UP:
    //         gamepad.buttons.up = true;
    //         break;
    //     case DINPUT_HAT_UPRIGHT:
    //         gamepad.buttons.up = true;
    //         gamepad.buttons.right = true;
    //         break;
    //     case DINPUT_HAT_RIGHT:
    //         gamepad.buttons.right = true;
    //         break;
    //     case DINPUT_HAT_DOWNRIGHT:
    //         gamepad.buttons.right = true;
    //         gamepad.buttons.down = true;
    //         break;
    //     case DINPUT_HAT_DOWN:
    //         gamepad.buttons.down = true;
    //         break;
    //     case DINPUT_HAT_DOWNLEFT:
    //         gamepad.buttons.down = true;
    //         gamepad.buttons.left = true;
    //         break;
    //     case DINPUT_HAT_LEFT:
    //         gamepad.buttons.left = true;
    //         break;
    //     case DINPUT_HAT_UPLEFT:
    //         gamepad.buttons.up = true;
    //         gamepad.buttons.left = true;
    //         break;
    // }

    // if (dinput_report->square)   gamepad.buttons.x = true;
    // if (dinput_report->triangle) gamepad.buttons.y = true;
    // if (dinput_report->cross)    gamepad.buttons.a = true;
    // if (dinput_report->circle)   gamepad.buttons.b = true;

    // if (dinput_report->select)   gamepad.buttons.back = true;
    // if (dinput_report->start)    gamepad.buttons.start = true;
    // if (dinput_report->ps)       gamepad.buttons.sys = true;

    // if (dinput_report->l3)       gamepad.buttons.l3 = true;
    // if (dinput_report->r3)       gamepad.buttons.r3 = true;
         
    // if (dinput_report->l1)       gamepad.buttons.lb = true;
    // if (dinput_report->r1)       gamepad.buttons.rb = true;
         
    // if (dinput_report->l2)       gamepad.triggers.l = 0xFF;
    // if (dinput_report->r2)       gamepad.triggers.r = 0xFF;

    // gamepad.joysticks.lx = scale_uint8_to_int16(dinput_report->lx_axis, false);
    // gamepad.joysticks.ly = scale_uint8_to_int16(dinput_report->ly_axis, true);
    // gamepad.joysticks.rx = scale_uint8_to_int16(dinput_report->rx_axis, false);
    // gamepad.joysticks.ry = scale_uint8_to_int16(dinput_report->ry_axis, true);
}

void Dualshock3::update_gamepad_from_ds3(Gamepad& gamepad, const Dualshock3Report* ds3_data)
{
    gamepad.reset_pad();

    if (ds3_data->up)       gamepad.buttons.up =true;
    if (ds3_data->down)     gamepad.buttons.down =true;
    if (ds3_data->left)     gamepad.buttons.left =true;
    if (ds3_data->right)    gamepad.buttons.right =true;

    if (ds3_data->square)   gamepad.buttons.x = true;
    if (ds3_data->triangle) gamepad.buttons.y = true;
    if (ds3_data->cross)    gamepad.buttons.a = true;
    if (ds3_data->circle)   gamepad.buttons.b = true;

    if (ds3_data->select)   gamepad.buttons.back = true;
    if (ds3_data->start)    gamepad.buttons.start = true;
    if (ds3_data->ps)       gamepad.buttons.sys = true;

    if (ds3_data->l3)       gamepad.buttons.l3 = true;
    if (ds3_data->r3)       gamepad.buttons.r3 = true;

    if (ds3_data->l1)       gamepad.buttons.lb = true;
    if (ds3_data->r1)       gamepad.buttons.rb = true;

    if (ds3_data->l2)       gamepad.triggers.l = 0xFF;
    if (ds3_data->r2)       gamepad.triggers.r = 0xFF;

    gamepad.joysticks.lx = scale_uint8_to_int16(ds3_data->left_x, false);
    gamepad.joysticks.ly = scale_uint8_to_int16(ds3_data->left_y, true);
    gamepad.joysticks.rx = scale_uint8_to_int16(ds3_data->right_x, false);
    gamepad.joysticks.ry = scale_uint8_to_int16(ds3_data->right_y, true);
}

void Dualshock3::process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
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
        update_gamepad_from_ds3(gamepad, &ds3_report);
        prev_report = ds3_report;
    // }

    tuh_hid_receive_report(dev_addr, instance);
}

void Dualshock3::process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool Dualshock3::send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance)
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

    return true;

    // uint8_t default_report[] = 
    // {
    //     0x01, 0xff, 0x00, 0xff, 0x00,
    //     0x00, 0x00, 0x00, 0x00, 0x00,
    //     0xff, 0x27, 0x10, 0x00, 0x32,
    //     0xff, 0x27, 0x10, 0x00, 0x32,
    //     0xff, 0x27, 0x10, 0x00, 0x32,
    //     0xff, 0x27, 0x10, 0x00, 0x32,
    //     0x00, 0x00, 0x00, 0x00, 0x00
    // };

    // Dualshock3OutReport out_report = {};

    // // memcpy(&out_report, default_report, sizeof(out_report));

    // // out_report.leds_bitmap |= 0x1 << (instance+1);
    // // out_report.leds_bitmap = 0x02;
    // // out_report.led->time_enabled = 0xFF;
    // // out_report.led->duty_on = 0xFF;

    // // out_report.rumble.right_duration = UINT8_MAX / 2;
    // // if (gamepad.rumble.r > 0) out_report.rumble.right_motor_on = 1;

    // // out_report.rumble.left_duration = UINT8_MAX / 2;
    // // out_report.rumble.left_motor_force = gamepad.rumble.l;

    // return tuh_hid_send_report(dev_addr, instance, 0x1, &out_report, sizeof(Dualshock3OutReport));
}