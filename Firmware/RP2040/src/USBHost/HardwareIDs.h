#ifndef _HW_ID_H_
#define _HW_ID_H_

#include <cstdint>

#include "USBHost/HostDriver/HostDriver.h"

struct HardwareID
{
    uint16_t vid;
    uint16_t pid;
};

static const HardwareID DINPUT_IDS[] =
{
    {0x044F, 0xB324}, // ThrustMaster Dual Trigger (PS3 mode)
    {0x0738, 0x8818}, // MadCatz Street Fighter IV Arcade FightStick
    {0x0810, 0x0003}, // Personal Communication Systems, Inc. Generic
    {0x146B, 0x0902}, // BigBen Interactive Wired Mini PS3 Game Controller
    {0x2563, 0x0575}, // SHANWAN 2In1 USB Joystick
    {0x046D, 0xC218} // Logitech RumblePad 2
};

static const HardwareID PS3_IDS[] =
{
    {0x054C, 0x0268}, // Sony Batoh (Dualshock 3)
};

static const HardwareID PS4_IDS[] =
{
    {0x054C, 0x05C4}, // DS4
    {0x054C, 0x09CC}, // DS4
    {0x054C, 0x0BA0}, // DS4 wireless adapter
    {0x2563, 0x0357}, // MPOW Wired Gamepad (ShenZhen ShanWan)
    {0x0F0D, 0x005E}, // Hori FC4
    {0x0F0D, 0x00EE}, // Hori PS4 Mini (PS4-099U)
    {0x1F4F, 0x1002}  // ASW GG Xrd controller
};

static const HardwareID PS5_IDS[] =
{
    {0x054C, 0x0CE6}, // dualsense
    {0x054C, 0x0DF2} // dualsense edge
};

static const HardwareID PSCLASSIC_IDS[] =
{
    {0x054C, 0x0CDA} // psclassic
};

static const HardwareID SWITCH_PRO_IDS[] =
{
    {0x057E, 0x2009}, // Switch Pro
    {0x057E, 0x2009}  // Mytrix Sakura Nintendo Switch Controller (Wired)
    // {0x20D6, 0xA711}, // OpenSteamController, emulated pro controller
};

static const HardwareID SWITCH_WIRED_IDS[] =
{
    {0x20D6, 0xA719}, // PowerA wired
    {0x20D6, 0xA713}, // PowerA Enhanced wired
    {0x0F0D, 0x0092},  // Hori Pokken wired, I don't have this one so not 100% on if it'll work
    {0x045E, 0x028E}  // DATA FROG Nintendo Switch Pro Controller (Wired)
    {0x0F0D, 0x0092}, // Hori Pokken Tournament Pro
    {0x0F0D, 0x00C1}, // Hori Pokken Horipad
};

static const HardwareID N64_IDS[] =
{
    {0x0079, 0x0006} // Retrolink N64 USB gamepad
};

struct HostTypeMap
{
    const HardwareID* ids;
    size_t num_ids;
    HostDriverType type;
};

static const HostTypeMap HOST_TYPE_MAP[] = 
{
    { DINPUT_IDS, sizeof(DINPUT_IDS) / sizeof(HardwareID), HostDriverType::DINPUT },
    { PS4_IDS, sizeof(PS4_IDS) / sizeof(HardwareID), HostDriverType::PS4 },
    { PS5_IDS, sizeof(PS5_IDS) / sizeof(HardwareID), HostDriverType::PS5 },
    { PS3_IDS, sizeof(PS3_IDS) / sizeof(HardwareID), HostDriverType::PS3 },
    { SWITCH_WIRED_IDS, sizeof(SWITCH_WIRED_IDS) / sizeof(HardwareID), HostDriverType::SWITCH },
    { SWITCH_PRO_IDS, sizeof(SWITCH_PRO_IDS) / sizeof(HardwareID), HostDriverType::SWITCH_PRO },
    { PSCLASSIC_IDS, sizeof(PSCLASSIC_IDS) / sizeof(HardwareID), HostDriverType::PSCLASSIC },
    { N64_IDS, sizeof(N64_IDS) / sizeof(HardwareID), HostDriverType::N64 },
};

#endif // _HW_ID_H_
