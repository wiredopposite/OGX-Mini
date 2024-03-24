#pragma once

#ifndef _XINPUT_H_
#define _XINPUT_H_

#include <stdint.h>

#include "usbh/GPHostDriver.h"

struct XInputState
{
    bool report_received = {false};
    bool leds_set = {false};
};

class XInputHost : public GPHostDriver
{
    public:
        virtual void init(uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual bool send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance);
    private:
        XInputState xinput;
        void set_leds(uint8_t dev_addr, uint8_t instance);
};

#endif