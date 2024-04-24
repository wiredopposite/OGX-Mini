#pragma once

#ifndef _XINPUT_H_
#define _XINPUT_H_

#include <stdint.h>

#include "usbh/GPHostDriver.h"

struct XInputState
{
    uint8_t player_id = {0};
    bool report_received = {false};
    bool leds_set = {false};
};

class XInputHost : public GPHostDriver
{
    public:
        ~XInputHost() override {}
        
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual void hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len);
        virtual bool send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance);
    private:
        XInputState xinput;
        void set_leds(uint8_t dev_addr, uint8_t instance);
};

#endif