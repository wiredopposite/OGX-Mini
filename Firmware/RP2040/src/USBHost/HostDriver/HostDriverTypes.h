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
    /** Debug probe: Flydigi APEX 4 Wukong / V1 composite (04B4:2412). Not production mapping. */
    FLYDIGI_APEX4_WUKONG,
    /** GameSir Cyclone 2: wired XInput + session-owned Switch NS / DS4. Debug builds. */
    GAMESIR_CYCLONE2,
    /** GameSir G7 Pro wired HID (3537:1022) — Xbox-order face bits, not DInput PS layout. */
    GAMESIR_G7_PRO,
    /** Victrix Gambit Tournament Controller — wired Xbox GIP (0E6F:0250 verified, 02D6 documented). */
    VICTRIX_GAMBIT,
    XBOXOG,
    XBOXONE,
    XBOX360W,
    XBOX360,
    XBOX360_CHATPAD,
    HID_GENERIC
};

#endif // HOST_DRIVER_TYPES_H