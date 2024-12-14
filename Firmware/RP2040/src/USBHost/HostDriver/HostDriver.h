#ifndef _HOST_DRIVER_H_
#define _HOST_DRIVER_H_

#include <cstdint>

#include "UserSettings/UserProfile.h"
#include "UserSettings/UserSettings.h"
#include "Gamepad.h"

//Use HostManager, don't use this directly
class HostDriver
{
public:
    enum class Type
    {
        UNKNOWN = 0,
        SWITCH_PRO,
        SWITCH,
        PSCLASSIC,
        DINPUT,
        PS3,
        PS4,
        PS5,
        N64,
        XBOXOG,
        XBOXONE,
        XBOX360W,
        XBOX360,
        XBOX360_CHATPAD,
        HID_GENERIC
    };

    HostDriver(uint8_t idx)
        : idx_(idx) {}

    virtual ~HostDriver() {};

    virtual void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, uint8_t const* report_desc, uint16_t desc_len) = 0;
    virtual void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) = 0;
    virtual bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) = 0;

protected:
    const uint8_t idx_;

    void manage_rumble(Gamepad& gamepad)
    {
        Gamepad::PadOut gp_out = gamepad.get_pad_out();
        bool reset = false;
        if (gp_out.rumble_l != UINT_8::MAX)
        {
            gp_out.rumble_l = 0;
            reset = true;
        }
        if (gp_out.rumble_r != UINT_8::MAX)
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