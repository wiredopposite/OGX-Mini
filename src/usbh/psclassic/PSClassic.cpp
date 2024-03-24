#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/psclassic/PSClassic.h"

void PSClassic::init(uint8_t dev_addr, uint8_t instance) 
{
    tuh_hid_receive_report(dev_addr, instance);
}

void PSClassic::update_gamepad(Gamepad& gp, const PSClassicReport* psc_data) 
{
    gp.reset_state();

    switch (psc_data->buttons & 0x3C00) {
        case PSCLASSIC_MASK_UP_LEFT:
            gp.state.up = true;
            gp.state.left = true;
            break;
        case PSCLASSIC_MASK_UP:
            gp.state.up = true;
            break;
        case PSCLASSIC_MASK_UP_RIGHT:
            gp.state.up = true;
            gp.state.right = true;
            break;
        case PSCLASSIC_MASK_LEFT:
            gp.state.left = true;
            break;
        case PSCLASSIC_MASK_RIGHT:
            gp.state.right = true;
            break;
        case PSCLASSIC_MASK_DOWN_LEFT:
            gp.state.down = true;
            gp.state.left = true;
            break;
        case PSCLASSIC_MASK_DOWN:
            gp.state.down = true;
            break;
        case PSCLASSIC_MASK_DOWN_RIGHT:
            gp.state.down = true;
            gp.state.right = true;
            break;
    }

    if (psc_data->buttons & PSCLASSIC_MASK_TRIANGLE)    gp.state.y = true;
    if (psc_data->buttons & PSCLASSIC_MASK_CIRCLE)      gp.state.b = true;
    if (psc_data->buttons & PSCLASSIC_MASK_CROSS)       gp.state.a = true;
    if (psc_data->buttons & PSCLASSIC_MASK_SQUARE)      gp.state.x = true;
    
    if (psc_data->buttons & PSCLASSIC_MASK_L2)          gp.state.lt = 0xFF;
    if (psc_data->buttons & PSCLASSIC_MASK_R2)          gp.state.rt = 0xFF;

    if (psc_data->buttons & PSCLASSIC_MASK_L1)          gp.state.lb = true;
    if (psc_data->buttons & PSCLASSIC_MASK_R1)          gp.state.rb = true;

    if (psc_data->buttons & PSCLASSIC_MASK_SELECT)      gp.state.back = true;
    if (psc_data->buttons & PSCLASSIC_MASK_START)       gp.state.start = true;
}

void PSClassic::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    static PSClassicReport prev_report = { 0 };

    PSClassicReport psc_report;
    memcpy(&psc_report, report, sizeof(psc_report));

    if (memcmp(&psc_report, &prev_report, sizeof(psc_report)) != 0)
    {
        update_gamepad(gp, &psc_report);

        prev_report = psc_report;
    }
    
    tuh_hid_receive_report(dev_addr, instance);
}

void PSClassic::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool PSClassic::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
{
    return true;
}