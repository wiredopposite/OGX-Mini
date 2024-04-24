#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/shared/scaling.h"
#include "usbh/n64usb/N64USB.h"

void N64USB::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance)
{
    n64usb.player_id = player_id;
    tuh_hid_receive_report(dev_addr, instance);
}

void N64USB::update_gamepad(Gamepad* gamepad, const N64USBReport* n64_data)
{
    gamepad->reset_pad(gamepad);

    uint8_t n64_dpad = n64_data->buttons & N64_DPAD_MASK;

    switch(n64_dpad)
    {
        case N64_DPAD_MASK_UP:
            gamepad->buttons.up = true;
            break;
        case N64_DPAD_MASK_UP_RIGHT:
            gamepad->buttons.up = true;
            gamepad->buttons.right = true;
            break;
        case N64_DPAD_MASK_RIGHT:
            gamepad->buttons.right = true;
            break;
        case N64_DPAD_MASK_RIGHT_DOWN:
            gamepad->buttons.right = true;
            gamepad->buttons.down = true;
            break;
        case N64_DPAD_MASK_DOWN:
            gamepad->buttons.down = true;
            break;
        case N64_DPAD_MASK_DOWN_LEFT:
            gamepad->buttons.down = true;
            gamepad->buttons.left = true;
            break;
        case N64_DPAD_MASK_LEFT:
            gamepad->buttons.left = true;
            break;
        case N64_DPAD_MASK_LEFT_UP:
            gamepad->buttons.left = true;
            gamepad->buttons.up = true;
            break;
    }

    if (n64_data->buttons & N64_C_UP_MASK)      gamepad->joysticks.ry = INT16_MAX;
    if (n64_data->buttons & N64_C_DOWN_MASK)    gamepad->joysticks.ry = INT16_MIN;
    if (n64_data->buttons & N64_C_LEFT_MASK)    gamepad->joysticks.rx = INT16_MIN;
    if (n64_data->buttons & N64_C_RIGHT_MASK)   gamepad->joysticks.rx = INT16_MAX;

    if (n64_data->buttons & N64_A_MASK)         gamepad->buttons.a = true;
    if (n64_data->buttons & N64_B_MASK)         gamepad->buttons.b = true;
    if (n64_data->buttons & N64_START_MASK)     gamepad->buttons.start = true;
    if (n64_data->buttons & N64_L_MASK)         gamepad->buttons.lb = true;
    if (n64_data->buttons & N64_R_MASK)         gamepad->buttons.rb = true;

    if (n64_data->buttons & N64_Z_MASK)         gamepad->triggers.r = 0xFF;

    gamepad->joysticks.ly = scale_uint8_to_int16(n64_data->y, true);
    gamepad->joysticks.lx = scale_uint8_to_int16(n64_data->x, false);
}

void N64USB::process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void)len;

    static N64USBReport prev_report = {};

    N64USBReport n64_report;
    memcpy(&n64_report, report, sizeof(n64_report));

    if (memcmp(&n64_report, &prev_report, sizeof(n64_report)) != 0)
    {
        update_gamepad(gamepad, &n64_report);

        prev_report = n64_report;
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void N64USB::process_xinput_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) 
{
    (void)gamepad;
    (void)dev_addr;
    (void)instance;
    (void)report;
    (void)len;
}

void N64USB::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) 
{
    (void)dev_addr;
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)len;
}

bool N64USB::send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance)
{
    (void)gamepad;
    (void)dev_addr;
    (void)instance;

    return true;
}