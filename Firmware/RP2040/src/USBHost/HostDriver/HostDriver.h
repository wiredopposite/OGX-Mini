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
        if (gamepad.get_rumble_l().uint8() != UINT8_MAX)
        {
            gamepad.set_rumble_l(static_cast<uint8_t>(0));
        }
        if (gamepad.get_rumble_r().uint8() != UINT8_MAX)
        {
            gamepad.set_rumble_r(static_cast<uint8_t>(0));
        }
    }
};

#endif // _HOST_DRIVER_H_