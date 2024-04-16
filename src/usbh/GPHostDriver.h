#pragma once

#ifndef _GPHOSTDRIVER_H_
#define _GPHOSTDRIVER_H_

#include <stdint.h>
#include "host/usbh.h" // needed so xinput_host will build

#include "usbh/shared/shared.h"
#include "usbh/xinput/driver/xinput_host.h"
#include "Gamepad.h"

class GPHostDriver 
{
    public:
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance) = 0;
        virtual void process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) = 0;
        virtual void process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) = 0;
        virtual bool send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance) = 0;
};

#endif