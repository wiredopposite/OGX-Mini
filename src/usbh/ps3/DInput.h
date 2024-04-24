#ifndef _DINPUT_H_
#define _DINPUT_H_

#include <stdint.h>
#include "descriptors/DInputDescriptors.h"

#include "usbh/GPHostDriver.h"

const usb_vid_pid_t dinput_devices[] = 
{
    {0x044F, 0xB324}, // ThrustMaster Dual Trigger (PS3 mode)
    {0x0738, 0x8818}, // MadCatz Street Fighter IV Arcade FightStick
    {0x0810, 0x0003}, // Personal Communication Systems, Inc. Generic
    {0x146B, 0x0902}, // BigBen Interactive Wired Mini PS3 Game Controller
    {0x2563, 0x0575}  // SHANWAN 2In1 USB Joystick
};

struct DInputState
{
    uint8_t player_id = {0};
};

class DInput : public GPHostDriver
{
    public:
        ~DInput() override {}

        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad* gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual void hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len);
        virtual bool send_fb_data(const Gamepad* gamepad, uint8_t dev_addr, uint8_t instance);
    private:
        DInputState dinput;
        void update_gamepad(Gamepad* gp, const DInputReport* dinput_report);
};

#endif // _DINPUT_H_