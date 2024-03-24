#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "utilities/scaling.h"
#include "usbh/n64usb/N64USB.h"

void N64USB::init(uint8_t dev_addr, uint8_t instance)
{
    tuh_hid_receive_report(dev_addr, instance);
}

void N64USB::update_gamepad(Gamepad& gp, const N64USBReport* n64_data)
{
    gp.reset_state();

    uint8_t n64_dpad = n64_data->buttons & N64_DPAD_MASK;

    switch(n64_dpad)
    {
        case N64_DPAD_MASK_UP:
            gp.state.up = true;
            break;
        case N64_DPAD_MASK_UP_RIGHT:
            gp.state.up = true;
            gp.state.right = true;
            break;
        case N64_DPAD_MASK_RIGHT:
            gp.state.right = true;
            break;
        case N64_DPAD_MASK_RIGHT_DOWN:
            gp.state.right = true;
            gp.state.down = true;
            break;
        case N64_DPAD_MASK_DOWN:
            gp.state.down = true;
            break;
        case N64_DPAD_MASK_DOWN_LEFT:
            gp.state.down = true;
            gp.state.left = true;
            break;
        case N64_DPAD_MASK_LEFT:
            gp.state.left = true;
            break;
        case N64_DPAD_MASK_LEFT_UP:
            gp.state.left = true;
            gp.state.up = true;
            break;
    }

    if (n64_data->buttons & N64_C_UP_MASK)      gp.state.ry = INT16_MAX;
    if (n64_data->buttons & N64_C_DOWN_MASK)    gp.state.ry = INT16_MIN;
    if (n64_data->buttons & N64_C_LEFT_MASK)    gp.state.rx = INT16_MIN;
    if (n64_data->buttons & N64_C_RIGHT_MASK)   gp.state.rx = INT16_MAX;

    if (n64_data->buttons & N64_A_MASK)         gp.state.a = true;
    if (n64_data->buttons & N64_B_MASK)         gp.state.b = true;
    if (n64_data->buttons & N64_START_MASK)     gp.state.start = true;
    if (n64_data->buttons & N64_L_MASK)         gp.state.lb = true;
    if (n64_data->buttons & N64_R_MASK)         gp.state.rb = true;

    if (n64_data->buttons & N64_Z_MASK)         gp.state.rt = 0xFF;

    gp.state.ly = scale_uint8_to_int16(n64_data->y, true);
    gp.state.lx = scale_uint8_to_int16(n64_data->x, false);
}

void N64USB::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    static N64USBReport prev_report = { 0 };

    N64USBReport n64_report;
    memcpy(&n64_report, report, sizeof(n64_report));

    if (memcmp(&n64_report, &prev_report, sizeof(n64_report)) != 0)
    {
        update_gamepad(gp, &n64_report);

        prev_report = n64_report;
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void N64USB::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) {}

bool N64USB::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
{
    return true;
}