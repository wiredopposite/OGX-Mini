#pragma once

#ifndef _MOUSE_H_
#define _MOUSE_H_

#include <stdint.h>

#include "class/hid/hid.h"
#include "usbh/tusb_hid/shared.h"

struct MouseState
{
    uint8_t dev_addr = {0};
    uint8_t instance = {0};
};

class Mouse
{
    public:
        void init(uint8_t dev_addr, uint8_t instance);
        void process_report(uint8_t const* report, uint16_t len);
        bool send_fb_data();
        
    private:
        MouseState mouse;

        int16_t scale_and_clamp_axes(int32_t value);
        void update_gamepad(const hid_mouse_report_t* mouse_report);
};

#endif // _MOUSE_H_