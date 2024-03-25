#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "CRC32.h"

#include "usbh/ps4/Dualshock4.h"

#include "utilities/scaling.h"
#include "Gamepad.h"

#define REPORT_ID_GAMEPAD_STATE 0x11

void Dualshock4::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    dualshock4.player_id = player_id;
    // set_leds();
    tuh_hid_receive_report(dev_addr, instance);
}

/* this DCs the controller, come back to it */
bool Dualshock4::set_leds(uint8_t dev_addr, uint8_t instance)
{
    // see: https://github.com/Hydr8gon/VitaControl

    static uint8_t buffer[79] = {};
    buffer[0]  = 0xA2;
    buffer[1]  = 0x11;
    buffer[2]  = 0xC0;
    buffer[3]  = 0x20;
    buffer[4]  = 0xF3;
    buffer[5]  = 0x04;
    buffer[9]  = led_colors[instance][0];
    buffer[10] = led_colors[instance][1];
    buffer[11] = led_colors[instance][2];

    // Calculate the CRC of the data (including the 0xA2 byte) and append it to the end
    uint32_t crc = CRC32::calculate(buffer, 75);
    buffer[75] = crc >>  0;
    buffer[76] = crc >>  8;
    buffer[77] = crc >> 16;
    buffer[78] = crc >> 24;

    // Send the write request, omitting the 0xA2 byte
    dualshock4.leds_set = tuh_hid_send_report(dev_addr, instance, 0, &buffer + 1, sizeof(buffer) - 1);

    return dualshock4.leds_set;
}

void Dualshock4::update_gamepad(Gamepad& gp, const Dualshock4Report* ds4_data)
{
    gp.reset_state();

    switch(ds4_data->dpad)
    {
        case PS4_DPAD_MASK_UP:
            gp.state.up = true;
            break;
        case PS4_DPAD_MASK_UP_RIGHT:
            gp.state.up = true;
            gp.state.right = true;
            break;
        case PS4_DPAD_MASK_RIGHT:
            gp.state.right = true;
            break;
        case PS4_DPAD_MASK_RIGHT_DOWN:
            gp.state.right = true;
            gp.state.down = true;
            break;
        case PS4_DPAD_MASK_DOWN:
            gp.state.down = true;
            break;
        case PS4_DPAD_MASK_DOWN_LEFT:
            gp.state.down = true;
            gp.state.left = true;
            break;
        case PS4_DPAD_MASK_LEFT:
            gp.state.left = true;
            break;
        case PS4_DPAD_MASK_LEFT_UP:
            gp.state.left = true;
            gp.state.up = true;
            break;
    }

    if (ds4_data->square)   gp.state.x = true;
    if (ds4_data->cross)    gp.state.a = true;
    if (ds4_data->circle)   gp.state.b = true;
    if (ds4_data->triangle) gp.state.y = true;

    if (ds4_data->share)    gp.state.back = true;
    if (ds4_data->option)   gp.state.start = true;
    if (ds4_data->ps)       gp.state.sys = true;
    if (ds4_data->tpad)     gp.state.misc = true;

    if (ds4_data->l1) gp.state.lb = true;
    if (ds4_data->r1) gp.state.rb = true;

    if (ds4_data->l3) gp.state.l3 = true;
    if (ds4_data->r3) gp.state.r3 = true;

    gp.state.lt = ds4_data->l2_trigger;
    gp.state.rt = ds4_data->r2_trigger;

    gp.state.lx = scale_uint8_to_int16(ds4_data->lx, false);
    gp.state.ly = scale_uint8_to_int16(ds4_data->ly, true);
    gp.state.rx = scale_uint8_to_int16(ds4_data->rx, false);
    gp.state.ry = scale_uint8_to_int16(ds4_data->ry, true);
}

void Dualshock4::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    static Dualshock4Report prev_report = { 0 };
    Dualshock4Report ds4_report;
    memcpy(&ds4_report, report, sizeof(ds4_report));

    if (memcmp(&ds4_report, &prev_report, sizeof(ds4_report)) != 0)
    {
        update_gamepad(gp, &ds4_report);

        prev_report = ds4_report;
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void Dualshock4::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool Dualshock4::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
{
    Dualshock4OutReport out_report = {0};
    out_report.set_rumble = 1;
    out_report.motor_left = gp_out.state.lrumble;
    out_report.motor_right = gp_out.state.rrumble;
    
    return tuh_hid_send_report(dev_addr, instance, 5, &out_report, sizeof(out_report));
}