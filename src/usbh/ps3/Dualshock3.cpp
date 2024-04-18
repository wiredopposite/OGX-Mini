#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "class/hid/hid_host.h"

#include "usbh/ps3/Dualshock3.h"

#include "usbh/shared/scaling.h"

void Dualshock3::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    dualshock3.player_id = player_id;
    dualshock3.response_count = 0;
    dualshock3.reports_enabled = false;

    // tuh_hid_get_report not working correctly?
    tusb_control_request_t setup_packet = 
    {
        .bmRequestType = 0xA1, 
        .bRequest = 0x01, // GET_REPORT
        .wValue = (HID_REPORT_TYPE_FEATURE << 8) | 0xF2, 
        .wIndex = 0x0000,      
        .wLength = 17         
    };

    tuh_xfer_s transfer = 
    {
        .daddr = dev_addr,
        .ep_addr = 0x00,     
        .setup = &setup_packet, 
        .buffer = (uint8_t*)&dualshock3.en_buffer,
        .complete_cb = NULL, 
        .user_data = 0          
    };

    if (tuh_control_xfer(&transfer))
    {
        get_report_complete_cb(dev_addr, instance);
    }

    // tuh_hid_get_report(dev_addr, instance, 0xF2, HID_REPORT_TYPE_FEATURE, &dualshock3.en_buffer, 17);
}

void Dualshock3::get_report_complete_cb(uint8_t dev_addr, uint8_t instance)
{   
    if (dualshock3.response_count == 0)
    {
        tusb_control_request_t setup_packet = 
        {
            .bmRequestType = 0xA1,  
            .bRequest = 0x01, // GET_REPORT
            .wValue = (HID_REPORT_TYPE_FEATURE << 8) | 0xF2, 
            .wIndex = 0x0000,    
            .wLength = 17     
        };

        tuh_xfer_s transfer = 
        {
            .daddr = dev_addr,
            .ep_addr = 0x00,
            .setup = &setup_packet, 
            .buffer = (uint8_t*)&dualshock3.en_buffer,
            .complete_cb = NULL, 
            .user_data = 0
        };

        if (tuh_control_xfer(&transfer))
        {
            dualshock3.response_count++;
            get_report_complete_cb(dev_addr, instance);
            return;
        }
    }
    else if (dualshock3.response_count == 1)
    {
        tusb_control_request_t setup_packet = 
        {
            .bmRequestType = 0xA1, 
            .bRequest = 0x01, // GET_REPORT
            .wValue = (HID_REPORT_TYPE_FEATURE << 8) | 0xF2,
            .wIndex = 0x0000, 
            .wLength = 8 
        };

        tuh_xfer_s transfer = 
        {
            .daddr = dev_addr,
            .ep_addr = 0x00, 
            .setup = &setup_packet,
            .buffer = (uint8_t*)&dualshock3.en_buffer,
            .complete_cb = NULL, 
            .user_data = 0          
        };

        if (tuh_control_xfer(&transfer))
        {
            dualshock3.response_count++;
            get_report_complete_cb(dev_addr, instance);
            return;
        }
    }
    else if (dualshock3.response_count == 2)
    {
        dualshock3.response_count++;

        uint8_t default_report[] = 
        {
            0x01, 0xff, 0x00, 0xff, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0x00, 0x00, 0x00, 0x00, 0x00
        };

        memcpy(&dualshock3.out_report, &default_report, sizeof(Dualshock3OutReport));

        dualshock3.out_report.leds_bitmap = 0x1 << (instance + 1);
        dualshock3.out_report.led[instance].time_enabled = UINT8_MAX;

        tusb_control_request_t setup_packet = 
        {
            .bmRequestType = 0x21,
            .bRequest = 0x09, // SET_REPORT
            .wValue = 0x0201,
            .wIndex = 0x0000,   
            .wLength = sizeof(Dualshock3OutReport)
        };

        tuh_xfer_s transfer = 
        {
            .daddr = dev_addr, 
            .ep_addr = 0x00, 
            .setup = &setup_packet, 
            .buffer = (uint8_t*)&dualshock3.out_report, 
            .complete_cb = NULL,
            .user_data = 0 
        };

        if (tuh_control_xfer(&transfer))
        {
            dualshock3.reports_enabled = true;
            tuh_hid_receive_report(dev_addr, instance);
        }
    }
}

void Dualshock3::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{   
    // if (dualshock3.response_count == 0)
    // {
    //     if (tuh_hid_get_report(dev_addr, instance, 0xF2, HID_REPORT_TYPE_FEATURE, &dualshock3.en_buffer, 17))
    //     {
    //         dualshock3.response_count++;
    //     }
    // }
    // else if (dualshock3.response_count == 1)
    // {
    //     if (tuh_hid_get_report(dev_addr, instance, 0xF5, HID_REPORT_TYPE_FEATURE, &dualshock3.en_buffer, 8))
    //     {
    //         dualshock3.response_count++;
    //     }
    // }
    // else if (dualshock3.response_count == 2)
    // {
    //     dualshock3.response_count++;

    //     uint8_t default_report[] = 
    //     {
    //         0x01, 0xff, 0x00, 0xff, 0x00,
    //         0x00, 0x00, 0x00, 0x00, 0x00,
    //         0xff, 0x27, 0x10, 0x00, 0x32,
    //         0xff, 0x27, 0x10, 0x00, 0x32,
    //         0xff, 0x27, 0x10, 0x00, 0x32,
    //         0xff, 0x27, 0x10, 0x00, 0x32,
    //         0x00, 0x00, 0x00, 0x00, 0x00
    //     };

    //     memcpy(&dualshock3.out_report, &default_report, sizeof(Dualshock3OutReport));

    //     dualshock3.out_report.leds_bitmap = 0x1 << (instance + 1);
    //     dualshock3.out_report.led[instance].time_enabled = UINT8_MAX;

    //     tusb_control_request_t setup_packet = 
    //     {
    //         .bmRequestType = 0x21,
    //         .bRequest = 0x09, // SET_REPORT
    //         .wValue = 0x0201,
    //         .wIndex = 0x0000,   
    //         .wLength = sizeof(Dualshock3OutReport)
    //     };

    //     tuh_xfer_s transfer = 
    //     {
    //         .daddr = dev_addr, 
    //         .ep_addr = 0x00, 
    //         .setup = &setup_packet, 
    //         .buffer = (uint8_t*)&dualshock3.out_report, 
    //         .complete_cb = NULL,
    //         .user_data = 0 
    //     };

    //     if (tuh_control_xfer(&transfer))
    //     {
    //         dualshock3.reports_enabled = true;
    //         tuh_hid_receive_report(dev_addr, instance);
    //     }
    // }
}

void Dualshock3::update_gamepad(Gamepad& gamepad, const Dualshock3Report* ds3_data)
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

    gamepad.triggers.l = ds3_data->l2_axis;
    gamepad.triggers.r = ds3_data->r2_axis;

    gamepad.joysticks.lx = scale_uint8_to_int16(ds3_data->left_x, false);
    gamepad.joysticks.ly = scale_uint8_to_int16(ds3_data->left_y, true);
    gamepad.joysticks.rx = scale_uint8_to_int16(ds3_data->right_x, false);
    gamepad.joysticks.ry = scale_uint8_to_int16(ds3_data->right_y, true);
}

void Dualshock3::process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    static Dualshock3Report prev_report = { 0 };
    Dualshock3Report ds3_report;
    memcpy(&ds3_report, report, sizeof(ds3_report));

    if (memcmp(&ds3_report, &prev_report, sizeof(ds3_report)) != 0)
    {
        update_gamepad(gamepad, &ds3_report);
        prev_report = ds3_report;
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void Dualshock3::process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool Dualshock3::send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance)
{
    static absolute_time_t next_allowed_time = {0};
    absolute_time_t current_time = get_absolute_time();

    if (!dualshock3.reports_enabled)
    {
        return false;
    }

    dualshock3.out_report.rumble.right_duration    = (gamepad.rumble.r > 0) ? 20: 0;
    dualshock3.out_report.rumble.right_motor_on    = (gamepad.rumble.r > 0) ? 1 : 0;

    dualshock3.out_report.rumble.left_duration     = (gamepad.rumble.l > 0) ? 20 : 0;
    dualshock3.out_report.rumble.left_motor_force  = gamepad.rumble.l;

    if (gamepad.rumble.l > 0 || gamepad.rumble.r > 0 || 
        absolute_time_diff_us(current_time, next_allowed_time) < 0) 
    {
        tusb_control_request_t setup_packet = 
        {
            .bmRequestType = 0x21,
            .bRequest = 0x09,
            .wValue = 0x0201,
            .wIndex = 0x0000, 
            .wLength = sizeof(Dualshock3OutReport)
        };

        tuh_xfer_s transfer = 
        {
            .daddr = dev_addr,
            .ep_addr = 0x00, 
            .setup = &setup_packet, 
            .buffer = (uint8_t*)&dualshock3.out_report,
            .complete_cb = NULL,       
            .user_data = 0          
        };

        if (tuh_control_xfer(&transfer)) 
        {
            if (gamepad.rumble.l == 0 && gamepad.rumble.r == 0) 
            {
                next_allowed_time = delayed_by_us(get_absolute_time(), 500000);
            }

            return true;
        }
    }

    return false;
}