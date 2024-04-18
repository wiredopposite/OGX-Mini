#include <stdint.h>
#include <cmath>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/switch/SwitchPro.h"

void SwitchPro::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    switch_pro.player_id = player_id;
}

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

void SwitchPro::process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
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
        update_gamepad(gamepad, &switch_report);

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

void SwitchPro::update_gamepad(Gamepad& gamepad, const SwitchProReport* switch_report)
{
    gamepad.reset_pad();

    if (switch_report->up)    gamepad.buttons.up =true;
    if (switch_report->down)  gamepad.buttons.down =true;
    if (switch_report->left)  gamepad.buttons.left =true;
    if (switch_report->right) gamepad.buttons.right =true;

    if (switch_report->y) gamepad.buttons.x = true;
    if (switch_report->x) gamepad.buttons.y = true;
    if (switch_report->b) gamepad.buttons.a = true;
    if (switch_report->a) gamepad.buttons.b = true;

    if (switch_report->minus)   gamepad.buttons.back = true;
    if (switch_report->plus)    gamepad.buttons.start = true;
    if (switch_report->home)    gamepad.buttons.sys = true;
    if (switch_report->capture) gamepad.buttons.misc = true;

    if (switch_report->stickL) gamepad.buttons.l3 = true;
    if (switch_report->stickR) gamepad.buttons.r3 = true;

    if (switch_report->l) gamepad.buttons.lb = true;
    if (switch_report->r) gamepad.buttons.rb = true;

    if (switch_report->zl) gamepad.triggers.l = 0xFF;
    if (switch_report->zr) gamepad.triggers.r = 0xFF;

    gamepad.joysticks.lx = normalize_axes(switch_report->leftX );
    gamepad.joysticks.ly = normalize_axes(switch_report->leftY );
    gamepad.joysticks.rx = normalize_axes(switch_report->rightX);
    gamepad.joysticks.ry = normalize_axes(switch_report->rightY);
}

void SwitchPro::process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

void SwitchPro::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}

bool SwitchPro::send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance)
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

    SwitchProOutReport report = {};
    uint8_t report_size = 10;

    report.command = CMD_RUMBLE_ONLY;
    report.sequence_counter = get_output_sequence_counter();

    if (gamepad.rumble.l > 0) 
    {
        uint8_t amplitude_l = static_cast<uint8_t>(((gamepad.rumble.l / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        report.rumble_l[0] = amplitude_l;
        report.rumble_l[1] = 0x88;
        report.rumble_l[2] = amplitude_l / 2;
        report.rumble_l[3] = 0x61;  
    } 
    else 
    {
        report.rumble_l[0] = 0x00;
        report.rumble_l[1] = 0x01;
        report.rumble_l[2] = 0x40;
        report.rumble_l[3] = 0x40;           
    }

    if (gamepad.rumble.r > 0) 
    {
        uint8_t amplitude_r = static_cast<uint8_t>(((gamepad.rumble.r / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        report.rumble_r[0] = amplitude_r;
        report.rumble_r[1] = 0x88;
        report.rumble_r[2] = amplitude_r / 2;
        report.rumble_r[3] = 0x61;
    } 
    else 
    {
        report.rumble_r[0] = 0x00;
        report.rumble_r[1] = 0x01;
        report.rumble_r[2] = 0x40;
        report.rumble_r[3] = 0x40;   
    }

    if (!switch_pro.commands_sent)
    {
        if (!switch_pro.led_set)
        {
            report_size = 12;

            report.command = CMD_AND_RUMBLE;
            report.sub_command = CMD_LED;
            report.sub_command_args[0] = switch_pro.player_id;

            switch_pro.led_set = tuh_hid_send_report(dev_addr, instance, 0, &report, report_size);

            return switch_pro.led_set;
        }
        else if (!switch_pro.led_home_set)
        {
            report_size = 14;

            report.command = CMD_AND_RUMBLE;
            report.sub_command = CMD_LED_HOME;
            report.sub_command_args[0] = (0 /* Number of cycles */ << 4) | (true ? 0xF : 0);
            report.sub_command_args[1] = (0xF /* LED start intensity */ << 4) | 0x0 /* Number of full cycles */;
            report.sub_command_args[2] = (0xF /* Mini Cycle 1 LED intensity */ << 4) | 0x0 /* Mini Cycle 2 LED intensity */;

            switch_pro.led_home_set = tuh_hid_send_report(dev_addr, instance, 0, &report, report_size);
            
            return switch_pro.led_home_set;
        }
        else if (!switch_pro.full_report_enabled)
        {
            report_size = 12;

            report.command = CMD_AND_RUMBLE;
            report.sub_command = CMD_MODE;
            report.sub_command_args[0] = SUBCMD_FULL_REPORT_MODE;
            
            switch_pro.full_report_enabled = tuh_hid_send_report(dev_addr, instance, 0, &report, report_size);
            
            return switch_pro.full_report_enabled;
        }
        else if (!switch_pro.imu_enabled)
        {
            report_size = 12;
            
            report.command = CMD_AND_RUMBLE;
            report.sub_command = CMD_GYRO;
            report.sub_command_args[0] = 1 ? 1 : 0;
            
            switch_pro.imu_enabled = tuh_hid_send_report(dev_addr, instance, 0, &report, report_size);
            
            return switch_pro.imu_enabled;
        }
        else
        {
            switch_pro.commands_sent = true;
        }
    }

    return tuh_hid_send_report(dev_addr, instance, 0, &report, report_size);
}