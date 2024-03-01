#pragma once

#ifndef _SWITCH_PRO_H_
#define _SWITCH_PRO_H_

#include <stdint.h>

#include "usbh/tusb_hid/shared.h"

const usb_vid_pid_t switch_pro_devices[] = 
{
    {0x057E, 0x2009} // Switch Pro
};

struct SwitchProReport
{
    uint8_t reportId;
    uint8_t timer;

    uint8_t connInfo : 4;
    uint8_t battery  : 4;

    uint8_t y  : 1;
    uint8_t x  : 1;
    uint8_t b  : 1;
    uint8_t a  : 1;
    uint8_t    : 2;
    uint8_t r  : 1;
    uint8_t zr : 1;

    uint8_t minus   : 1;
    uint8_t plus    : 1;
    uint8_t stickR  : 1;
    uint8_t stickL  : 1;
    uint8_t home    : 1;
    uint8_t capture : 1;
    uint8_t         : 0;

    uint8_t down  : 1;
    uint8_t up    : 1;
    uint8_t right : 1;
    uint8_t left  : 1;
    uint8_t       : 2;
    uint8_t l     : 1;
    uint8_t zl    : 1;

    uint16_t leftX  : 12;
    uint16_t leftY  : 12;
    uint16_t rightX : 12;
    uint16_t rightY : 12;

    uint8_t vibrator;

    uint16_t accelerX;
    uint16_t accelerY;
    uint16_t accelerZ;

    uint16_t velocityX;
    uint16_t velocityY;
    uint16_t velocityZ;
}
__attribute__((packed));

struct SwitchProState
{
    bool handshake_sent {false};
    bool timeout_disabled {false};
    bool full_report_enabled {false};
    bool led_set {false};
    bool led_home_set {false};
    bool imu_enabled {false};
    bool commands_sent {false};
    uint8_t output_sequence_counter {0};
    uint8_t dev_addr = {0};
    uint8_t instance = {0};
};

class SwitchPro
{
    public:
        void init(uint8_t dev_addr, uint8_t instance);
        void process_report(uint8_t const* report, uint16_t len);
        bool send_fb_data();
        
    private:
        SwitchProState switch_pro;

        bool disable_timeout();
        void reset_state();
        uint8_t get_output_sequence_counter();
        int16_t normalize_axes(uint16_t value);
        void update_gamepad(const SwitchProReport* switch_pro_data);
};

#endif // _SWITCH_PRO_H_