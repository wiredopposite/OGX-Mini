#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/ps5/Dualsense.h"

#include "utilities/scaling.h"

void Dualsense::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    dualsense.player_id = player_id;
    tuh_hid_receive_report(dev_addr, instance);
}

void Dualsense::update_gamepad(Gamepad& gp, const DualsenseReport* ds_data) 
{
    gp.reset_state();

    switch(ds_data->dpad) 
    {
        case PS5_MASK_DPAD_UP:
            gp.state.up = true;
            break;
        case PS5_MASK_DPAD_UP_RIGHT:
            gp.state.up = true;
            gp.state.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT:
            gp.state.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT_DOWN:
            gp.state.right = true;
            gp.state.down = true;
            break;
        case PS5_MASK_DPAD_DOWN:
            gp.state.down = true;
            break;
        case PS5_MASK_DPAD_DOWN_LEFT:
            gp.state.down = true;
            gp.state.left = true;
            break;
        case PS5_MASK_DPAD_LEFT:
            gp.state.left = true;
            break;
        case PS5_MASK_DPAD_LEFT_UP:
            gp.state.left = true;
            gp.state.up = true;
            break;
    }

    if (ds_data->square)    gp.state.x = true;
    if (ds_data->cross)     gp.state.a = true;
    if (ds_data->circle)    gp.state.b = true;
    if (ds_data->triangle)  gp.state.y = true;

    if (ds_data->buttons[0] & PS5_MASK_L1) gp.state.lb = true;
    if (ds_data->buttons[0] & PS5_MASK_R1) gp.state.rb = true;

    if (ds_data->buttons[0] & PS5_MASK_SHARE)    gp.state.back = true;
    if (ds_data->buttons[0] & PS5_MASK_OPTIONS)  gp.state.start = true;
    
    if (ds_data->buttons[0] & PS5_MASK_L3) gp.state.l3 = true;
    if (ds_data->buttons[0] & PS5_MASK_R3) gp.state.r3 = true;

    if (ds_data->buttons[1] & PS5_MASK_PS)   gp.state.sys = true;
    if (ds_data->buttons[1] & PS5_MASK_MIC)  gp.state.misc = true;

    gp.state.lt = ds_data->lt;
    gp.state.rt = ds_data->rt;

    gp.state.lx = scale_uint8_to_int16(ds_data->lx, false);
    gp.state.ly = scale_uint8_to_int16(ds_data->ly, true);
    gp.state.rx = scale_uint8_to_int16(ds_data->rx, false);
    gp.state.ry = scale_uint8_to_int16(ds_data->ry, true);
}

void Dualsense::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    DualsenseReport ds_report;
    memcpy(&ds_report, report, sizeof(ds_report));
    update_gamepad(gp, &ds_report);
    tuh_hid_receive_report(dev_addr, instance);
}

void Dualsense::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool Dualsense::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
{
    // need to figure out if the flags are necessary and how the LEDs work
    DualsenseOutReport out_report = {0};
    out_report.valid_flag0 = 0x02; // idk what this means
    out_report.valid_flag1 = 0x02; // this one either
    out_report.valid_flag2 = 0x04; // uhhhhh
    out_report.motor_left = gp_out.state.lrumble;
    out_report.motor_right = gp_out.state.rrumble;

    return tuh_hid_send_report(dev_addr, instance, 5, &out_report, sizeof(out_report));
}