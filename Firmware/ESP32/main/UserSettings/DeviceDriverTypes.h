#ifndef _DEVICE_DRIVER_TYPES_H_
#define _DEVICE_DRIVER_TYPES_H_

#include <cstdint>

enum class DeviceDriverType : uint8_t
{
    NONE = 0,
    XBOXOG,
    XBOXOG_SB,
    XBOXOG_XR,
    XINPUT,
    PS3,
    DINPUT,
    PSCLASSIC,
    SWITCH,
    WEBAPP = 100,
    UART_BRIDGE
};

static inline std::string DRIVER_NAME(DeviceDriverType driver)
{
    switch (driver)
    {
        case DeviceDriverType::XBOXOG: return "XBOX OG";
        case DeviceDriverType::XBOXOG_SB: return "XBOX OG SB";
        case DeviceDriverType::XBOXOG_XR: return "XBOX OG XR";
        case DeviceDriverType::XINPUT: return "XINPUT";
        case DeviceDriverType::PS3: return "PS3";
        case DeviceDriverType::DINPUT: return "DINPUT";
        case DeviceDriverType::PSCLASSIC: return "PS CLASSIC";
        case DeviceDriverType::SWITCH: return "SWITCH";
        case DeviceDriverType::WEBAPP: return "WEBAPP";
        case DeviceDriverType::UART_BRIDGE: return "UART BRIDGE";
        default: return "UNKNOWN";
    }
}

#endif // _DEVICE_DRIVER_TYPES_H_