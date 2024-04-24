#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"
#include "descriptors/SwitchDescriptors.h"

#include "usbh/switch/SwitchWired.h"
#include "usbh/shared/scaling.h"

void SwitchWired::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    switch_wired.player_id = player_id;
    tuh_hid_receive_report(dev_addr, instance);
}

void SwitchWired::process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void)len;

    static SwitchWiredReport prev_report = {};

    SwitchWiredReport switch_report;
    memcpy(&switch_report, report, sizeof(switch_report));

    if (memcmp(&switch_report, &prev_report, sizeof(switch_report)) != 0)
    {
        update_gamepad(gamepad, &switch_report);

        prev_report = switch_report;
    }       

    tuh_hid_receive_report(dev_addr, instance);
}

void SwitchWired::update_gamepad(Gamepad* gamepad, const SwitchWiredReport* switch_report)
{
    gamepad->reset_pad(gamepad);

    switch (switch_report->dpad)
    {
        case SWITCH_HAT_UP:
            gamepad->buttons.up = true;
            break;
        case SWITCH_HAT_UPRIGHT:
            gamepad->buttons.up = true;
            gamepad->buttons.right = true;
            break;
        case SWITCH_HAT_RIGHT:
            gamepad->buttons.right = true;
            break;
        case SWITCH_HAT_DOWNRIGHT:
            gamepad->buttons.right = true;
            gamepad->buttons.down = true;
            break;
        case SWITCH_HAT_DOWN:
            gamepad->buttons.down = true;
            break;
        case SWITCH_HAT_DOWNLEFT:
            gamepad->buttons.down = true;
            gamepad->buttons.left = true;
            break;
        case SWITCH_HAT_LEFT:
            gamepad->buttons.left = true;
            break;
        case SWITCH_HAT_UPLEFT:
            gamepad->buttons.up = true;
            gamepad->buttons.left = true;
            break;
    }

    if (switch_report->b) gamepad->buttons.a = true;
    if (switch_report->a) gamepad->buttons.b = true;
    if (switch_report->y) gamepad->buttons.x = true;
    if (switch_report->x) gamepad->buttons.y = true;

    if (switch_report->minus) gamepad->buttons.back = true;
    if (switch_report->plus) gamepad->buttons.start = true;
    if (switch_report->l3) gamepad->buttons.l3 = true;
    if (switch_report->r3) gamepad->buttons.r3 = true;

    if(switch_report->home) gamepad->buttons.sys = true;
    if(switch_report->capture) gamepad->buttons.misc = true;

    if(switch_report->l) gamepad->buttons.lb = true;
    if(switch_report->r) gamepad->buttons.rb = true;

    if(switch_report->lz) gamepad->triggers.l = 0xFF;
    if(switch_report->rz) gamepad->triggers.r = 0xFF;

    gamepad->joysticks.lx = scale_uint8_to_int16(switch_report->lx, false);
    gamepad->joysticks.ly = scale_uint8_to_int16(switch_report->ly, true);
    gamepad->joysticks.rx = scale_uint8_to_int16(switch_report->rx, false);
    gamepad->joysticks.ry = scale_uint8_to_int16(switch_report->ry, true);
}

void SwitchWired::process_xinput_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) 
{
    (void)gamepad;
    (void)dev_addr;
    (void)instance;
    (void)report;
    (void)len;
}

void SwitchWired::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) 
{
    (void)dev_addr;
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)len;
}

bool SwitchWired::send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance)
{
    (void)gamepad;
    (void)dev_addr;
    (void)instance;
    
    return true; // not aware of a wired switch gamepad with rumble
}