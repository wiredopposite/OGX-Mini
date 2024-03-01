#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/tusb_hid/ps5.h"

#include "Gamepad.h"

void Dualsense::init(uint8_t dev_addr, uint8_t instance)
{
    dualsense.dev_addr = dev_addr;
    dualsense.instance = instance;
}

void Dualsense::update_gamepad(const DualsenseReport* ds_data) 
{
    gamepad.reset_state();

    switch(ds_data->dpad) 
    {
        case PS5_MASK_DPAD_UP:
            gamepad.state.up = true;
            break;
        case PS5_MASK_DPAD_UP_RIGHT:
            gamepad.state.up = true;
            gamepad.state.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT:
            gamepad.state.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT_DOWN:
            gamepad.state.right = true;
            gamepad.state.down = true;
            break;
        case PS5_MASK_DPAD_DOWN:
            gamepad.state.down = true;
            break;
        case PS5_MASK_DPAD_DOWN_LEFT:
            gamepad.state.down = true;
            gamepad.state.left = true;
            break;
        case PS5_MASK_DPAD_LEFT:
            gamepad.state.left = true;
            break;
        case PS5_MASK_DPAD_LEFT_UP:
            gamepad.state.left = true;
            gamepad.state.up = true;
            break;
    }

    if (ds_data->square)    gamepad.state.x = true;
    if (ds_data->cross)     gamepad.state.a = true;
    if (ds_data->circle)    gamepad.state.b = true;
    if (ds_data->triangle)  gamepad.state.y = true;

    if (ds_data->buttons[0] & PS5_MASK_L1) gamepad.state.lb = true;
    if (ds_data->buttons[0] & PS5_MASK_R1) gamepad.state.rb = true;

    if (ds_data->buttons[0] & PS5_MASK_SHARE)    gamepad.state.back = true;
    if (ds_data->buttons[0] & PS5_MASK_OPTIONS)  gamepad.state.start = true;
    
    if (ds_data->buttons[0] & PS5_MASK_L3) gamepad.state.l3 = true;
    if (ds_data->buttons[0] & PS5_MASK_R3) gamepad.state.r3 = true;

    if (ds_data->buttons[1] & PS5_MASK_PS)   gamepad.state.sys = true;
    if (ds_data->buttons[1] & PS5_MASK_MIC)  gamepad.state.misc = true;

    gamepad.state.lt = ds_data->lt;
    gamepad.state.rt = ds_data->rt;

    gamepad.state.lx = scale_uint8_to_int16(ds_data->lx, false);
    gamepad.state.ly = scale_uint8_to_int16(ds_data->ly, true);
    gamepad.state.rx = scale_uint8_to_int16(ds_data->rx, false);
    gamepad.state.ry = scale_uint8_to_int16(ds_data->ry, true);
}

/* not much point in comparing changes unless we only look at buttons */
void Dualsense::process_report(uint8_t const* report, uint16_t len)
{
    // static DualsenseReport prev_report = { 0 };

    DualsenseReport ds_report;
    memcpy(&ds_report, report, sizeof(ds_report));

    // if (memcmp(&ds_report, &prev_report, sizeof(ds_report)) != 0)
    // {
        update_gamepad(&ds_report);

    //     prev_report = ds_report;
    // }
}

bool Dualsense::send_fb_data()
{
    // need to figure out if the flags are necessary and how the LEDs work
    DualsenseOutReport output_report = {0};
    output_report.valid_flag0 = 0x02; // idk what this means
    output_report.valid_flag1 = 0x02; // this one either
    output_report.valid_flag2 = 0x04; // uhhhhh
    output_report.motor_left = gamepadOut.out_state.lrumble;
    output_report.motor_right = gamepadOut.out_state.rrumble;

    return tuh_hid_send_report(dualsense.dev_addr, dualsense.instance, 5, &output_report, sizeof(output_report));
}