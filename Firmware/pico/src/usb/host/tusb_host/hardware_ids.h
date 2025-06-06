#pragma once

#include <stdint.h>
#include <stddef.h>
#include "usb/host/host.h"

typedef struct {
    uint16_t vid;
    uint16_t pid;
} hardware_id_t;

static const hardware_id_t HW_IDS_DINPUT[] = {
    {0x044F, 0xB324}, // ThrustMaster Dual Trigger (PS3 mode)
    {0x0738, 0x8818}, // MadCatz Street Fighter IV Arcade FightStick
    {0x0810, 0x0003}, // Personal Communication Systems, Inc. Generic
    {0x146B, 0x0902}, // BigBen Interactive Wired Mini PS3 Game Controller
    {0x2563, 0x0575}, // SHANWAN 2In1 USB Joystick
    {0x046D, 0xC218} // Logitech RumblePad 2
};

static const hardware_id_t HW_IDS_PS3[] = {
    {0x054C, 0x0268}, // Sony Batoh (Dualshock 3)
};

static const hardware_id_t HW_IDS_PS4[] = {
    {0x054C, 0x05C4}, // DS4
    {0x054C, 0x09CC}, // DS4
    {0x054C, 0x0BA0}, // DS4 wireless adapter
    {0x2563, 0x0357}, // MPOW Wired Gamepad (ShenZhen ShanWan)
    {0x0F0D, 0x005E}, // Hori FC4
    {0x0F0D, 0x00EE}, // Hori PS4 Mini (PS4-099U)
    {0x1F4F, 0x1002}  // ASW GG Xrd controller
};

static const hardware_id_t HW_IDS_PS5[] = {
    {0x054C, 0x0CE6}, // dualsense
    {0x054C, 0x0DF2}  // dualsense edge
};

static const hardware_id_t HW_IDS_PSCLASSIC[] = {
    {0x054C, 0x0CDA} // psclassic
};

static const hardware_id_t HW_IDS_SWITCH_PRO[] = {
    {0x057E, 0x2009}, // Switch Pro
    // {0x20D6, 0xA711}, // OpenSteamController, emulated pro controller
};

static const hardware_id_t HW_IDS_SWITCH[] = {
    {0x20D6, 0xA719}, // PowerA wired
    {0x20D6, 0xA713}, // PowerA Enhanced wired
    {0x0F0D, 0x0092}, // Hori Pokken Tournament Pro
    {0x0F0D, 0x00C1}, // Hori Pokken Horipad
};

static const hardware_id_t HW_IDS_N64[] = {
    {0x0079, 0x0006} // Retrolink N64 USB gamepad
};

typedef struct {
    const hardware_id_t* ids;
    size_t num_ids;
    usbh_type_t type;
} usbh_type_map_t;

static const usbh_type_map_t HW_IDS_MAP[] = {
    { HW_IDS_DINPUT,        sizeof(HW_IDS_DINPUT) / sizeof(hardware_id_t),      USBH_TYPE_HID_DINPUT },
    { HW_IDS_PS4,           sizeof(HW_IDS_PS4) / sizeof(hardware_id_t),         USBH_TYPE_HID_PS4 },
    { HW_IDS_PS5,           sizeof(HW_IDS_PS5) / sizeof(hardware_id_t),         USBH_TYPE_HID_PS5 },
    { HW_IDS_PS3,           sizeof(HW_IDS_PS3) / sizeof(hardware_id_t),         USBH_TYPE_HID_PS3 },
    { HW_IDS_SWITCH,        sizeof(HW_IDS_SWITCH) / sizeof(hardware_id_t),      USBH_TYPE_HID_SWITCH },
    { HW_IDS_SWITCH_PRO,    sizeof(HW_IDS_SWITCH_PRO) / sizeof(hardware_id_t),  USBH_TYPE_HID_SWITCH_PRO },
    { HW_IDS_PSCLASSIC,     sizeof(HW_IDS_PSCLASSIC) / sizeof(hardware_id_t),   USBH_TYPE_HID_PSCLASSIC },
    { HW_IDS_N64,           sizeof(HW_IDS_N64) / sizeof(hardware_id_t),         USBH_TYPE_HID_N64 },
};