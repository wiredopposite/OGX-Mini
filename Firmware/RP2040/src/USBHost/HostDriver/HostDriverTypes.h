#ifndef HOST_DRIVER_TYPES_H
#define HOST_DRIVER_TYPES_H

#include <cstdint>

enum class HostDriverType
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

#endif // HOST_DRIVER_TYPES_H