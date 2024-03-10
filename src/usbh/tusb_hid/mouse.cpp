#include <stdint.h>
#include "pico/stdlib.h"
#include "tusb.h"

#include "usbh/tusb_hid/mouse.h"

#include "utilities/scaling.h"

#include "Gamepad.h"

void Mouse::init(uint8_t dev_addr, uint8_t instance)
{
    mouse.dev_addr = dev_addr;
    mouse.instance = instance;
}

int16_t Mouse::scale_and_clamp_axes(int32_t value)
{
    // minimum % of int16 +/- joystick value allowed
    float minimum_percentage = 0.10;  // 5%
    int32_t scaled_value = 0;

    if (value > 0)
    {
        scaled_value = (INT16_MAX * minimum_percentage) + ((1 - minimum_percentage) * value);        
    }
    else if (value < 0)
    {
        scaled_value = (INT16_MIN * minimum_percentage) + ((1 - minimum_percentage) * value);
    }
    else 
    {
        return 0;
    }

    if (scaled_value >= INT16_MAX) return INT16_MAX;
    else if (scaled_value <= INT16_MIN) return INT16_MIN;

    return (int16_t)scaled_value;
}

void Mouse::update_gamepad(const hid_mouse_report_t* mouse_report) 
{
    // gamepad.reset_state();

    gamepad.state.rt = (mouse_report->buttons & MOUSE_BUTTON_LEFT ) ? 0xFF : 0x00;
    gamepad.state.lt = (mouse_report->buttons & MOUSE_BUTTON_RIGHT) ? 0xFF : 0x00;

    // for testing
    gamepad.state.x  = (mouse_report->buttons & MOUSE_BUTTON_BACKWARD);
    gamepad.state.a  = (mouse_report->buttons & MOUSE_BUTTON_FORWARD );

    gamepad.state.y  = (mouse_report->wheel != 0);

    int32_t scaled_y = scale_int8_to_int16(mouse_report->y, true) * 2; // * 2 to make more responsive
    int32_t scaled_x = scale_int8_to_int16(mouse_report->x, false) * 2;

    gamepad.state.ry = scale_and_clamp_axes(scaled_y);
    gamepad.state.rx = scale_and_clamp_axes(scaled_x);
}

void Mouse::process_report(uint8_t const* report, uint16_t len)
{
    static hid_mouse_report_t prev_report = { 0 };

    hid_mouse_report_t mouse_report;
    memcpy(&mouse_report, report, sizeof(mouse_report));

    if (memcmp(&mouse_report, &prev_report, sizeof(mouse_report)) != 0)
        update_gamepad(&mouse_report);
}

bool Mouse::send_fb_data()
{
    return true;
}