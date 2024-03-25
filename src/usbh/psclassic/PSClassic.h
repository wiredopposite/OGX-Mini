#pragma once

#ifndef _PSCLASSIC_H_
#define _PSCLASSIC_H_

#include <stdint.h>

#include "usbh/GPHostDriver.h"
#include "descriptors/PSClassicDescriptors.h"

const usb_vid_pid_t psc_devices[] = 
{
    {0x054C, 0x0CDA} // psclassic
};

struct PSClassicState 
{
    uint8_t player_id = {0};
};

class PSClassic : public GPHostDriver
{
    public:
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual bool send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance);
    private:
        PSClassicState psclassic;
        void update_gamepad(Gamepad& gp, const PSClassicReport* psc_data);
};

#endif // _PSCLASSIC_H_