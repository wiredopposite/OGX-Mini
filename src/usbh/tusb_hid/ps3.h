#pragma once

#ifndef _PS3_H_
#define _PS3_H_

#include <stdint.h>

#include "usbh/tusb_hid/shared.h"
#include "descriptors/DInputDescriptors.h"
#include "descriptors/PS3Descriptors.h"

#define PS3_REPORT_BUFFER_SIZE 48

const usb_vid_pid_t ps3_devices[] = 
{
    {0x054C, 0x0268}, // Sony Batoh (Dualshock 3)
    {0x2563, 0x0575}, // Nuplay Armor 3
    // {0x045E, 0x028E}, // Voyee generic "P3", won't work, IDs are for a 360 controller (dumb)
    {0x044F, 0xB324}, // ThrustMaster Dual Trigger (PS3 mode)
    {0x0738, 0x8818}, // MadCatz Street Fighter IV Arcade FightStick
    {0x0810, 0x0003}, // Personal Communication Systems, Inc. Generic
    {0x146B, 0x0902} // BigBen Interactive Wired Mini PS3 Game Controller
};

struct Dualshock3State
{
    bool report_enabled {false};
    bool sixaxis {false};
    uint8_t dev_addr = {0};
    uint8_t instance = {0};
};

class Dualshock3
{
    public:
        void init(uint8_t dev_addr, uint8_t instance);
        void process_report(uint8_t const* report, uint16_t len);
        bool send_fb_data();
        
    private:
        Dualshock3State dualshock3;

        bool enable_reports();
        // void reset_state();
        void update_gamepad(const DualShock3Report* ds3_data);
        void update_gamepad_dinput(const DInputReport* dinput_report);
};

#endif // _PS3_H_