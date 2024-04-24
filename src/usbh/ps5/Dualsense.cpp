#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/ps5/Dualsense.h"
#include "usbh/shared/scaling.h"

void Dualsense::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    dualsense.player_id = player_id;
    tuh_hid_receive_report(dev_addr, instance);
}

void Dualsense::update_gamepad(Gamepad* gamepad, const DualsenseReport* ds_report) 
{
    gamepad->reset_pad(gamepad);

    switch(ds_report->dpad) 
    {
        case PS5_MASK_DPAD_UP:
            gamepad->buttons.up = true;
            break;
        case PS5_MASK_DPAD_UP_RIGHT:
            gamepad->buttons.up = true;
            gamepad->buttons.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT:
            gamepad->buttons.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT_DOWN:
            gamepad->buttons.right = true;
            gamepad->buttons.down = true;
            break;
        case PS5_MASK_DPAD_DOWN:
            gamepad->buttons.down = true;
            break;
        case PS5_MASK_DPAD_DOWN_LEFT:
            gamepad->buttons.down = true;
            gamepad->buttons.left = true;
            break;
        case PS5_MASK_DPAD_LEFT:
            gamepad->buttons.left = true;
            break;
        case PS5_MASK_DPAD_LEFT_UP:
            gamepad->buttons.left = true;
            gamepad->buttons.up = true;
            break;
    }

    if (ds_report->square)    gamepad->buttons.x = true;
    if (ds_report->cross)     gamepad->buttons.a = true;
    if (ds_report->circle)    gamepad->buttons.b = true;
    if (ds_report->triangle)  gamepad->buttons.y = true;

    if (ds_report->buttons[0] & PS5_MASK_L1) gamepad->buttons.lb = true;
    if (ds_report->buttons[0] & PS5_MASK_R1) gamepad->buttons.rb = true;

    if (ds_report->buttons[0] & PS5_MASK_SHARE)    gamepad->buttons.back = true;
    if (ds_report->buttons[0] & PS5_MASK_OPTIONS)  gamepad->buttons.start = true;
    
    if (ds_report->buttons[0] & PS5_MASK_L3) gamepad->buttons.l3 = true;
    if (ds_report->buttons[0] & PS5_MASK_R3) gamepad->buttons.r3 = true;

    if (ds_report->buttons[1] & PS5_MASK_PS)   gamepad->buttons.sys = true;
    if (ds_report->buttons[1] & PS5_MASK_MIC)  gamepad->buttons.misc = true;

    gamepad->triggers.l = ds_report->lt;
    gamepad->triggers.r = ds_report->rt;

    gamepad->joysticks.lx = scale_uint8_to_int16(ds_report->lx, false);
    gamepad->joysticks.ly = scale_uint8_to_int16(ds_report->ly, true);
    gamepad->joysticks.rx = scale_uint8_to_int16(ds_report->rx, false);
    gamepad->joysticks.ry = scale_uint8_to_int16(ds_report->ry, true);
}

void Dualsense::process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void)len;

    DualsenseReport ds_report;
    memcpy(&ds_report, report, sizeof(ds_report));
    update_gamepad(gamepad, &ds_report);
    tuh_hid_receive_report(dev_addr, instance);
}

void Dualsense::process_xinput_report(Gamepad* gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) 
{
    (void)gp;
    (void)dev_addr;
    (void)instance;
    (void)report;
    (void)len;
}

void Dualsense::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) 
{
    (void)dev_addr;
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)len;
}

bool Dualsense::send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance)
{
    // need to figure out if the flags are necessary and how the LEDs work
    DualsenseOutReport out_report = {0};
    out_report.valid_flag0 = 0x02; // idk what this means
    out_report.valid_flag1 = 0x02; // this one either
    out_report.valid_flag2 = 0x04; // uhhhhh
    out_report.motor_left = gamepad->rumble.l;
    out_report.motor_right = gamepad->rumble.r;

    return tuh_hid_send_report(dev_addr, instance, 5, &out_report, sizeof(out_report));
}