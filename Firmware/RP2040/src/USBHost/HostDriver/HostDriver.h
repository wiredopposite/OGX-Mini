#ifndef _HOST_DRIVER_H_
#define _HOST_DRIVER_H_

#include <cstdint>

#include "UserSettings/UserProfile.h"
#include "UserSettings/UserSettings.h"
#include "Gamepad/Gamepad.h"
#include "USBHost/HostDriver/HostDriverTypes.h"

//Use HostManager, don't use this directly
class HostDriver
{
public:
    HostDriver(uint8_t idx)
        : idx_(idx) {}

    virtual ~HostDriver() {};

    virtual void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, uint8_t const* report_desc, uint16_t desc_len) = 0;
    virtual void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) = 0;
    virtual bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) = 0;

    virtual void connect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) {}; //Wireless specific
    virtual void disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance) {}; //Wireless specific

protected:
    const uint8_t idx_;

    void manage_rumble(Gamepad& gamepad)
    {
        Gamepad::PadOut gp_out = gamepad.get_pad_out();
        bool reset = false;
        if (gp_out.rumble_l != Range::MAX<uint8_t>)
        {
            gp_out.rumble_l = 0;
            reset = true;
        }
        if (gp_out.rumble_r != Range::MAX<uint8_t>)
        {
            gp_out.rumble_r = 0;
            reset = true;
        }
        if (reset)
        {
            gamepad.set_pad_out(gp_out);
        }
    }
};

#endif // _HOST_DRIVER_H_