#ifndef _HW_ID_H_
#define _HW_ID_H_

#include <cstdint>

#include "USBHost/HostDriver/HostDriver.h"

struct HardwareID
{
    uint16_t vid;
    uint16_t pid;
};

// DInput / generic HID; includes Xbox controllers when presented as HID (XInput uses class driver when possible)
static const HardwareID DINPUT_IDS[] =
{
    {0x044F, 0xB324}, // ThrustMaster Dual Trigger (PS3 mode)
    {0x0738, 0x8818}, // MadCatz Street Fighter IV Arcade FightStick
    {0x0810, 0x0003}, // Personal Communication Systems, Inc. Generic
    {0x146B, 0x0902}, // BigBen Interactive Wired Mini PS3 Game Controller
    {0x2563, 0x0575}, // SHANWAN 2In1 USB Joystick
    {0x046D, 0xC218}, // Logitech RumblePad 2
    // Microsoft Xbox (045E) not in DINPUT: driven only by XInput. Adding them here causes
    // double driver on Adafruit Feather (HID + XInput) and "vibrates but no input". See upstream OGX-Mini.
    {0x2DC8, 0x3016}, // 8BitDo wired XInput (HID fallback)
    {0x2DC8, 0x3106}, // 8BitDo wired XInput (HID fallback)
    {0x2DC8, 0x2101}, // 8BitDo Xbox One SN30 Pro
    {0x0E6F, 0x0131}, // Afterglow Xbox
    {0x0E6F, 0x0132}, // Afterglow Xbox
    {0x0E6F, 0x0134}, // Afterglow Xbox
    {0x0E6F, 0x1314}, // Afterglow Xbox
    {0x0E6F, 0x0113}, // Afterglow Xbox Controller
    {0x0E6F, 0x0213}, // Afterglow Xbox Controller
    {0x0E6F, 0x0413}, // Afterglow Xbox Controller
    {0x0E6F, 0x0139}, // Afterglow Xbox
    {0x12AB, 0x0301}, // Afterglow Xbox
    {0x1BAD, 0xF900}, // Afterglow Xbox
    {0x1BAD, 0x00FD}, // Xbox 360 Controller (PDP/hub)
    {0x1BAD, 0x01FD}, // Xbox 360 Controller
    {0x1BAD, 0x16F0}, // Xbox 360 Controller
    {0x1BAD, 0x028E}, // Xbox 360 Controller
    {0x0F0D, 0x0086}, // Hori Fighting Commander Xbox 360
    {0x0F0D, 0x00BA}, // Hori Fighting Commander Xbox 360
    {0x0738, 0x4718}, // Mad Catz Street Fighter 4 Xbox 360 FightStick
    {0x0738, 0x4716}, // Xbox 360 Controller
    {0x0738, 0x4520}, // Xbox 360 Controller
    {0x0738, 0x4426}, // Xbox 360 Controller
    {0x0738, 0x4726}, // Xbox 360 Controller
    {0x0738, 0xB726}, // Xbox 360 Controller
    {0x0738, 0x4736}, // Xbox 360 Controller
    {0x0738, 0x4728}, // Xbox 360 Fightpad
    {0x0738, 0x4516}, // Xbox Controller
    {0x0738, 0x4526}, // Xbox Controller
    {0x0738, 0x4536}, // Xbox Controller
    {0x0738, 0x4586}, // Xbox Controller
    // Microsoft SideWinder / legacy (VID 045E)
    {0x045E, 0x0028}, // Microsoft Dual Strike
    {0x045E, 0x0003}, // Microsoft SideWinder
    {0x045E, 0x0007}, // Microsoft SideWinder
    {0x045E, 0x000E}, // Microsoft SideWinder Freestyle Pro
    {0x045E, 0x0027}, // Microsoft SideWinder Plug and Play
    {0x120C, 0x8802}, // Nyko Air Flo Xbox
    {0x120C, 0x880A}, // Xbox Controller
    {0x120C, 0x8810}, // Xbox Controller
    {0x162E, 0xBEEF}, // Xbox 360 Controller (generic)
    {0x24C6, 0x5300}, // Xbox 360 Controller
    {0x24C6, 0xFAFD}, // Xbox 360 Controller
    {0x062A, 0x0020}, // Xbox Controller
    // GameSir (multi-mode HID fallback, from SDL_GameControllerDB)
    {0x05AC, 0x033D}, // GameSir G3
    {0x05AC, 0x055B}, // GameSir G3w
    {0x05AC, 0x022D}, // GameSir G4
    {0x05AC, 0x044D}, // GameSir G4 (alt)
    {0x05AC, 0x061A}, // GameSir T3 2.02
    {0x3537, 0x1022}, // GameSir G7 Pro
    {0x3537, 0x1004}, // GameSir T4 Kaleid
    {0x3537, 0x1094}, // GameSir Tegenaria Lite
    // 8BitDo multi-mode (from SDL_GameControllerDB, HID fallback)
    {0x2DC8, 0x3019}, // 8BitDo 64
    {0x2DC8, 0x5109}, // 8BitDo Dogbone
    {0x2DC8, 0x9000}, // 8BitDo FC30 Pro
    {0x2DC8, 0x5112}, // 8BitDo Lite 2
    {0x2DC8, 0x5111}, // 8BitDo Lite SE
    {0x2DC8, 0x286A}, // 8BitDo GameCube
    {0x2DC8, 0x6003}, // 8BitDo Pro 2
    {0x2DC8, 0x6006}, // 8BitDo Pro 2
    {0x2DC8, 0x6103}, // 8BitDo Pro 2
    {0x2DC8, 0x6009}, // 8BitDo Pro 3
    {0x2DC8, 0x3012}, // 8BitDo Ultimate
    {0x2DC8, 0x3011}, // 8BitDo Ultimate Wired
    {0x2DC8, 0x3013}, // 8BitDo Ultimate Wireless
    {0x2DC8, 0x3100}, // 8BitDo Adapter
    {0x2DC8, 0x3105}, // 8BitDo Adapter 2
    {0x2DC8, 0x3101}, // 8BitDo Receiver
    {0x2DC8, 0x3102}, // 8BitDo Receiver
    {0x2DC8, 0x3103}, // 8BitDo Receiver
    {0x2DC8, 0x3104}, // 8BitDo Receiver
    // Flydigi (multi-mode; DInput fallback when not in XInput)
    {0xEED1, 0x04B4}, // Flydigi Vader 4 Pro (DInput mode)
    // Fightsticks / arcade sticks (from SDL_GameControllerDB PC/Arcade + PC/PlayStation)
    {0x8000, 0x1002}, // 8BitDo F30
    {0x1235, 0xAB11}, // 8BitDo F30 Arcade Joystick
    {0x2DC8, 0x2810}, // 8BitDo F30 Arcade Joystick
    {0x2DC8, 0xAB11}, // 8BitDo F30 Arcade Joystick
    {0x1080, 0x0009}, // 8BitDo F30 Arcade Stick
    {0x2DC8, 0x3810}, // 8BitDo F30 Pro
    {0x1C5A, 0x0024}, // Capcom Home Arcade
    {0x1C5B, 0x0024}, // Capcom Home Arcade
    {0x1C5B, 0x0025}, // Capcom Home Arcade
    {0x14D8, 0xCD07}, // Cthulhu
    {0x14D8, 0xFACE}, // Cthulhu
    {0x14D8, 0x6208}, // HitBox Edition Cthulhu
    {0x0F0D, 0x0010}, // Hori Fightstick
    {0x0F0D, 0x0032}, // Hori Fightstick 3W
    {0x0F0D, 0x00C0}, // Hori Fightstick 4
    {0x0F0D, 0x000D}, // Hori Fightstick EX2
    {0x0F0D, 0x0137}, // Hori Fightstick Mini
    {0x0F0D, 0x0040}, // Hori Fightstick Mini 3
    {0x0F0D, 0x0021}, // Hori Fightstick V3
    {0x0F0D, 0x0027}, // Hori Fightstick V3
    {0x0F0D, 0x0011}, // Hori Real Arcade Pro 3
    {0x0F0D, 0x0026}, // Hori Real Arcade Pro 3P
    {0x0F0D, 0x004B}, // Hori Real Arcade Pro 3W
    // RAP4 PS3 (0x006A/0x006B) are in PS3_IDS. RAP4/RAP4 Kai PS4 (0x008A/0x008B) are in PS4_IDS.
    // RAP4 PC/XInput (0x008C) is unlisted so tuh_xinput or HIDGeneric can claim it.
    {0x0F0D, 0x006F}, // Hori Real Arcade Pro 4 VLX
    {0x0F0D, 0x0070}, // Hori Real Arcade Pro 4 VLX
    {0x0F0D, 0x003D}, // Hori Real Arcade Pro N3
    {0x0F0D, 0x00AE}, // Hori Real Arcade Pro N4
    {0x0F0D, 0x00AA}, // Hori Real Arcade Pro S
    {0x0F0D, 0x00D8}, // Hori Real Arcade Pro S
    {0x0F0D, 0x0022}, // Hori Real Arcade Pro V3
    {0x0F0D, 0x005B}, // Hori Real Arcade Pro V4
    {0x0F0D, 0x005C}, // Hori Real Arcade Pro V4
    {0x0F0D, 0x00AF}, // Hori Real Arcade Pro VHS
    {0x0F0D, 0x001B}, // Hori Real Arcade Pro VX
    {0x1BAD, 0xF502}, // Hori Real Arcade Pro VX
    {0x0F0D, 0x0025}, // Hori Fighting Commander 3
    {0x0F0D, 0x002D}, // Hori Fighting Commander 3 Pro
    {0x0F0D, 0x0084}, // Hori Fighting Commander 5
    {0x0F0D, 0x0162}, // Hori Fighting Commander Octa
    {0x0F0D, 0x0164}, // Hori Fighting Commander Octa
    {0x0738, 0xB738}, // Mad Catz Fightstick TE
    {0x0738, 0x3187}, // Mad Catz PlayStation Fightstick
    {0x0738, 0x4738}, // Street Fighter Fightstick TE
    {0x0925, 0x2801}, // Mayflash Arcade Stick
    {0x0079, 0x1830}, // Mayflash F300 Arcade Joystick
    {0x2F24, 0x0039}, // Mayflash F300 Elite Arcade Joystick
    {0x1BAD, 0xF03E}, // MLG Fightstick TE
    {0x1BAD, 0xF028}, // Fightpad (Xbox)
    {0x1BAD, 0xF02E}, // Fightpad (Xbox)
    {0x1BAD, 0xF038}, // Fightpad TE (Xbox)
    {0x1292, 0x4E47}, // NeoGeo X Arcade Stick
    {0x0F30, 0x1101}, // Qanba 2
    {0x0F30, 0x1102}, // Qanba 2P
    {0x0F30, 0x1100}, // Qanba Arcade Stick 1008
    {0x0F30, 0x1116}, // Qanba Arcade Stick 4018
    {0x0F30, 0x1112}, // Qanba Joystick
    {0x0F30, 0x1012}, // Qanba Joystick Plus
    {0x1A34, 0x0401}, // Qanba Joystick Q4RAF
    {0x2C22, 0x2500}, // Qanba Dragon Arcade Joystick
    {0x2C22, 0x2000}, // Qanba Drone Arcade Stick
    {0x2C22, 0x2302}, // Qanba Obsidian Arcade Stick PS3 (PC mode)
    {0x1DD8, 0x000E}, // iBuffalo AC02 Arcade Joystick
    {0x16C0, 0x05E1}, // XinMo Dual Arcade
    {0x3507, 0x0004}, // Zenaim Arcade Controller
    {0x1C1A, 0x0100}, // Datel Arcade Joystick
    {0x0E6F, 0x0185}, // PDP Fightpad Pro GameCube
    // PowerA Switch-mode pads are in SWITCH_WIRED_IDS (SwitchWiredHost).
    // PowerA Xbox/PC pads are intentionally *not* listed here: DInputHost uses a fixed
    // PS3-style report layout. On Sakura's minimal ID list these fall through to HIDGeneric
    // (descriptor parse) or tuh_xinput — both work. Forcing DINPUT breaks them.
    // Elecom (from SDL_GameControllerDB)
    {0x056E, 0x200A}, // Elecom DUX60 MMO
    {0x056E, 0x2003}, // Elecom U3613M
    {0x056E, 0x2005}, // Elecom P301U PlayStation Controller Adapter
    {0x056E, 0x2007}, // Elecom W01U
    {0x056E, 0x200E}, // Elecom U3912T
    {0x056E, 0x200F}, // Elecom U4013S
    {0x056E, 0x2010}, // Elecom U4113S
    {0x056E, 0x2013}, // Elecom U4113
    {0x05B8, 0x1004}, // Elecom Gamepad
    {0x05B8, 0x1006}, // Elecom Gamepad
    {0x1241, 0x5044}, // Elecom U1012
    {0x0925, 0x1802}, // Elecom PlayStation Adapter (Mayflash OEM)
    // Brook adapters (DInput; Brook Mars PS4 in PS4_IDS)
    {0x120C, 0x0C31}, // Brook Super Converter
    {0x120C, 0x0EF1}, // Brook PS2 Adapter
    // Mad Catz PC / generic (from SDL_GameControllerDB)
    {0x0738, 0x5262}, // Mad Catz Micro CTRLR
    {0x0738, 0x5263}, // Mad Catz CTRLR
    {0x0738, 0x5266}, // Mad Catz CTRLR
    {0x0738, 0x3282}, // Mad Catz PlayStation Brawlpad
    // Hori pads / misc (remaining from SDL_GameControllerDB)
    {0x0F0D, 0x000A}, // Hori DOA
    {0x0F0D, 0x0009}, // Hori Pad 3 Turbo
    {0x0F0D, 0x0013}, // Horipad 3W
    {0x0F0D, 0x0042}, // Horipad A
    {0x0F0D, 0x004D}, // Hori Pad A
    {0x0F0D, 0x0054}, // Hori Pad 3
    {0x0F0D, 0x0055}, // Horipad 4 FPS
    {0x0F0D, 0x0064}, // Horipad 3TP
    {0x0F0D, 0x0067}, // Horipad One
    {0x0F0D, 0x00A0}, // Hori Grip TAC4
    {0x0F0D, 0x009C}, // Hori TAC Pro
    {0x0F0D, 0x00C9}, // Hori Taiko Controller
    {0x0F0D, 0x0101}, // Hori Mini Hatsune Miku FT
    {0x0F0D, 0x0138}, // Hori PC Engine Mini Controller
    {0x0F0D, 0x0196}, // Horipad Steam
    {0x1BAD, 0xF501}, // Horipad EXT2
    // Sony PlayStation (adapters / DualShock 2; DS3/DS4/DS5 in dedicated arrays)
    {0x0E8F, 0x1009}, // Sony DualShock 2
    {0x05E3, 0x0596}, // Sony PlayStation Adapter
    {0x14FE, 0x232A}, // Sony PlayStation Adapter
    {0x6666, 0x0667}, // Sony PlayStation Adapter
    {0x04D9, 0x0F16}, // Sony PlayStation Controller Adapter
    // Steam / Valve (from SDL_GameControllerDB)
    {0x28DE, 0x11FC}, // Steam Virtual Gamepad
    {0x28DE, 0x11FF}, // Steam Virtual Gamepad
    {0x28DE, 0x1102}, // Steam Controller
    {0x28DE, 0x1105}, // Steam Controller
    {0x28DE, 0x1106}, // Steam Controller
    {0x28DE, 0x1142}, // Steam Controller
    {0x28DE, 0x1201}, // Steam Controller
    {0x28DE, 0x1205}, // Steam Deck
    // Scuf Envision (Scuf PS4 in PS4_IDS; Envision PC/HID)
    {0x2E95, 0x434B}, // Scuf Envision (Linux/HID)
    {0x2E95, 0x434D}, // Scuf Envision (Linux/HID)
    {0x2E95, 0x434E}, // Scuf Envision (Linux/HID)
    // Logitech (from SDL_GameControllerDB; one entry already above)
    {0x046D, 0xC209}, // Logitech WingMan
    {0x046D, 0xC20A}, // Logitech WingMan RumblePad
    {0x046D, 0xC20B}, // Logitech WingMan Action Pad
    {0x046D, 0xC21A}, // Logitech Precision
    {0x046D, 0xC21D}, // Logitech F310
    {0x046D, 0xC21E}, // Logitech F510
    {0x046D, 0xC21F}, // Logitech F710
    {0x046D, 0xC216}, // Logitech Dual Action
    {0x046D, 0xC211}, // Logitech Cordless Wingman
    {0x046D, 0xCAD1}, // Logitech ChillStream
    {0x046D, 0xCAD2}, // Logitech Cordless Precision
    // Razer (from SDL_GameControllerDB; PS4 Razer in PS4_IDS)
    {0x27F8, 0x0BBF}, // Razer Kishi
    {0x1532, 0x0300}, // Razer Hydra
    {0x1532, 0x0900}, // Razer Serval
    {0x1532, 0x1000}, // Razer Raiju
    {0x1532, 0x0705}, // Razer Raiju Mobile
    {0x1532, 0x0707}, // Razer Raiju Mobile
    {0x1532, 0x1007}, // Razer Raiju TE
    {0x1532, 0x100A}, // Razer Raiju TE
    {0x1532, 0x1004}, // Razer Raiju UE
    {0x1532, 0x1009}, // Razer Raiju UE
};

// PS3 controllers (from SDL_GameControllerDB + existing); many third-party pads use PS3 protocol
static const HardwareID PS3_IDS[] =
{
    {0x054C, 0x0268}, // Sony Dualshock 3 (Batoh)
    {0x7331, 0x0001}, // Sony DualShock 3 (alt/clone VID)
    {0x1532, 0x0402}, // Razer Panthera PS3
    {0x1A34, 0x0836}, // Afterglow PS3
    {0x0E6F, 0x6302}, // Afterglow PS3
    {0x0E6F, 0x0111}, // Afterglow PS3
    {0x0E6F, 0x0114}, // Afterglow PS3
    {0x0E6F, 0x0214}, // Afterglow PS3
    {0x0E6F, 0x0119}, // Afterglow PS3
    {0x0E6F, 0x011A}, // Afterglow PS3
    {0x0010, 0x0082}, // Akishop Customs PS360
    {0x0E6F, 0x0132}, // Battlefield 4 PS3
    {0x146B, 0x0055}, // Bigben PS3
    {0x146B, 0x0301}, // Bigben PS3
    {0x25F0, 0xC121}, // Gioteck PS3
    {0x25F0, 0xC131}, // Gioteck PS3
    {0x0F0D, 0x0049}, // Hatsune Miku / Hori PS3
    {0x0F0D, 0x0085}, // Hori Fighting Commander 2016 PS3
    {0x0F0D, 0x005F}, // Hori Fighting Commander 4 PS3
    {0x0F0D, 0x0051}, // Hori Fighting Commander PS3
    {0x0F0D, 0x0088}, // Hori Fighting Stick mini 4 PS3
    {0x0F0D, 0x006E}, // Horipad 4 PS3
    {0x0F0D, 0x006A}, // Hori Real Arcade Pro 4 PS3
    {0x0F0D, 0x006B}, // Hori Real Arcade Pro 4 PS3 (alt)
    {0x0E6F, 0x0124}, // Injustice Fightstick PS3
    {0x0079, 0x0002}, // King PS3
    {0x0738, 0x8838}, // Mad Catz Arcade Fightstick TE S+ PS3
    {0x0738, 0x3285}, // Mad Catz Arcade Fightstick TE S PS3
    {0x0738, 0x3250}, // Mad Catz Fightpad Pro PS3
    {0x0738, 0x3180}, // Mad Catz FightStick Alpha PS3
    {0x0738, 0x3384}, // Mad Catz Fightstick TE S PS3
    {0x0738, 0x3481}, // Mad Catz Fightstick TE2 PS3
    {0x0738, 0xA856}, // Mad Catz PS3
    {0x0738, 0x8818}, // Mad Catz SFIV Fightstick PS3
    {0x0738, 0x3480}, // Mad Catz TE2 PS3 Fightstick
    {0x0738, 0x8263}, // MLG PS3
    {0x20D6, 0x571D}, // Nyko Airflo PS3
    {0x20D6, 0x576D}, // OPP PS3
    {0x0E6F, 0x0109}, // PDP PS3 Versus Fighting
    {0x0662, 0x70D5}, // PowerA PS3 (SDL DB VID)
    {0x2606, 0x70D5}, // PowerA PS3 (alt VID)
    {0x20D6, 0x5795}, // Pro Elite PS3
    {0x20D6, 0x319F}, // Pro Ex mini PS3
    {0x20D6, 0x57C7}, // Pro Ex mini PS3
    {0x120A, 0x0001}, // PS3 Controller (generic)
    {0x120C, 0x0713}, // PS3 Controller (generic)
    {0x120C, 0xF11C}, // PS3 Controller (generic)
    {0x120C, 0x0EF9}, // PS3 Controller (generic)
    {0x2509, 0x1801}, // PS3 Controller (generic)
    {0x2509, 0x0005}, // PS3 Controller (generic)
    {0x1F4F, 0x0008}, // PS3 Controller (generic)
    {0x2563, 0x0575}, // SHANWAN / generic PS3
    {0x8888, 0x0308}, // PS3 Controller (generic)
    {0x8888, 0x0408}, // PS3 Controller (generic)
};

// PS4 controllers (from SDL_GameControllerDB + existing)
static const HardwareID PS4_IDS[] =
{
    {0x054C, 0x05C4}, // Sony DS4
    {0x054C, 0x09CC}, // Sony DS4
    {0x054C, 0x0BA0}, // Sony DS4 wireless adapter
    {0x2563, 0x0357}, // MPOW Wired Gamepad (ShenZhen ShanWan)
    {0x0F0D, 0x005E}, // Hori Fighting Commander 4 PS4
    {0x0F0D, 0x00EE}, // Hori PS4 Mini (PS4-099U)
    {0x0F0D, 0x008A}, // Hori Real Arcade Pro 4 / RAP4 Kai (PS4 mode)
    {0x0F0D, 0x008B}, // Hori Real Arcade Pro 4 (PS4 mode alt)
    {0x1F4F, 0x1002}, // ASW GG Xrd controller
    {0x9886, 0x0025}, // Astro C40 TR PS4
    {0x20D6, 0x792A}, // BDA PS4 Fightpad
    {0x120C, 0x0E20}, // Brook Mars PS4
    {0x120C, 0x0E21}, // Brook Mars PS4
    {0x11C0, 0x4001}, // GameStop PS4 Fun Controller
    {0x0F0D, 0x0087}, // Hori Fighting Stick mini 4 PS4
    {0x0F0D, 0x00A5}, // Hori Miku Project Diva X HD PS4
    {0x0F0D, 0x00A6}, // Hori Miku Project Diva X HD PS4
    {0x0F0D, 0x0123}, // Hori PS4 Controller Light
    {0x0F0D, 0x0066}, // Horipad 4 PS4
    {0x0738, 0x8250}, // Mad Catz Fightpad Pro PS4
    {0x0738, 0x8384}, // Mad Catz Fightstick TE S PS4
    {0x0738, 0x8481}, // Mad Catz Fightstick TE2 PS4
    {0x0738, 0x8180}, // Mad Catz SFV Arcade Fightstick Alpha PS4
    {0x0738, 0x8480}, // Mad Catz TE2 PS4 Fightstick
    {0x146B, 0x0611}, // Nacon Revolution 3 PS4
    {0x146B, 0x0D10}, // Nacon Revolution Infinity PS4
    {0x120C, 0x0708}, // PS4 Controller (generic)
    {0x120C, 0x1E11}, // PS4 Controller (generic)
    {0x120C, 0x1E12}, // PS4 Controller (generic)
    {0x120C, 0x0E13}, // PS4 Controller (generic)
    {0x120C, 0x0E15}, // PS4 Controller (generic)
    {0x120C, 0x0E18}, // PS4 Controller (generic)
    {0x120C, 0x1E18}, // PS4 Controller (generic)
    {0x120C, 0x1E19}, // PS4 Controller (generic)
    {0x120C, 0x0E1E}, // PS4 Controller (generic)
    {0x120C, 0x57A9}, // PS4 Controller (generic)
    {0x120C, 0x57AA}, // PS4 Controller (generic)
    {0x120C, 0x1CF2}, // PS4 Controller (generic)
    {0x120C, 0x1CF3}, // PS4 Controller (generic)
    {0x120C, 0x1CF4}, // PS4 Controller (generic)
    {0x120C, 0x1CF5}, // PS4 Controller (generic)
    {0x120C, 0x0EF7}, // PS4 Controller (generic)
    {0x120E, 0x0C12}, // PS4 Controller (generic)
    {0x160E, 0x0C12}, // PS4 Controller (generic)
    {0x1E1A, 0x0C12}, // PS4 Controller (generic)
    {0x2C22, 0x2300}, // Qanba Obsidian Arcade Stick PS4
    {0x1532, 0x0401}, // Razer Panthera PS4
    {0x1532, 0x1100}, // Razer Raion PS4 Fightpad
    {0x2E95, 0x7725}, // Scuf PS4
    {0x120C, 0x1E1C}, // SnakeByte 4S PS4
    {0x120C, 0x0E16}, // Steel Play Metaltech PS4
    {0x0079, 0x181A}, // Venom PS4 Arcade Joystick
};

static const HardwareID PS5_IDS[] =
{
    {0x054C, 0x0CE6}, // Sony DualSense (PS5)
    {0x054C, 0x0DF2}, // Sony DualSense Edge (PS5)
    {0x054C, 0x0E5F}  // Sony PS5 Access Controller
};

static const HardwareID PSCLASSIC_IDS[] =
{
    {0x054C, 0x0CDA} // psclassic
};

// Switch Pro / Joy-Con / Wii U Pro (from SDL_GameControllerDB + existing)
static const HardwareID SWITCH_PRO_IDS[] =
{
    {0x057E, 0x2009}, // Nintendo Switch Pro Controller
    {0x057E, 0x0330}, // Wii U Pro
    {0x057E, 0x2006}, // Joy-Con (L)
    {0x057E, 0x2007}, // Joy-Con (R)
    {0x057E, 0x2067}, // Joy-Con 2 (L) — same USB bulk bring-up as Pro 2
    {0x057E, 0x2066}, // Joy-Con 2 (R)
    {0x057E, 0x2073}, // NSO GameCube Controller (Switch 2–family USB)
    // {0x20D6, 0xA711}, // OpenSteamController, emulated pro controller
};

// Switch 2 Pro only — uses Switch2ProHost (same InReport decode as SwitchProHost; see Wired_Controllers.md)
static const HardwareID SWITCH_PRO_2_IDS[] =
{
    {0x057E, 0x2069}, // Nintendo Switch 2 Pro Controller
};

// Wired Switch controllers (from SDL_GameControllerDB + existing)
static const HardwareID SWITCH_WIRED_IDS[] =
{
    {0x20D6, 0xA719}, // PowerA wired
    {0x20D6, 0xA713}, // PowerA Enhanced wired
    {0x20D6, 0xA712}, // PowerA Fusion Fight Pad
    {0x20D6, 0xA714}, // PowerA Spectra
    {0x62DD, 0xA715}, // PowerA Fusion Arcade Stick
    {0x62DD, 0xA716}, // PowerA Fusion Pro
    {0x0F0D, 0x0092}, // Hori Pokken Tournament Pro
    {0x0F0D, 0x00C1}, // Hori Pokken Horipad
    {0x0F0D, 0x00F6}, // Hori Horipad Nintendo Switch
    {0x0F0D, 0x0202}, // Hori Horipad O Nintendo Switch 2
    {0x0F0D, 0x00DC}, // Hori Horipad Switch
    {0x0E6F, 0x0188}, // Afterglow Deluxe Nintendo Switch
    {0x0E6F, 0x0184}, // Faceoff Deluxe Nintendo Switch
    {0x0E6F, 0x0181}, // Faceoff Deluxe Pro Nintendo Switch
    {0x0E6F, 0x0180}, // Faceoff Pro Nintendo Switch
    {0x0E6F, 0x0189}, // PDP Realmz Nintendo Switch
    {0x0E6F, 0x0187}, // Rock Candy Nintendo Switch
    {0x146B, 0x0D08}, // Nacon Revolution Unlimited Pro
    {0x146B, 0x0D01}, // Nacon Revolution Pro Controller
    {0x146B, 0x0D02}, // Nacon Revolution Pro Controller 2
    {0x146B, 0x0D13}, // Nacon Revolution Pro Controller 3
    {0x11EC, 0xA7E1}, // Nintendo Switch (generic)
    {0x044F, 0xD00E}, // Thrustmaster eSwap Pro Controller
    {0x1DD8, 0x000B}, // Buffalo BSGP1601 (from SDL_GameControllerDB)
    {0x1DD8, 0x000F}, // iBuffalo BSGP1204
    {0x1DD8, 0x0010}, // iBuffalo BSGP1204P
};

static const HardwareID N64_IDS[] =
{
    {0x0079, 0x0006}, // Retrolink N64 USB gamepad
    {0x2E24, 0x200B}, // Hyperkin Admiral N64 (from SDL_GameControllerDB)
    {0x0079, 0x954E}, // Hyperkin N64 Controller Adapter
    {0x0079, 0x1879}, // Mayflash N64 Adapter
    {0x2F24, 0x00F4}, // Mayflash N64 Adapter
    {0x057E, 0x2019}, // NSO N64 Controller
    {0x1234, 0x0004}, // RetroUSB N64 RetroPort
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
    { SWITCH_PRO_2_IDS, sizeof(SWITCH_PRO_2_IDS) / sizeof(HardwareID), HostDriverType::SWITCH_PRO_2 },
    { SWITCH_PRO_IDS, sizeof(SWITCH_PRO_IDS) / sizeof(HardwareID), HostDriverType::SWITCH_PRO },
    { PSCLASSIC_IDS, sizeof(PSCLASSIC_IDS) / sizeof(HardwareID), HostDriverType::PSCLASSIC },
    { N64_IDS, sizeof(N64_IDS) / sizeof(HardwareID), HostDriverType::N64 },
};

#endif // _HW_ID_H_