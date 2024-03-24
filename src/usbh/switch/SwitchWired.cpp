#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"

#include "usbh/switch/SwitchWired.h"

#include "utilities/scaling.h"
#include "descriptors/SwitchDescriptors.h"

void SwitchWired::init(uint8_t dev_addr, uint8_t instance) 
{
    tuh_hid_receive_report(dev_addr, instance);
}

void SwitchWired::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    static SwitchWiredReport prev_report = {0};

    SwitchWiredReport switch_report;
    memcpy(&switch_report, report, sizeof(switch_report));

    if (memcmp(&switch_report, &prev_report, sizeof(switch_report)) != 0)
    {
        update_gamepad(gp, &switch_report);

        prev_report = switch_report;
    }       

    tuh_hid_receive_report(dev_addr, instance);
}

void SwitchWired::update_gamepad(Gamepad& gp, const SwitchWiredReport* switch_report)
{
    gp.reset_state();

    switch (switch_report->dpad)
    {
        case SWITCH_HAT_UP:
            gp.state.up = true;
            break;
        case SWITCH_HAT_UPRIGHT:
            gp.state.up = true;
            gp.state.right = true;
            break;
        case SWITCH_HAT_RIGHT:
            gp.state.right = true;
            break;
        case SWITCH_HAT_DOWNRIGHT:
            gp.state.right = true;
            gp.state.down = true;
            break;
        case SWITCH_HAT_DOWN:
            gp.state.down = true;
            break;
        case SWITCH_HAT_DOWNLEFT:
            gp.state.down = true;
            gp.state.left = true;
            break;
        case SWITCH_HAT_LEFT:
            gp.state.left = true;
            break;
        case SWITCH_HAT_UPLEFT:
            gp.state.up = true;
            gp.state.left = true;
            break;
    }

    if (switch_report->b) gp.state.a = true;
    if (switch_report->a) gp.state.b = true;
    if (switch_report->y) gp.state.x = true;
    if (switch_report->x) gp.state.y = true;

    if (switch_report->minus) gp.state.back = true;
    if (switch_report->plus) gp.state.start = true;
    if (switch_report->l3) gp.state.l3 = true;
    if (switch_report->r3) gp.state.r3 = true;

    if(switch_report->home) gp.state.sys = true;
    if(switch_report->capture) gp.state.misc = true;

    if(switch_report->l) gp.state.lb = true;
    if(switch_report->r) gp.state.rb = true;

    if(switch_report->lz) gp.state.lt = 0xFF;
    if(switch_report->rz) gp.state.rt = 0xFF;

    gp.state.lx = scale_uint8_to_int16(switch_report->lx, false);
    gp.state.ly = scale_uint8_to_int16(switch_report->ly, true);
    gp.state.rx = scale_uint8_to_int16(switch_report->rx, false);
    gp.state.ry = scale_uint8_to_int16(switch_report->ry, true);
}

void SwitchWired::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool SwitchWired::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
{
    return true; // not aware of a wired switch gamepad with rumble
}