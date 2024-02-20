#include <stdint.h>

#include "pico/stdlib.h"

#include "tusb.h"

#include "tusb_hid/ps5.h"
#include "Gamepad.h"

void process_dualsense(uint8_t const* report, uint16_t len)
{
    // previous report, compared for changes
    static dualsense_input_report prev_report = { 0 };

    // Increment pointer and decrement length to skip report ID
    report++;
    len--;

    dualsense_input_report ds5_report;
    memcpy(&ds5_report, report, sizeof(ds5_report));

    // Check if the new report is different from the previous one
    if (memcmp(&ds5_report, &prev_report, sizeof(ds5_report)) != 0)
    {
        gamepad.update_gamepad_state_from_dualsense(&ds5_report);

        prev_report = ds5_report;
    }
}

bool send_fb_data_to_dualsense(uint8_t dev_addr, uint8_t instance)
{
    // need to figure out if the flags are necessary and how the LEDs work
    dualsense_output_report_t output_report = {0};
    output_report.valid_flag0 = 0x02; // idk what this means
    output_report.valid_flag1 = 0x02; // this one either
    output_report.valid_flag2 = 0x04; // uhhhhh
    output_report.motor_left = gamepadOut.out_state.lrumble;
    output_report.motor_right = gamepadOut.out_state.rrumble;

    bool rumble_sent = tuh_hid_send_report(dev_addr, instance, 5, &output_report, sizeof(output_report));

    return rumble_sent;
}