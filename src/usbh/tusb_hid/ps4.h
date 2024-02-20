#pragma once

#ifndef _PS4_H_
#define _PS4_H_

#include <stdint.h>

#include "tusb_hid/hid_vid_pid.h"

const usb_vid_pid_t ps4_devices[] = 
{
    {0x054C, 0x05C4}, // DS4
    {0x054C, 0x09CC}, // DS4
    {0x0F0D, 0x005E}, // Hori FC4
    {0x0F0D, 0x00EE}, // Hori PS4 Mini (PS4-099U)
    {0x1F4F, 0x1002}  // ASW GG Xrd controller
};

enum dualshock4_dpad_mask
{
    PS4_DPAD_MASK_UP = 0x00,
    PS4_DPAD_MASK_UP_RIGHT = 0x01,
    PS4_DPAD_MASK_RIGHT = 0x02,
    PS4_DPAD_MASK_RIGHT_DOWN = 0x03,
    PS4_DPAD_MASK_DOWN = 0x04,
    PS4_DPAD_MASK_DOWN_LEFT = 0x05,
    PS4_DPAD_MASK_LEFT = 0x06,
    PS4_DPAD_MASK_LEFT_UP = 0x07,
    PS4_DPAD_MASK_NONE = 0x08,
};

typedef struct __attribute__((packed))
{
    uint8_t lx, ly, rx, ry; // joystick

    struct {
        uint8_t dpad     : 4;
        uint8_t square   : 1;
        uint8_t cross    : 1;
        uint8_t circle   : 1;
        uint8_t triangle : 1;
    };

    struct {
        uint8_t l1     : 1;
        uint8_t r1     : 1;
        uint8_t l2     : 1;
        uint8_t r2     : 1;
        uint8_t share  : 1;
        uint8_t option : 1;
        uint8_t l3     : 1;
        uint8_t r3     : 1;
    };

    struct {
        uint8_t ps      : 1; // playstation button
        uint8_t tpad    : 1; // track pad click
        uint8_t counter : 6; // +1 each report
    };

    uint8_t l2_trigger; // 0 released, 0xff fully pressed
    uint8_t r2_trigger; // as above

    // uint16_t timestamp;
    // uint8_t  battery;
    
    // int16_t gyro[3];  // x, y, z;
    // int16_t accel[3]; // x, y, z;

    // there is still more info

} sony_ds4_report_t;

typedef struct __attribute__((packed))
{
    // First 16 bits set what data is pertinent in this structure (1 = set; 0 = not set)
    uint8_t set_rumble : 1;
    uint8_t set_led : 1;
    uint8_t set_led_blink : 1;
    uint8_t set_ext_write : 1;
    uint8_t set_left_volume : 1;
    uint8_t set_right_volume : 1;
    uint8_t set_mic_volume : 1;
    uint8_t set_speaker_volume : 1;
    uint8_t set_flags2;

    uint8_t reserved;

    uint8_t motor_right;
    uint8_t motor_left;

    uint8_t lightbar_red;
    uint8_t lightbar_green;
    uint8_t lightbar_blue;
    uint8_t lightbar_blink_on;
    uint8_t lightbar_blink_off;

    uint8_t ext_data[8];

    uint8_t volume_left;
    uint8_t volume_right;
    uint8_t volume_mic;
    uint8_t volume_speaker;

    uint8_t other[9];
} sony_ds4_output_report_t;

// #ifdef __cplusplus
// extern "C" {
// #endif

bool send_fb_data_to_dualshock4(uint8_t dev_addr, uint8_t instance);
void process_dualshock4(uint8_t const* report, uint16_t len);

// #ifdef __cplusplus
// }
// #endif

#endif // _PS4_H_