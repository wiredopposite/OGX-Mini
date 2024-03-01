#pragma once

#ifndef _PSCLASSIC_H_
#define _PSCLASSIC_H_

#include <stdint.h>

#include "usbh/tusb_hid/shared.h"

#include "descriptors/PSClassicDescriptors.h"

const usb_vid_pid_t psc_devices[] = 
{
    {0x054C, 0x0CDA} // psclassic
};

struct PSClassicState
{
    uint8_t dev_addr = {0};
    uint8_t instance = {0};
};

class PSClassic
{
    public:
        void init(uint8_t dev_addr, uint8_t instance);
        void process_report(uint8_t const* report, uint16_t len);
        bool send_fb_data();
    private:
        PSClassicState psclassic;
        void update_gamepad(const PSClassicReport* psc_data);
};

#endif // _PSCLASSIC_H_