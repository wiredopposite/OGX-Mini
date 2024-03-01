#include <stdint.h>

#include "pico/stdlib.h"

#include "tusb.h"

#include "usbh/tusb_hid/shared.h"
#include "usbh/tusb_hid/ps3.h"

#include "Gamepad.h"

void Dualshock3::init(uint8_t dev_addr, uint8_t instance)
{
    reset_state();

    dualshock3.dev_addr = dev_addr;
    dualshock3.instance = instance;

    enable_report();
}

bool Dualshock3::enable_report()
{
    static uint8_t buffer[5] = {};
    buffer[0] = 0xF4;
    buffer[1] = 0x42;
    buffer[2] = 0x03;
    buffer[3] = 0x00;
    buffer[4] = 0x00; 

    if (tuh_hid_send_ready)
    {
        dualshock3.report_enabled = tuh_hid_set_report(dualshock3.dev_addr, dualshock3.instance, 0, HID_REPORT_TYPE_FEATURE, &buffer, sizeof(buffer));
    }

    return dualshock3.report_enabled;
}

void Dualshock3::reset_state()
{
    dualshock3.report_enabled = false;
    dualshock3.dev_addr = 0;
    dualshock3.instance = 0;
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

    gamepad.state.lx = scale_uint8_to_int16(ds3_data->leftX, false);
    gamepad.state.ly = scale_uint8_to_int16(ds3_data->leftY, true);
    gamepad.state.rx = scale_uint8_to_int16(ds3_data->rightX, false);
    gamepad.state.ry = scale_uint8_to_int16(ds3_data->rightY, true);
}

void Dualshock3::process_report(uint8_t const* report, uint16_t len)
{
    if (!dualshock3.report_enabled)
    {
        return;
    }

    static DualShock3Report prev_report = { 0 };

    DualShock3Report ds3_report;
    memcpy(&ds3_report, report, sizeof(ds3_report));

    if (memcmp(&ds3_report, &prev_report, sizeof(ds3_report)) != 0)
    {
        update_gamepad(&ds3_report);
        prev_report = ds3_report;
    }
}

bool Dualshock3::send_fb_data()
{
    // sony_ds4_output_report_t output_report = {0};
    // output_report.set_rumble = 1;
    // output_report.motor_left = gamepadOut.out_state.lrumble;
    // output_report.motor_right = gamepadOut.out_state.rrumble;
    
    // bool rumble_sent = tuh_hid_send_report(dev_addr, instance, 5, &output_report, sizeof(output_report));

    // if (rumble_sent)
    // {
    //     gamepadOut.rumble_hid_reset();
    // }

    // return rumble_sent;
    return true;
}