#pragma once

#ifndef _DUALSHOCK3_H_
#define _DUALSHOCK3_H_

#include <stdint.h>

#include "usbh/GPHostDriver.h"
#include "usbd/descriptors/PS3Descriptors.h"
#include "usbd/descriptors/DInputDescriptors.h"

const usb_vid_pid_t ps3_devices[] = 
{
    {0x054C, 0x0268}, // Sony Batoh (Dualshock 3)
    // {0x045E, 0x028E}, // Voyee generic "P3", won't work, IDs are for a 360 controller (dumb)
    {0x044F, 0xB324}, // ThrustMaster Dual Trigger (PS3 mode)
    {0x0738, 0x8818}, // MadCatz Street Fighter IV Arcade FightStick
    {0x0810, 0x0003}, // Personal Communication Systems, Inc. Generic
    {0x146B, 0x0902}, // BigBen Interactive Wired Mini PS3 Game Controller
    {0x2563, 0x0575}  // SHANWAN 2In1 USB Joystick
};

struct Dualshock3State
{
    uint8_t player_id = {0};
    bool reports_enabled {false};
    bool sixaxis {true};
};

class Dualshock3 : public GPHostDriver
{
    public:
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual bool send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance);
    private:
        Dualshock3State dualshock3;
        void enable_reports(uint8_t dev_addr, uint8_t instance);
        void update_gamepad_from_ds3(Gamepad& gp, const Dualshock3Report* ds3_data);
        void update_gamepad_from_dinput(Gamepad& gp, const DInputReport* dinput_report);
};

#endif