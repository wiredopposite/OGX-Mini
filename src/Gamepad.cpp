#include "Gamepad.h"
#include "board_config.h"

void Gamepad::reset_pad() 
{
    buttons.up = buttons.down = buttons.left = buttons.right = false;
    buttons.a  = buttons.b    = buttons.x    = buttons.y     = false;
    buttons.l3 = buttons.r3   = buttons.back = buttons.start = false;
    buttons.rb = buttons.lb   = buttons.sys  = buttons.misc  = false;

    triggers.l = triggers.r = 0;

    joysticks.lx = joysticks.ly = joysticks.rx = joysticks.ry = 0;
}

void Gamepad::reset_rumble() 
{
    rumble.r = rumble.l = 0;
}

void Gamepad::reset_hid_rumble()
{
    if (rumble.l != UINT8_MAX)
    {
        rumble.l = 0;
    }
    
    if (rumble.r != UINT8_MAX)
    {
        rumble.r = 0;
    }
}

Gamepad& gamepad(int idx)
{
    static Gamepad gamepad[MAX_GAMEPADS];
    
    return gamepad[idx];
}