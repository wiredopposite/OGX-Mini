#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/psclassic/PSClassic.h"

void PSClassic::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    psclassic.player_id = player_id;
    tuh_hid_receive_report(dev_addr, instance);
}

void PSClassic::update_gamepad(Gamepad& gamepad, const PSClassicReport* psc_data) 
{
    gamepad.reset_pad();

    switch (psc_data->buttons & 0x3C00) {
        case PSCLASSIC_MASK_UP_LEFT:
            gamepad.buttons.up = true;
            gamepad.buttons.left = true;
            break;
        case PSCLASSIC_MASK_UP:
            gamepad.buttons.up = true;
            break;
        case PSCLASSIC_MASK_UP_RIGHT:
            gamepad.buttons.up = true;
            gamepad.buttons.right = true;
            break;
        case PSCLASSIC_MASK_LEFT:
            gamepad.buttons.left = true;
            break;
        case PSCLASSIC_MASK_RIGHT:
            gamepad.buttons.right = true;
            break;
        case PSCLASSIC_MASK_DOWN_LEFT:
            gamepad.buttons.down = true;
            gamepad.buttons.left = true;
            break;
        case PSCLASSIC_MASK_DOWN:
            gamepad.buttons.down = true;
            break;
        case PSCLASSIC_MASK_DOWN_RIGHT:
            gamepad.buttons.down = true;
            gamepad.buttons.right = true;
            break;
    }

    if (psc_data->buttons & PSCLASSIC_MASK_TRIANGLE)    gamepad.buttons.y = true;
    if (psc_data->buttons & PSCLASSIC_MASK_CIRCLE)      gamepad.buttons.b = true;
    if (psc_data->buttons & PSCLASSIC_MASK_CROSS)       gamepad.buttons.a = true;
    if (psc_data->buttons & PSCLASSIC_MASK_SQUARE)      gamepad.buttons.x = true;
    
    if (psc_data->buttons & PSCLASSIC_MASK_L2)          gamepad.triggers.l = 0xFF;
    if (psc_data->buttons & PSCLASSIC_MASK_R2)          gamepad.triggers.r = 0xFF;

    if (psc_data->buttons & PSCLASSIC_MASK_L1)          gamepad.buttons.lb = true;
    if (psc_data->buttons & PSCLASSIC_MASK_R1)          gamepad.buttons.rb = true;

    if (psc_data->buttons & PSCLASSIC_MASK_SELECT)      gamepad.buttons.back = true;
    if (psc_data->buttons & PSCLASSIC_MASK_START)       gamepad.buttons.start = true;
}

void PSClassic::process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    static PSClassicReport prev_report = { 0 };

    PSClassicReport psc_report;
    memcpy(&psc_report, report, sizeof(psc_report));

    if (memcmp(&psc_report, &prev_report, sizeof(psc_report)) != 0)
    {
        update_gamepad(gamepad, &psc_report);

        prev_report = psc_report;
    }
    
    tuh_hid_receive_report(dev_addr, instance);
}

void PSClassic::process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

void PSClassic::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}

bool PSClassic::send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance)
{
    return true;
}