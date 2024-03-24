#include <stdint.h>
#include <cmath>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/switch/SwitchPro.h"

// commands
#define CMD_HID 0x80
#define SUBCMD_HANDSHAKE 0x02
#define SUBCMD_DISABLE_TIMEOUT 0x04

// out report commands
#define CMD_RUMBLE_ONLY 0x10
#define CMD_AND_RUMBLE 0x01

// out report subcommands
#define CMD_LED 0x30
#define CMD_LED_HOME 0x38
#define CMD_GYRO 0x40
#define CMD_MODE 0x03
#define SUBCMD_FULL_REPORT_MODE 0x30

void SwitchPro::init(uint8_t dev_addr, uint8_t instance) {}

void SwitchPro::send_handshake(uint8_t dev_addr, uint8_t instance)
{
    if (tuh_hid_send_ready)
    {
        uint8_t handshake_command[2] = {CMD_HID, SUBCMD_HANDSHAKE};
        switch_pro.handshake_sent = tuh_hid_send_report(dev_addr, instance, 0, handshake_command, sizeof(handshake_command));
    }

    tuh_hid_receive_report(dev_addr, instance);
}

uint8_t SwitchPro::get_output_sequence_counter()
{
    // increments each report, resets to 0 after 15
    uint8_t counter = switch_pro.output_sequence_counter;
    switch_pro.output_sequence_counter = (switch_pro.output_sequence_counter + 1) & 0x0F;
    return counter;
}

void SwitchPro::disable_timeout(uint8_t dev_addr, uint8_t instance)
{
    if (tuh_hid_send_ready)
    {
        uint8_t disable_timeout_cmd[2] = {CMD_HID, SUBCMD_DISABLE_TIMEOUT};
        switch_pro.timeout_disabled = tuh_hid_send_report(dev_addr, instance, 0, disable_timeout_cmd, sizeof(disable_timeout_cmd));
    }
}

void SwitchPro::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    if (!switch_pro.handshake_sent)
    {
        return;
    }
    else if (!switch_pro.timeout_disabled)
    {
        disable_timeout(dev_addr, instance); // response to handshake
        tuh_hid_receive_report(dev_addr, instance);
        return;
    }
    
    static SwitchProReport prev_report = {0};

    SwitchProReport switch_report;
    memcpy(&switch_report, report, sizeof(switch_report));

    if (memcmp(&switch_report, &prev_report, sizeof(switch_report)) != 0)
    {
        update_gamepad(gp, &switch_report);

        prev_report = switch_report;
    }

    tuh_hid_receive_report(dev_addr, instance);
}

int16_t SwitchPro::normalize_axes(uint16_t value) 
{
    /*  12bit value from the controller doesnt cover the full 12bit range seemingly
        doesn't seem completely centered at 2047 so I may be missing something here
        tried to get as close as possible with the multiplier */

    int32_t normalized_value = (value - 2047) * 22;

    if (normalized_value < INT16_MIN) 
    {
        normalized_value = INT16_MIN;
    } 
    else if (normalized_value > INT16_MAX) 
    {
        normalized_value = INT16_MAX;
    }

    return (int16_t)normalized_value;   
}

void SwitchPro::update_gamepad(Gamepad& gp, const SwitchProReport* switch_report)
{
    gp.reset_state();

    if (switch_report->up)    gp.state.up =true;
    if (switch_report->down)  gp.state.down =true;
    if (switch_report->left)  gp.state.left =true;
    if (switch_report->right) gp.state.right =true;

    if (switch_report->y) gp.state.x = true;
    if (switch_report->x) gp.state.y = true;
    if (switch_report->b) gp.state.a = true;
    if (switch_report->a) gp.state.b = true;

    if (switch_report->minus)   gp.state.back = true;
    if (switch_report->plus)    gp.state.start = true;
    if (switch_report->home)    gp.state.sys = true;
    if (switch_report->capture) gp.state.misc = true;

    if (switch_report->stickL) gp.state.l3 = true;
    if (switch_report->stickR) gp.state.r3 = true;

    if (switch_report->l) gp.state.lb = true;
    if (switch_report->r) gp.state.rb = true;

    if (switch_report->zl) gp.state.lt = 0xFF;
    if (switch_report->zr) gp.state.rt = 0xFF;

    gp.state.lx = normalize_axes(switch_report->leftX );
    gp.state.ly = normalize_axes(switch_report->leftY );
    gp.state.rx = normalize_axes(switch_report->rightX);
    gp.state.ry = normalize_axes(switch_report->rightY);
}

void SwitchPro::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool SwitchPro::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
{
    if (!switch_pro.handshake_sent)
    {
        send_handshake(dev_addr, instance);
    }
    else if (!switch_pro.timeout_disabled)
    {
        return false;
    }

    // See: https://github.com/Dan611/hid-procon
    //      https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering
    //      https://github.com/HisashiKato/USB_Host_Shield_Library_2.0

    uint8_t buffer[14] = { 0 };
    uint8_t report_size = 10;

    buffer[0] = CMD_RUMBLE_ONLY;
    buffer[1] = get_output_sequence_counter();

    // whoever came up with "hd" rumble is some kind of sick freak, I'm just guessing here 

    if (gp_out.out_state.lrumble > 0) 
    {
        // full on
        // buffer[2] = 0x28;
        // buffer[3] = 0x88;
        // buffer[4] = 0x60;
        // buffer[5] = 0x61;  

        // uint8_t amplitude_l = static_cast<uint8_t>((gamepadOut.out_state.lrumble / 255.0) * (0xC0 - 0x40) + 0x40);
        uint8_t amplitude_l = static_cast<uint8_t>(((gp_out.out_state.lrumble / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        buffer[2] = amplitude_l;
        buffer[3] = 0x88;
        buffer[4] = amplitude_l / 2;
        buffer[5] = 0x61;  
    } 
    else 
    {
        buffer[2] = 0x00;
        buffer[3] = 0x01;
        buffer[4] = 0x40;
        buffer[5] = 0x40;           
    }

    if (gp_out.out_state.rrumble > 0) 
    {
        // full on
        // buffer[6] = 0x28;
        // buffer[7] = 0x88;
        // buffer[8] = 0x60;
        // buffer[9] = 0x61;

        // uint8_t amplitude_r = static_cast<uint8_t>((gamepadOut.out_state.rrumble / 255.0) * (0xC0 - 0x40) + 0x40);
        uint8_t amplitude_r = static_cast<uint8_t>(((gp_out.out_state.rrumble / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        buffer[6] = amplitude_r;
        buffer[7] = 0x88;
        buffer[8] = amplitude_r / 2;
        buffer[9] = 0x61;
    } 
    else 
    {
        buffer[6] = 0x00;
        buffer[7] = 0x01;
        buffer[8] = 0x40;
        buffer[9] = 0x40;   
    }

    if (!switch_pro.commands_sent)
    {
        buffer[0] = CMD_AND_RUMBLE;

        if (!switch_pro.led_set)
        {
            report_size = 12;
            buffer[10] = CMD_LED;
            buffer[11] = 0x01;
            switch_pro.led_set = tuh_hid_send_report(dev_addr, instance, 0, &buffer, report_size);
            return switch_pro.led_set;
        }
        else if (!switch_pro.led_home_set)
        {
            report_size = 14;
            buffer[10] = CMD_LED_HOME;
            buffer[11] = (0 /* Number of cycles */ << 4) | (true ? 0xF : 0);
            buffer[12] = (0xF /* LED start intensity */ << 4) | 0x0 /* Number of full cycles */;
            buffer[13] = (0xF /* Mini Cycle 1 LED intensity */ << 4) | 0x0 /* Mini Cycle 2 LED intensity */;
            switch_pro.led_home_set = tuh_hid_send_report(dev_addr, instance, 0, &buffer, report_size);
            return switch_pro.led_home_set;
        }
        else if (!switch_pro.full_report_enabled)
        {
            report_size = 12;
            buffer[10] = CMD_MODE;
            buffer[11] = SUBCMD_FULL_REPORT_MODE;
            switch_pro.full_report_enabled = tuh_hid_send_report(dev_addr, instance, 0, &buffer, report_size);
            return switch_pro.full_report_enabled;
        }
        else if (!switch_pro.imu_enabled)
        {
            report_size = 12;
            buffer[10] = CMD_GYRO;
            buffer[11] = 1 ? 1 : 0;
            switch_pro.imu_enabled = tuh_hid_send_report(dev_addr, instance, 0, &buffer, report_size);
            return switch_pro.imu_enabled;
        }
        else
        {
            switch_pro.commands_sent = true;
        }
    }

    return tuh_hid_send_report(dev_addr, instance, 0, &buffer, report_size);
}