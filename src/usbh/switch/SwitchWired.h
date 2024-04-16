#pragma once

#ifndef _SWITCHWIRED_H_
#define _SWITCHWIRED_H_

#include <stdint.h>

#include "usbh/GPHostDriver.h"

const usb_vid_pid_t switch_wired_devices[] = 
{
    {0x20D6, 0xA719}, // PowerA wired
    {0x0F0D, 0x0092}  // Hori Pokken wired, I don't have this one so not 100% on if it'll work
};

struct SwitchWiredReport
{
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t x : 1;   

    uint8_t l : 1;
    uint8_t r : 1;
    uint8_t lz : 1;
    uint8_t rz : 1;

    uint8_t minus : 1;
    uint8_t plus : 1;
    uint8_t l3 : 1;
    uint8_t r3 : 1;

    uint8_t home : 1;
    uint8_t capture : 1;

    uint8_t : 2;

    uint8_t dpad : 4;
    uint8_t : 4;

    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
}
__attribute__((packed));

struct SwitchWiredState 
{
    uint8_t player_id = {0};
};

class SwitchWired : public GPHostDriver
{
    public:
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual bool send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance);
    private:
        SwitchWiredState switch_wired;
        void update_gamepad(Gamepad& gp, const SwitchWiredReport* switch_pro_data);
};

#endif // _SWITCHWIRED_H_