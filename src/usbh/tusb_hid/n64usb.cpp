#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/tusb_hid/n64usb.h"

#include "Gamepad.h"

void N64USB::init(uint8_t dev_addr, uint8_t instance)
{
    n64usb.dev_addr = dev_addr;
    n64usb.instance = instance;
}

void N64USB::update_gamepad(const N64USBReport* n64_data)
{
    gamepad.reset_state();

    uint8_t n64_dpad = n64_data->buttons & N64_DPAD_MASK;

    switch(n64_dpad)
    {
        case N64_DPAD_MASK_UP:
            gamepad.state.up = true;
            break;
        case N64_DPAD_MASK_UP_RIGHT:
            gamepad.state.up = true;
            gamepad.state.right = true;
            break;
        case N64_DPAD_MASK_RIGHT:
            gamepad.state.right = true;
            break;
        case N64_DPAD_MASK_RIGHT_DOWN:
            gamepad.state.right = true;
            gamepad.state.down = true;
            break;
        case N64_DPAD_MASK_DOWN:
            gamepad.state.down = true;
            break;
        case N64_DPAD_MASK_DOWN_LEFT:
            gamepad.state.down = true;
            gamepad.state.left = true;
            break;
        case N64_DPAD_MASK_LEFT:
            gamepad.state.left = true;
            break;
        case N64_DPAD_MASK_LEFT_UP:
            gamepad.state.left = true;
            gamepad.state.up = true;
            break;
    }

    if (n64_data->buttons & N64_C_UP_MASK)      gamepad.state.ry = INT16_MAX;
    if (n64_data->buttons & N64_C_DOWN_MASK)    gamepad.state.ry = INT16_MIN;
    if (n64_data->buttons & N64_C_LEFT_MASK)    gamepad.state.rx = INT16_MIN;
    if (n64_data->buttons & N64_C_RIGHT_MASK)   gamepad.state.rx = INT16_MAX;

    if (n64_data->buttons & N64_A_MASK)         gamepad.state.a = true;
    if (n64_data->buttons & N64_B_MASK)         gamepad.state.b = true;
    if (n64_data->buttons & N64_START_MASK)     gamepad.state.start = true;
    if (n64_data->buttons & N64_L_MASK)         gamepad.state.lb = true;
    if (n64_data->buttons & N64_R_MASK)         gamepad.state.rb = true;

    if (n64_data->buttons & N64_Z_MASK)         gamepad.state.rt = 0xFF;

    int32_t new_value_ly = -(static_cast<int32_t>(n64_data->y - 127) << 8);
    int32_t new_value_lx = static_cast<int32_t>(n64_data->x - 127) << 8;

    if (new_value_lx > INT16_MAX) 
    {
        new_value_lx = INT16_MAX;
    }
    else if (new_value_lx < INT16_MIN)
    {
        new_value_lx = INT16_MIN;
    }
    
    if (new_value_ly > INT16_MAX) 
    {
        new_value_ly = INT16_MAX;
    }
    else if (new_value_ly < INT16_MIN)
    {
        new_value_ly = INT16_MIN;
    }

    gamepad.state.ly = (int16_t)new_value_ly;
    gamepad.state.lx = (int16_t)new_value_lx;
}

void N64USB::process_report(uint8_t const* report, uint16_t len)
{
    static N64USBReport prev_report = { 0 };

    N64USBReport n64_report;
    memcpy(&n64_report, report, sizeof(n64_report));

    if (memcmp(&n64_report, &prev_report, sizeof(n64_report)) != 0)
    {
        update_gamepad(&n64_report);

        prev_report = n64_report;
    }
}

bool N64USB::send_fb_data()
{
    return true;
}