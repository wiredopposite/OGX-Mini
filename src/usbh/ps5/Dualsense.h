#ifndef _DUALSENSE_H_
#define _DUALSENSE_H_

#include <stdint.h>

#include "usbh/GPHostDriver.h"

const usb_vid_pid_t ps5_devices[] = 
{
    {0x054C, 0x0CE6}, // dualsense
    {0x054C, 0x0DF2} // dualsense edge
};

enum dualsense_dpad_mask
{
    PS5_MASK_DPAD_UP = 0x00,
    PS5_MASK_DPAD_UP_RIGHT = 0x01,
    PS5_MASK_DPAD_RIGHT = 0x02,
    PS5_MASK_DPAD_RIGHT_DOWN = 0x03,
    PS5_MASK_DPAD_DOWN = 0x04,
    PS5_MASK_DPAD_DOWN_LEFT = 0x05,
    PS5_MASK_DPAD_LEFT = 0x06,
    PS5_MASK_DPAD_LEFT_UP = 0x07,
    PS5_MASK_DPAD_NONE = 0x08,
};

// enum dualsense_action_mask
// {
//     PS5_MASK_SQUARE = 0x10,
//     PS5_MASK_CROSS = 0x20,
//     PS5_MASK_CIRCLE = 0x40,
//     PS5_MASK_TRIANGLE = 0x80,
// };

enum dualsense_buttons0_mask
{
    PS5_MASK_L1 = 0x01,
    PS5_MASK_R1 = 0x02,
    PS5_MASK_L2_DIGITAL = 0x04,
    PS5_MASK_R2_DIGITAL = 0x08,
    PS5_MASK_SHARE = 0x10,
    PS5_MASK_OPTIONS = 0x20,
    PS5_MASK_L3 = 0x40,
    PS5_MASK_R3 = 0x80,
};

enum dualsense_buttons1_mask
{
    PS5_MASK_PS = 0x01,
    PS5_MASK_TOUCH_PAD = 0x02,
    PS5_MASK_MIC = 0x04,
};

struct DualsenseReport {
    uint8_t report_id;

    uint8_t lx, ly; 
    uint8_t rx, ry; 
    uint8_t lt, rt;
    uint8_t seq_number;

    uint8_t dpad : 4;
    uint8_t square : 1;
    uint8_t cross : 1;
    uint8_t circle : 1;
    uint8_t triangle : 1;

    uint8_t buttons[3];

    // Motion sensors
    uint16_t gyro[3]; // Gyroscope data for x, y, z axes
    uint16_t accel[3]; // Accelerometer data for x, y, z axes
    uint32_t sensor_timestamp; // Timestamp for sensor data
    uint8_t reserved2;

    // Touchpad
    struct dualsense_touch_point {
        uint8_t counter : 7; // Incremented every time a finger touches the touchpad
        uint8_t touching : 1; // Indicates if a finger is currently touching the touchpad
        uint16_t x : 12; // X coordinate of the touchpoint
        uint16_t y : 12; // Y coordinate of the touchpoint
    } points[2]; // Array of touchpoints (up to 2)

    uint8_t reserved3[12];

    uint8_t status; // ?

    uint8_t reserved4[10];
} __attribute__((packed));

struct DualsenseOutReport {
    uint8_t valid_flag0;
    uint8_t valid_flag1;

    /* For DualShock 4 compatibility mode. */
    uint8_t motor_right;
    uint8_t motor_left;

    /* Audio controls */
    uint8_t headphone_audio_volume; /* 0-0x7f */
    uint8_t speaker_audio_volume;   /* 0-255 */
    uint8_t internal_microphone_volume; /* 0-0x40 */
    uint8_t audio_flags;
    uint8_t mute_button_led;

    uint8_t power_save_control;

    /* right trigger motor */
    uint8_t right_trigger_motor_mode;
    uint8_t right_trigger_param[10];

    /* right trigger motor */
    uint8_t left_trigger_motor_mode;
    uint8_t left_trigger_param[10];

    uint8_t reserved2[4];

    uint8_t reduce_motor_power;
    uint8_t audio_flags2; /* 3 first bits: speaker pre-gain */

    /* LEDs and lightbar */
    uint8_t valid_flag2;
    uint8_t reserved3[2];
    uint8_t lightbar_setup;
    uint8_t led_brightness;
    uint8_t player_leds;
    uint8_t lightbar_red;
    uint8_t lightbar_green;
    uint8_t lightbar_blue;
} __attribute__((packed));

struct DualsenseState 
{
    uint8_t player_id = {0};
};

class Dualsense : public GPHostDriver 
{
    public:
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual bool send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance);
    private:
        DualsenseState dualsense;
        void update_gamepad(Gamepad& gp, const DualsenseReport* ds_report);
};

#endif // _DUALSENSE_H_