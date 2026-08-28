#ifndef _XBOX_ARCADE_STICK_H_
#define _XBOX_ARCADE_STICK_H_

#include <cstdint>

#include "Gamepad/Gamepad.h"
#include "USBHost/HardwareIDs.h"

/** Xbox One GIP arcade sticks (Razer Atrox XBO, Mad Catz TE2, etc.)
 *  Init/read from OOPMan/XBOFS.win; standard pads use 64-byte Linux xpad GIP. */
namespace XboxArcadeStick
{
    /** XBOFS: WinUsb_ReadPipe(..., 30). */
    static constexpr uint16_t GIP_IN_XFER_SIZE = 30;

    static constexpr HardwareID XBOX_ONE_GIP_IDS[] = {
        {0x1532, 0x0a00}, // Razer Atrox Arcade Stick (Xbox One)
        {0x0738, 0x4a01}, // Mad Catz FightStick TE 2 (Xbox One)
        {0x0e6f, 0x015c}, // PDP Xbox One Arcade Stick
        {0x0f0d, 0x0063}, // Hori RAP Hayabusa (Xbox One)
        {0x0f0d, 0x0078}, // Hori RAP V Kai (Xbox One)
        {0x0f0d, 0x00c5}, // Hori Fighting Commander ONE
    };

    static constexpr HardwareID XBOX360_DIGITAL_TRIGGER_IDS[] = {
        {0x24c6, 0x5000}, // Razer Atrox Arcade Stick (Xbox 360)
        {0x0738, 0x4758}, // Mad Catz Arcade Game Stick
        {0x0738, 0x4728}, // Mad Catz Fightpad
        {0x0738, 0x4738}, // Mad Catz SFIV Fightstick
        {0x146b, 0x0604}, // Bigben DAIJA Arcade Stick
    };

    inline bool matches_id(uint16_t vid, uint16_t pid, const HardwareID* ids, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (ids[i].vid == vid && ids[i].pid == pid)
            {
                return true;
            }
        }
        return false;
    }

    inline bool is_xbox_one_gip(uint16_t vid, uint16_t pid)
    {
        return matches_id(vid, pid, XBOX_ONE_GIP_IDS,
            sizeof(XBOX_ONE_GIP_IDS) / sizeof(XBOX_ONE_GIP_IDS[0]));
    }

    inline bool is_xbox360_digital_triggers(uint16_t vid, uint16_t pid)
    {
        return matches_id(vid, pid, XBOX360_DIGITAL_TRIGGER_IDS,
            sizeof(XBOX360_DIGITAL_TRIGGER_IDS) / sizeof(XBOX360_DIGITAL_TRIGGER_IDS[0]));
    }

    /** XBOFS GIP 0x20 layout (face/LT/RT on byte 22). */
    inline void map_gip_arcade_report(const uint8_t* data, uint16_t len, Gamepad& gamepad, Gamepad::PadIn& gp_in)
    {
        if (len < 23)
        {
            return;
        }

        if (data[4] & 0x04)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_START;
        }
        if (data[4] & 0x08)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
        }

        if (data[5] & 0x01)
        {
            gp_in.dpad |= gamepad.MAP_DPAD_UP;
        }
        if (data[5] & 0x02)
        {
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
        }
        if (data[5] & 0x04)
        {
            gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
        }
        if (data[5] & 0x08)
        {
            gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
        }

        const uint8_t face = data[22];
        if (face & 0x01)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_X;
        }
        if (face & 0x02)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_Y;
        }
        if (face & 0x10)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_A;
        }
        if (face & 0x20)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_B;
        }
        if (face & 0x04)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_RB;
        }
        if (face & 0x08)
        {
            gp_in.buttons |= gamepad.MAP_BUTTON_LB;
        }
        if (face & 0x40)
        {
            gp_in.trigger_r = 0xFF;
        }
        if (face & 0x80)
        {
            gp_in.trigger_l = 0xFF;
        }
    }

    inline bool gip_arcade_report_changed(const uint8_t* data, uint16_t len, const uint8_t* prev, uint16_t prev_len)
    {
        if (len < 23 || prev_len < 23)
        {
            return true;
        }
        return data[4] != prev[4] || data[5] != prev[5] || data[22] != prev[22];
    }
}

#endif // _XBOX_ARCADE_STICK_H_
