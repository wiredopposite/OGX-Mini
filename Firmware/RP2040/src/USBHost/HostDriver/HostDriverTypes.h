#ifndef HOST_DRIVER_TYPES_H
#define HOST_DRIVER_TYPES_H

#include <cstdint>

enum class HostDriverType
{
    UNKNOWN = 0,
    SWITCH_PRO,
    /** Nintendo Switch 2 Pro (PID 0x2069): wired USB; Switch2ProHost maps full digital + sticks + ZL/ZR (see Wired_Controllers.md) */
    SWITCH_PRO_2,
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

#endif // HOST_DRIVER_TYPES_H