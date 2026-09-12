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
    WIIU,
    WII,   // Wiimote (Pico W/2W: controller on PIO USB, BT for Wii link)
    PS1PS2,   // PS1/PS2 controller over GPIO (no USB device; output to console port)
    GAMECUBE, // GameCube controller over GPIO (single wire; no USB device)
    DREAMCAST, // Dreamcast controller over Maple Bus GPIO (no USB device; output/input to console)
    N64,       // N64 controller over GPIO (single wire; no USB device; output to N64 console)
    PS4,       // DualShock 4 USB HID (CUH-ZCT1x-class descriptor)
    STEAM,     // SteamOS/Bazzite: DualSense USB gamepad + HID mouse (touchpad passthrough)
    WEBAPP = 100,
    UART_BRIDGE
};

#endif // _DEVICE_DRIVER_TYPES_H_