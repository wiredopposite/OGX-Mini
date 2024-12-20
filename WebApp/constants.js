export const DEVICE_MODE_KEY_VALUE =
{
    "Xbox OG" : 7,
    "Xbox OG: Steel Battalion" : 8,
    "Xbox OG: XRemote" : 9,
    "XInput" : 6,
    "PS3" : 1,
    "PS Classic" : 5,
    "Switch" : 4,
    "WebApp" : 100,
};

export const PROFILE_ID_KEY_VALUE = 
{
    "Profile 1": 0x01,
    "Profile 2": 0x02,
    "Profile 3": 0x03,
    "Profile 4": 0x04,
    "Profile 5": 0x05,
    "Profile 6": 0x06,
    "Profile 7": 0x07,
    "Profile 8": 0x08,
};

export const DPAD_KEY_VALUE =
{
    "Up"    : 0x01,
    "Down"  : 0x02,
    "Left"  : 0x04,
    "Right" : 0x08,
};

export const BUTTON_KEY_VALUE = 
{
    "A"       : 0x0001,
    "B"       : 0x0002,
    "X"       : 0x0004,
    "Y"       : 0x0008,
    "L3"      : 0x0010,
    "R3"      : 0x0020,
    "Back"    : 0x0040,
    "Start"   : 0x0080,
    "LB"      : 0x0100,
    "RB"      : 0x0200,
    "Guide"   : 0x0400,
    "Misc"    : 0x0800
};

export const ANALOG_KEY_VALUE =
{
    "Up"    : 0x00,
    "Down"  : 0x01,
    "Left"  : 0x02,
    "Right" : 0x03,
    "A"     : 0x04,
    "B"     : 0x05,
    "X"     : 0x06,
    "Y"     : 0x07,
    "LB"    : 0x08,
    "RB"    : 0x09
};

export const PACKET_IDS =
{
    INIT_READ      : 0x88,
    READ_PROFILE   : 0x01,
    WRITE_PROFILE  : 0x02,
    // WRITE_MODE     : 0x03,
    RESPONSE_OK    : 0x10,
    RESPONSE_ERROR : 0x11,
};

export function new_packet()
{
    return {
        report_id: 0,
        input_mode: 0,
        max_gamepads: 0,
        player_idx: 0,

        profile:
        {
            id: 0,

            dz_trigger_l: 0,
            dz_trigger_r: 0,

            dz_joystick_l: 0,
            dz_joystick_r: 0,
        
            invert_ly: 0,
            invert_ry: 0,
        
            dpad_up: 0x01,
            dpad_down: 0x02,
            dpad_left: 0x04,
            dpad_right: 0x08,
        
            button_a: 0x0001,
            button_b: 0x0002,
            button_x: 0x0004,
            button_y: 0x0008,
            button_l3: 0x0010,
            button_r3: 0x0020,
            button_back: 0x0040,
            button_start: 0x0080,
            button_lb: 0x0100,
            button_rb: 0x0200,
            button_sys: 0x0400,
            button_misc: 0x0800,
        
            analog_enabled: 1,
        
            analog_off_up: 0,
            analog_off_down: 1,
            analog_off_left: 2,
            analog_off_right: 3,
            analog_off_a: 4,
            analog_off_b: 5,
            analog_off_x: 6,
            analog_off_y: 7,
            analog_off_lb: 8,
            analog_off_rb: 9
        }
    }
};