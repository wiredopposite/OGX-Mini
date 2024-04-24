#ifndef _DUALSHOCK3_H_
#define _DUALSHOCK3_H_

#include <stdint.h>
#include "descriptors/PS3Descriptors.h"

#include "usbh/GPHostDriver.h"

const usb_vid_pid_t ps3_devices[] = 
{
    {0x054C, 0x0268}, // Sony Batoh (Dualshock 3)
};

struct Dualshock3State
{
    uint8_t player_id {0};
    bool reports_enabled {false};
    uint8_t en_buffer[17];
    Dualshock3OutReport out_report;
    int response_count {0};
};

class Dualshock3 : public GPHostDriver
{
    public:
        ~Dualshock3() override {}

        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual void hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len);
        virtual bool send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance);
    private:
        Dualshock3State dualshock3;
        void update_gamepad(Gamepad* gp, const Dualshock3Report* ds3_data);
        void get_report_complete_cb(uint8_t dev_addr, uint8_t instance);
};

#endif