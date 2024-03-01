#include <stdint.h>
#include "pico/stdlib.h"
#include <cmath>

#include "tusb.h"

#include "usbh/tusb_hid/shared.h"
#include "usbh/tusb_hid/switch_pro.h"

#include "Gamepad.h"

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

void SwitchPro::init(uint8_t dev_addr, uint8_t instance)
{
    reset_state();

    switch_pro.dev_addr = dev_addr;
    switch_pro.instance = instance;

    if (tuh_hid_send_ready)
    {
        uint8_t handshake_command[2] = {CMD_HID, SUBCMD_HANDSHAKE};
        switch_pro.handshake_sent = tuh_hid_send_report(switch_pro.dev_addr, switch_pro.instance, 0, handshake_command, sizeof(handshake_command));
    }
}

void SwitchPro::reset_state()
{
    switch_pro.handshake_sent = false;
    switch_pro.timeout_disabled = false;
    switch_pro.full_report_enabled = false;
    switch_pro.led_set = false;
    switch_pro.led_home_set = false;
    switch_pro.imu_enabled = false;
    switch_pro.dev_addr = 0;
    switch_pro.instance = 0;
    switch_pro.output_sequence_counter = 0;
}

uint8_t SwitchPro::get_output_sequence_counter()
{
    // increments each report, resets to 0 after 15
    uint8_t counter = switch_pro.output_sequence_counter;
    switch_pro.output_sequence_counter = (switch_pro.output_sequence_counter + 1) & 0x0F;
    return counter;
}

bool SwitchPro::disable_timeout()
{
    uint8_t disable_timeout_command[2] = {CMD_HID, SUBCMD_DISABLE_TIMEOUT};

    if (tuh_hid_send_ready)
    {
        switch_pro.timeout_disabled = tuh_hid_send_report(switch_pro.dev_addr, switch_pro.instance, 0, disable_timeout_command, sizeof(disable_timeout_command));
    }

    return switch_pro.timeout_disabled;
}

void SwitchPro::process_report(uint8_t const* report, uint16_t len)
{
    if (switch_pro.handshake_sent)
    {
        // this is here to the respond to handshake, doesn't seem to work otherwise
        if (!switch_pro.timeout_disabled)
        {
            disable_timeout();
            return;
        }
        
        static SwitchProReport prev_report = {0};

        SwitchProReport switch_report;
        memcpy(&switch_report, report, sizeof(switch_report));

        if (memcmp(&switch_report, &prev_report, sizeof(switch_report)) != 0)
        {
            update_gamepad(&switch_report);

            prev_report = switch_report;
        }
    }
}

int16_t SwitchPro::normalize_axes(uint16_t value) 
{
    /*  12bit value from the controller doesnt cover the full 12bit range seemingly
        doesn't seem completely centered at 2047 so I may be missing something here
        tried to get as close as possible with the multiplier */

    int32_t normalized_value = (value - 2047) * 22;

    if (normalized_value < -32768) 
    {
        normalized_value = -32768;
    } 
    else if (normalized_value > 32767) 
    {
        normalized_value = 32767;
    }

    return (int16_t)normalized_value;   
}

void SwitchPro::update_gamepad(const SwitchProReport* switch_report)
{
    gamepad.reset_state();

    if (switch_report->up)    gamepad.state.up =true;
    if (switch_report->down)  gamepad.state.down =true;
    if (switch_report->left)  gamepad.state.left =true;
    if (switch_report->right) gamepad.state.right =true;

    if (switch_report->y) gamepad.state.x = true;
    if (switch_report->x) gamepad.state.y = true;
    if (switch_report->b) gamepad.state.a = true;
    if (switch_report->a) gamepad.state.b = true;

    if (switch_report->minus)   gamepad.state.back = true;
    if (switch_report->plus)    gamepad.state.start = true;
    if (switch_report->home)    gamepad.state.sys = true;
    if (switch_report->capture) gamepad.state.misc = true;

    if (switch_report->stickL) gamepad.state.l3 = true;
    if (switch_report->stickR) gamepad.state.r3 = true;

    if (switch_report->l) gamepad.state.lb = true;
    if (switch_report->r) gamepad.state.rb = true;

    if (switch_report->zl) gamepad.state.lt = 0xFF;
    if (switch_report->zr) gamepad.state.rt = 0xFF;

    gamepad.state.lx = normalize_axes(switch_report->leftX );
    gamepad.state.ly = normalize_axes(switch_report->leftY );
    gamepad.state.rx = normalize_axes(switch_report->rightX);
    gamepad.state.ry = normalize_axes(switch_report->rightY);
}

bool SwitchPro::send_fb_data()
{
    if (!switch_pro.handshake_sent || !switch_pro.timeout_disabled)
    {
        return false;
    }

    // See: https://github.com/Dan611/hid-procon
    //      https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering
    //      https://github.com/HisashiKato/USB_Host_Shield_Library_2.0

    uint8_t buffer[14] = { 0 };
    uint8_t report_size = 10;

    buffer[1] = get_output_sequence_counter();

    // whoever came up with hd rumble is some kind of sick freak, I'm just guessing here 

    if (gamepadOut.out_state.lrumble > 0) 
    {
        // full on
        // buffer[2] = 0x28;
        // buffer[3] = 0x88;
        // buffer[4] = 0x60;
        // buffer[5] = 0x61;  

        // uint8_t amplitude_l = static_cast<uint8_t>((gamepadOut.out_state.lrumble / 255.0) * (0xC0 - 0x40) + 0x40);
        uint8_t amplitude_l = static_cast<uint8_t>(((gamepadOut.out_state.lrumble / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

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

    if (gamepadOut.out_state.rrumble > 0) 
    {
        // full on
        // buffer[6] = 0x28;
        // buffer[7] = 0x88;
        // buffer[8] = 0x60;
        // buffer[9] = 0x61;

        // uint8_t amplitude_r = static_cast<uint8_t>((gamepadOut.out_state.rrumble / 255.0) * (0xC0 - 0x40) + 0x40);
        uint8_t amplitude_r = static_cast<uint8_t>(((gamepadOut.out_state.rrumble / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

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
            switch_pro.led_set = tuh_hid_send_report(switch_pro.dev_addr, switch_pro.instance, 0, &buffer, report_size);
            return switch_pro.led_set;
        }
        else if (!switch_pro.led_home_set)
        {
            report_size = 14;
            buffer[10] = CMD_LED_HOME;
            buffer[11] = (0 /* Number of cycles */ << 4) | (true ? 0xF : 0);
            buffer[12] = (0xF /* LED start intensity */ << 4) | 0x0 /* Number of full cycles */;
            buffer[13] = (0xF /* Mini Cycle 1 LED intensity */ << 4) | 0x0 /* Mini Cycle 2 LED intensity */;
            switch_pro.led_home_set = tuh_hid_send_report(switch_pro.dev_addr, switch_pro.instance, 0, &buffer, report_size);
            return switch_pro.led_home_set;
        }
        else if (!switch_pro.full_report_enabled)
        {
            report_size = 12;
            buffer[10] = CMD_MODE;
            buffer[11] = SUBCMD_FULL_REPORT_MODE;
            switch_pro.full_report_enabled = tuh_hid_send_report(switch_pro.dev_addr, switch_pro.instance, 0, &buffer, report_size);
            return switch_pro.full_report_enabled;
        }
        else if (!switch_pro.imu_enabled)
        {
            report_size = 12;
            buffer[10] = CMD_GYRO;
            buffer[11] = 1 ? 1 : 0;
            switch_pro.imu_enabled = tuh_hid_send_report(switch_pro.dev_addr, switch_pro.instance, 0, &buffer, report_size);
            return switch_pro.imu_enabled;
        }
        else
        {
            switch_pro.commands_sent = true;
        }
    }

    buffer[0] = CMD_RUMBLE_ONLY;

    return tuh_hid_send_report(switch_pro.dev_addr, switch_pro.instance, 0, &buffer, report_size);
}