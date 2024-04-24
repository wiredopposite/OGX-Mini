#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "class/hid/hid_host.h"

#include "usbh/ps3/DInput.h"
#include "usbh/shared/scaling.h"

void DInput::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    dinput.player_id = player_id;

    tuh_hid_receive_report(dev_addr, instance);
}

void DInput::update_gamepad(Gamepad* gamepad, const DInputReport* dinput_report)
{
    gamepad->reset_pad(gamepad);

    switch (dinput_report->direction)
    {
        case DINPUT_HAT_UP:
            gamepad->buttons.up = true;
            break;
        case DINPUT_HAT_UPRIGHT:
            gamepad->buttons.up = true;
            gamepad->buttons.right = true;
            break;
        case DINPUT_HAT_RIGHT:
            gamepad->buttons.right = true;
            break;
        case DINPUT_HAT_DOWNRIGHT:
            gamepad->buttons.right = true;
            gamepad->buttons.down = true;
            break;
        case DINPUT_HAT_DOWN:
            gamepad->buttons.down = true;
            break;
        case DINPUT_HAT_DOWNLEFT:
            gamepad->buttons.down = true;
            gamepad->buttons.left = true;
            break;
        case DINPUT_HAT_LEFT:
            gamepad->buttons.left = true;
            break;
        case DINPUT_HAT_UPLEFT:
            gamepad->buttons.up = true;
            gamepad->buttons.left = true;
            break;
    }

    if (dinput_report->square_btn)   gamepad->buttons.x = true;
    if (dinput_report->triangle_btn) gamepad->buttons.y = true;
    if (dinput_report->cross_btn)    gamepad->buttons.a = true;
    if (dinput_report->circle_btn)   gamepad->buttons.b = true;

    if (dinput_report->select_btn)   gamepad->buttons.back = true;
    if (dinput_report->start_btn)    gamepad->buttons.start = true;
    if (dinput_report->ps_btn)       gamepad->buttons.sys = true;

    if (dinput_report->l3_btn)       gamepad->buttons.l3 = true;
    if (dinput_report->r3_btn)       gamepad->buttons.r3 = true;
         
    if (dinput_report->l1_btn)       gamepad->buttons.lb = true;
    if (dinput_report->r1_btn)       gamepad->buttons.rb = true;

    if (dinput_report->l2_axis > 0)  
    {
        gamepad->triggers.l = dinput_report->l2_axis;
    }
    else if (dinput_report->l2_btn)
    {
        gamepad->triggers.l = 0xFF;
    }

    if (dinput_report->r2_axis > 0)
    {
        gamepad->triggers.r = dinput_report->r2_axis;
    }
    else if (dinput_report->r2_btn)  
    {
        gamepad->triggers.r = 0xFF;
    }

    gamepad->analog_buttons.up    = dinput_report->up_axis;
    gamepad->analog_buttons.down  = dinput_report->down_axis;
    gamepad->analog_buttons.left  = dinput_report->left_axis;
    gamepad->analog_buttons.right = dinput_report->right_axis;

    gamepad->analog_buttons.x = dinput_report->square_axis;
    gamepad->analog_buttons.y = dinput_report->triangle_axis;
    gamepad->analog_buttons.a = dinput_report->cross_axis;
    gamepad->analog_buttons.b = dinput_report->circle_axis;

    gamepad->analog_buttons.lb = dinput_report->l1_axis;
    gamepad->analog_buttons.rb = dinput_report->r1_axis;

    gamepad->joysticks.lx = scale_uint8_to_int16(dinput_report->l_x_axis, false);
    gamepad->joysticks.ly = scale_uint8_to_int16(dinput_report->l_y_axis, true);
    gamepad->joysticks.rx = scale_uint8_to_int16(dinput_report->r_x_axis, false);
    gamepad->joysticks.ry = scale_uint8_to_int16(dinput_report->r_y_axis, true);
}

void DInput::process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void)len;

    static DInputReport prev_report = {};
    DInputReport dinput_report;
    memcpy(&dinput_report, report, sizeof(dinput_report));

    if (memcmp(&dinput_report, &prev_report, sizeof(dinput_report)) != 0)
    {
        update_gamepad(gamepad, &dinput_report);
        prev_report = dinput_report;
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void DInput::process_xinput_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) 
{
    (void)gamepad;
    (void)dev_addr;
    (void)instance;
    (void)report;
    (void)len;
}

void DInput::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) 
{
    (void)dev_addr;
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)len;
}

bool DInput::send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance)
{
    (void)gamepad;
    (void)dev_addr;
    (void)instance;

    return true;
}