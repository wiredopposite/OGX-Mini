#ifndef _GPHOSTDRIVER_H_
#define _GPHOSTDRIVER_H_

#include <stdint.h>
#include "host/usbh.h" // needed so xinput_host will build
#include "xinput_host.h"
#include "tusb_gamepad.h"

#include "usbh/shared/shared.h"

class GPHostDriver 
{
    public:
        virtual ~GPHostDriver() = default;
        
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance) = 0;
        virtual void process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) = 0;
        virtual void process_xinput_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len) = 0;
        virtual void hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) = 0;
        virtual bool send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance) = 0;
};

#endif // _GPHOSTDRIVER_H_