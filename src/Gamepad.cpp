#include <cstdint>

#include "Gamepad.h"

void Gamepad::reset_state() 
{
    state.up = state.down = state.left = state.right = false;
    state.a = state.b = state.x = state.y = false;
    state.l3 = state.r3 = state.back = state.start = false;
    state.rb = state.lb = state.sys = state.misc = false;
    state.lt = state.rt = 0;
    state.lx = state.ly = state.rx = state.ry = 0;
}

void GamepadOut::reset_hid_rumble()
{
    if (state.lrumble != UINT8_MAX)
    {
        state.lrumble = 0;
    }
    
    if (state.rrumble != UINT8_MAX)
    {
        state.rrumble = 0;
    }
}