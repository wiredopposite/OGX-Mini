#include <cstdint>

#include "Gamepad.h"

int16_t scale_uint8_to_int16(uint8_t value, bool invert) 
{
    const uint32_t scaling_factor = 65535;
    const int32_t bias = -32768;

    int32_t scaled_value = ((uint32_t)value * scaling_factor) >> 8;
    scaled_value += bias;

    if (scaled_value < -32768) 
    {
        scaled_value = -32768;
    } 
    else if (scaled_value > 32767) 
    {
        scaled_value = 32767;
    }

    if (invert)
    {
        scaled_value = -scaled_value - 1;
    }

    return (int16_t)scaled_value;
}

void Gamepad::update_gamepad_state_from_dualshock4(const sony_ds4_report_t* ds4_data)
{
    reset_state();

    switch(ds4_data->dpad)
    {
        case PS4_DPAD_MASK_UP:
            state.up = true;
            break;
        case PS4_DPAD_MASK_UP_RIGHT:
            state.up = true;
            state.right = true;
            break;
        case PS4_DPAD_MASK_RIGHT:
            state.right = true;
            break;
        case PS4_DPAD_MASK_RIGHT_DOWN:
            state.right = true;
            state.down = true;
            break;
        case PS4_DPAD_MASK_DOWN:
            state.down = true;
            break;
        case PS4_DPAD_MASK_DOWN_LEFT:
            state.down = true;
            state.left = true;
            break;
        case PS4_DPAD_MASK_LEFT:
            state.left = true;
            break;
        case PS4_DPAD_MASK_LEFT_UP:
            state.left = true;
            state.up = true;
            break;
    }

    if (ds4_data->square)   state.x = true;
    if (ds4_data->cross)    state.a = true;
    if (ds4_data->circle)   state.b = true;
    if (ds4_data->triangle) state.y = true;

    if (ds4_data->share)    state.back = true;
    if (ds4_data->option)   state.start = true;
    if (ds4_data->ps)       state.sys = true;

    if (ds4_data->l1) state.lb = true;
    if (ds4_data->r1) state.rb = true;

    if (ds4_data->l3) state.l3 = true;
    if (ds4_data->r3) state.r3 = true;

    state.lt = ds4_data->l2_trigger;
    state.rt = ds4_data->r2_trigger;

    state.lx = scale_uint8_to_int16(ds4_data->lx, false);
    state.ly = scale_uint8_to_int16(ds4_data->ly, true);
    state.rx = scale_uint8_to_int16(ds4_data->rx, false);
    state.ry = scale_uint8_to_int16(ds4_data->ry, true);
}

void Gamepad::update_gamepad_state_from_dualsense(const dualsense_input_report* ds_data) 
{
    reset_state();

    switch(ds_data->button[0]) 
    {
        case PS5_MASK_DPAD_UP:
            state.up = true;
            break;
        case PS5_MASK_DPAD_UP_RIGHT:
            state.up = true;
            state.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT:
            state.right = true;
            break;
        case PS5_MASK_DPAD_RIGHT_DOWN:
            state.right = true;
            state.down = true;
            break;
        case PS5_MASK_DPAD_DOWN:
            state.down = true;
            break;
        case PS5_MASK_DPAD_DOWN_LEFT:
            state.down = true;
            state.left = true;
            break;
        case PS5_MASK_DPAD_LEFT:
            state.left = true;
            break;
        case PS5_MASK_DPAD_LEFT_UP:
            state.left = true;
            state.up = true;
            break;
    }

    if (ds_data->button[0] & PS5_MASK_SQUARE) state.x = true;
    if (ds_data->button[0] & PS5_MASK_CROSS) state.a = true;
    if (ds_data->button[0] & PS5_MASK_CIRCLE) state.b = true;
    if (ds_data->button[0] & PS5_MASK_TRIANGLE) state.y = true;

    if (ds_data->button[1] & PS5_MASK_L1) state.lb = true;
    if (ds_data->button[1] & PS5_MASK_R1) state.rb = true;

    if (ds_data->button[1] & PS5_MASK_SHARE) state.back = true;
    if (ds_data->button[1] & PS5_MASK_OPTIONS) state.start = true;
    
    if (ds_data->button[1] & PS5_MASK_L3) state.l3 = true;
    if (ds_data->button[1] & PS5_MASK_R3) state.r3 = true;

    if (ds_data->button[2] & PS5_MASK_PS) state.sys = true;

    state.lt = ds_data->lt;
    state.rt = ds_data->rt;

    state.lx = scale_uint8_to_int16(ds_data->lx, false);
    state.ly = scale_uint8_to_int16(ds_data->ly, true);
    state.rx = scale_uint8_to_int16(ds_data->rx, false);
    state.ry = scale_uint8_to_int16(ds_data->ry, true);
}

void Gamepad::update_gamepad_state_from_xinput(const xinput_gamepad_t* xinput_data) 
{
    reset_state();

    state.up    = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
    state.down  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
    state.left  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
    state.right = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
    
    state.a     = (xinput_data->wButtons & XINPUT_GAMEPAD_A) != 0;
    state.b     = (xinput_data->wButtons & XINPUT_GAMEPAD_B) != 0;
    state.x     = (xinput_data->wButtons & XINPUT_GAMEPAD_X) != 0;
    state.y     = (xinput_data->wButtons & XINPUT_GAMEPAD_Y) != 0;
    
    state.l3    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    state.r3    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
    state.back  = (xinput_data->wButtons & XINPUT_GAMEPAD_BACK) != 0;
    state.start = (xinput_data->wButtons & XINPUT_GAMEPAD_START) != 0;
    
    state.rb    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
    state.lb    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    state.sys   = (xinput_data->wButtons & XINPUT_GAMEPAD_GUIDE) != 0;   

    state.lt = xinput_data->bLeftTrigger;
    state.rt = xinput_data->bRightTrigger;

    state.lx = xinput_data->sThumbLX;
    state.ly = xinput_data->sThumbLY;

    state.rx = xinput_data->sThumbRX;
    state.ry = xinput_data->sThumbRY;
}

void Gamepad::reset_state() 
{
    state.up = state.down = state.left = state.right = false;
    state.a = state.b = state.x = state.y = false;
    state.l3 = state.r3 = state.back = state.start = false;
    state.rb = state.lb = state.sys = false;
    state.lt = state.rt = 0;
    state.lx = state.ly = state.rx = state.ry = 0;
}

void GamepadOut::update_gamepad_rumble(uint8_t left_rumble, uint8_t right_rumble) 
{
    out_state.lrumble = left_rumble;
    out_state.rrumble = right_rumble;}