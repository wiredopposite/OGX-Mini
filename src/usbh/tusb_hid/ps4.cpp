#include <stdint.h>

#include "pico/stdlib.h"

#include "tusb.h"

#include "tusb_hid/ps4.h"
#include "Gamepad.h"

void process_dualshock4(uint8_t const* report, uint16_t len)
{
    // previous report, compared for changes
    static sony_ds4_report_t prev_report = { 0 };

    // Increment pointer and decrement length to skip report ID
    report++;
    len--;

    sony_ds4_report_t ds4_report;
    memcpy(&ds4_report, report, sizeof(ds4_report));

    // Check if the new report is different from the previous one
    if (memcmp(&ds4_report, &prev_report, sizeof(ds4_report)) != 0)
    {
        gamepad.update_gamepad_state_from_dualshock4(&ds4_report);

        // Update the previous report
        prev_report = ds4_report;
    }
}

bool send_fb_data_to_dualshock4(uint8_t dev_addr, uint8_t instance)
{
    sony_ds4_output_report_t output_report = {0};
    output_report.set_rumble = 1;
    output_report.motor_left = gamepadOut.out_state.lrumble;
    output_report.motor_right = gamepadOut.out_state.rrumble;
    
    bool rumble_sent = tuh_hid_send_report(dev_addr, instance, 5, &output_report, sizeof(output_report));

    return rumble_sent;
}