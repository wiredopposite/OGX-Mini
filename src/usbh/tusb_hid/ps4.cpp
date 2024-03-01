#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "CRC32.h"

#include "usbh/tusb_hid/ps4.h"

#include "Gamepad.h"

void Dualshock4::init(uint8_t dev_addr, uint8_t instance)
{
    dualshock4.dev_addr = dev_addr;
    dualshock4.instance = instance;
    // set_leds();
}

/* this DCs the controller, come back to it */
bool Dualshock4::set_leds()
{
    // see: https://github.com/Hydr8gon/VitaControl

    static uint8_t buffer[79] = {};
    buffer[0]  = 0xA2;
    buffer[1]  = 0x11;
    buffer[2]  = 0xC0;
    buffer[3]  = 0x20;
    buffer[4]  = 0xF3;
    buffer[5]  = 0x04;
    buffer[9]  = led_colors[dualshock4.instance][0];
    buffer[10] = led_colors[dualshock4.instance][1];
    buffer[11] = led_colors[dualshock4.instance][2];

    // Calculate the CRC of the data (including the 0xA2 byte) and append it to the end
    uint32_t crc = CRC32::calculate(buffer, 75);
    buffer[75] = crc >>  0;
    buffer[76] = crc >>  8;
    buffer[77] = crc >> 16;
    buffer[78] = crc >> 24;

    // Send the write request, omitting the 0xA2 byte
    dualshock4.leds_set = tuh_hid_send_report(dualshock4.dev_addr, dualshock4.instance, 0, &buffer + 1, sizeof(buffer) - 1);

    return dualshock4.leds_set;
}

void Dualshock4::update_gamepad(const Dualshock4Report* ds4_data)
{
    gamepad.reset_state();

    switch(ds4_data->dpad)
    {
        case PS4_DPAD_MASK_UP:
            gamepad.state.up = true;
            break;
        case PS4_DPAD_MASK_UP_RIGHT:
            gamepad.state.up = true;
            gamepad.state.right = true;
            break;
        case PS4_DPAD_MASK_RIGHT:
            gamepad.state.right = true;
            break;
        case PS4_DPAD_MASK_RIGHT_DOWN:
            gamepad.state.right = true;
            gamepad.state.down = true;
            break;
        case PS4_DPAD_MASK_DOWN:
            gamepad.state.down = true;
            break;
        case PS4_DPAD_MASK_DOWN_LEFT:
            gamepad.state.down = true;
            gamepad.state.left = true;
            break;
        case PS4_DPAD_MASK_LEFT:
            gamepad.state.left = true;
            break;
        case PS4_DPAD_MASK_LEFT_UP:
            gamepad.state.left = true;
            gamepad.state.up = true;
            break;
    }

    if (ds4_data->square)   gamepad.state.x = true;
    if (ds4_data->cross)    gamepad.state.a = true;
    if (ds4_data->circle)   gamepad.state.b = true;
    if (ds4_data->triangle) gamepad.state.y = true;

    if (ds4_data->share)    gamepad.state.back = true;
    if (ds4_data->option)   gamepad.state.start = true;
    if (ds4_data->ps)       gamepad.state.sys = true;
    if (ds4_data->tpad)     gamepad.state.misc = true;

    if (ds4_data->l1) gamepad.state.lb = true;
    if (ds4_data->r1) gamepad.state.rb = true;

    if (ds4_data->l3) gamepad.state.l3 = true;
    if (ds4_data->r3) gamepad.state.r3 = true;

    gamepad.state.lt = ds4_data->l2_trigger;
    gamepad.state.rt = ds4_data->r2_trigger;

    gamepad.state.lx = scale_uint8_to_int16(ds4_data->lx, false);
    gamepad.state.ly = scale_uint8_to_int16(ds4_data->ly, true);
    gamepad.state.rx = scale_uint8_to_int16(ds4_data->rx, false);
    gamepad.state.ry = scale_uint8_to_int16(ds4_data->ry, true);
}

void Dualshock4::process_report(uint8_t const* report, uint16_t len)
{
    // if (report[0] != REPORT_ID_GAMEPAD_STATE)
    // {
    //     return;
    // }

    static Dualshock4Report prev_report = { 0 };

    Dualshock4Report ds4_report;

    memcpy(&ds4_report, report, sizeof(ds4_report));

    if (memcmp(&ds4_report, &prev_report, sizeof(ds4_report)) != 0)
    {
        update_gamepad(&ds4_report);

        prev_report = ds4_report;
    }
}

bool Dualshock4::send_fb_data()
{
    Dualshock4OutReport output_report = {0};
    output_report.set_rumble = 1;
    output_report.motor_left = gamepadOut.out_state.lrumble;
    output_report.motor_right = gamepadOut.out_state.rrumble;
    
    return tuh_hid_send_report(dualshock4.dev_addr, dualshock4.instance, 5, &output_report, sizeof(output_report));
}