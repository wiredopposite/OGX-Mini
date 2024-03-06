#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/time.h"

#include "tusb.h"

#include "utilities/scaling.h"
#include "descriptors/SwitchDescriptors.h"
#include "usbh/tusb_hid/switch_wired.h"

#include "Gamepad.h"

void SwitchWired::init(uint8_t dev_addr, uint8_t instance)
{
    reset_state();
    switch_wired.dev_addr = dev_addr;
    switch_wired.instance = instance;
}

void SwitchWired::reset_state()
{
    switch_wired.dev_addr = 0;
    switch_wired.instance = 0;
}

void SwitchWired::process_report(uint8_t const* report, uint16_t len)
{
    static SwitchWiredReport prev_report = {0};

    SwitchWiredReport switch_report;
    memcpy(&switch_report, report, sizeof(switch_report));

    if (memcmp(&switch_report, &prev_report, sizeof(switch_report)) != 0)
    {
        update_gamepad(&switch_report);

        prev_report = switch_report;
    }       
}

void SwitchWired::update_gamepad(const SwitchWiredReport* switch_report)
{
    gamepad.reset_state();

    switch (switch_report->dpad)
    {
        case SWITCH_HAT_UP:
            gamepad.state.up = true;
            break;
        case SWITCH_HAT_UPRIGHT:
            gamepad.state.up = true;
            gamepad.state.right = true;
            break;
        case SWITCH_HAT_RIGHT:
            gamepad.state.right = true;
            break;
        case SWITCH_HAT_DOWNRIGHT:
            gamepad.state.right = true;
            gamepad.state.down = true;
            break;
        case SWITCH_HAT_DOWN:
            gamepad.state.down = true;
            break;
        case SWITCH_HAT_DOWNLEFT:
            gamepad.state.down = true;
            gamepad.state.left = true;
            break;
        case SWITCH_HAT_LEFT:
            gamepad.state.left = true;
            break;
        case SWITCH_HAT_UPLEFT:
            gamepad.state.up = true;
            gamepad.state.left = true;
            break;
    }

    if (switch_report->b) gamepad.state.a = true;
    if (switch_report->a) gamepad.state.b = true;
    if (switch_report->y) gamepad.state.x = true;
    if (switch_report->x) gamepad.state.y = true;

    if (switch_report->minus) gamepad.state.back = true;
    if (switch_report->plus) gamepad.state.start = true;
    if (switch_report->l3) gamepad.state.l3 = true;
    if (switch_report->r3) gamepad.state.r3 = true;

    if(switch_report->home) gamepad.state.sys = true;
    if(switch_report->capture) gamepad.state.misc = true;

    if(switch_report->l) gamepad.state.lb = true;
    if(switch_report->r) gamepad.state.rb = true;

    if(switch_report->lz) gamepad.state.lt = 0xFF;
    if(switch_report->rz) gamepad.state.rt = 0xFF;

    gamepad.state.lx = scale_uint8_to_int16(switch_report->lx, false);
    gamepad.state.ly = scale_uint8_to_int16(switch_report->ly, true);
    gamepad.state.rx = scale_uint8_to_int16(switch_report->rx, false);
    gamepad.state.ry = scale_uint8_to_int16(switch_report->ry, true);
}

bool SwitchWired::send_fb_data()
{
    // mine doesn't have rumble motors, not sure what goes here
    return true;
}