#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/tusb_hid/psclassic.h"
// #include "usbh/tusb_hid/ps4.h"

#include "Gamepad.h"

void PSClassic::init(uint8_t dev_addr, uint8_t instance)
{
    psclassic.dev_addr = dev_addr;
    psclassic.instance = instance;
}

void PSClassic::update_gamepad(const PSClassicReport* psc_data) 
{
    gamepad.reset_state();

    switch (psc_data->buttons & 0x3C00) {
        case PSCLASSIC_MASK_UP_LEFT:
            gamepad.state.up = true;
            gamepad.state.left = true;
            break;
        case PSCLASSIC_MASK_UP:
            gamepad.state.up = true;
            break;
        case PSCLASSIC_MASK_UP_RIGHT:
            gamepad.state.up = true;
            gamepad.state.right = true;
            break;
        case PSCLASSIC_MASK_LEFT:
            gamepad.state.left = true;
            break;
        case PSCLASSIC_MASK_RIGHT:
            gamepad.state.right = true;
            break;
        case PSCLASSIC_MASK_DOWN_LEFT:
            gamepad.state.down = true;
            gamepad.state.left = true;
            break;
        case PSCLASSIC_MASK_DOWN:
            gamepad.state.down = true;
            break;
        case PSCLASSIC_MASK_DOWN_RIGHT:
            gamepad.state.down = true;
            gamepad.state.right = true;
            break;
    }

    if (psc_data->buttons & PSCLASSIC_MASK_TRIANGLE)    gamepad.state.y = true;
    if (psc_data->buttons & PSCLASSIC_MASK_CIRCLE)      gamepad.state.b = true;
    if (psc_data->buttons & PSCLASSIC_MASK_CROSS)       gamepad.state.a = true;
    if (psc_data->buttons & PSCLASSIC_MASK_SQUARE)      gamepad.state.x = true;
    
    if (psc_data->buttons & PSCLASSIC_MASK_L2)          gamepad.state.lt = 0xFF;
    if (psc_data->buttons & PSCLASSIC_MASK_R2)          gamepad.state.rt = 0xFF;

    if (psc_data->buttons & PSCLASSIC_MASK_L1)          gamepad.state.lb = true;
    if (psc_data->buttons & PSCLASSIC_MASK_R1)          gamepad.state.rb = true;

    if (psc_data->buttons & PSCLASSIC_MASK_SELECT)      gamepad.state.back = true;
    if (psc_data->buttons & PSCLASSIC_MASK_START)       gamepad.state.start = true;
}

void PSClassic::process_report(uint8_t const* report, uint16_t len)
{
    static PSClassicReport prev_report = { 0 };

    PSClassicReport psc_report;
    memcpy(&psc_report, report, sizeof(psc_report));

    if (memcmp(&psc_report, &prev_report, sizeof(psc_report)) != 0)
    {
        update_gamepad(&psc_report);

        prev_report = psc_report;
    }
}

bool PSClassic::send_fb_data()
{
    return true;
}