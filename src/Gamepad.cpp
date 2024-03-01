#include <cstdint>

#include "Gamepad.h"
#include "usbh/tusb_hid/shared.h"

void Gamepad::reset_state() 
{
    state.up = state.down = state.left = state.right = false;
    state.a = state.b = state.x = state.y = false;
    state.l3 = state.r3 = state.back = state.start = false;
    state.rb = state.lb = state.sys = state.misc = false;
    state.lt = state.rt = 0;
    state.lx = state.ly = state.rx = state.ry = 0;
}

void GamepadOut::update_gamepad_rumble(uint8_t left_rumble, uint8_t right_rumble) 
{
    out_state.lrumble = left_rumble;
    out_state.rrumble = right_rumble;
}

void GamepadOut::rumble_hid_reset()
{
    if (out_state.lrumble != UINT8_MAX)
    {
        out_state.lrumble = 0;
    }
    
    if (out_state.rrumble != UINT8_MAX)
    {
        gamepadOut.out_state.rrumble = 0;
    }
}