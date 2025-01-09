#include <cstring>
#include <cstdlib>
#include <pico/time.h>

#include "Descriptors/XInput.h"
#include "USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.h"
#include "USBDevice/DeviceDriver/XboxOG/XboxOG_SB.h"

static constexpr std::array<XboxOGSBDevice::ButtonMap, 9> GP_MAP = 
{{
    {Gamepad::BUTTON_START,   XboxOG::SB::Buttons0::START,              0},
    {Gamepad::BUTTON_LB,      XboxOG::SB::Buttons0::RIGHTJOYFIRE,       0},
    {Gamepad::BUTTON_R3,      XboxOG::SB::Buttons0::RIGHTJOYLOCKON,     0},
    {Gamepad::BUTTON_B,       XboxOG::SB::Buttons0::RIGHTJOYLOCKON,     0},
    {Gamepad::BUTTON_RB,      XboxOG::SB::Buttons0::RIGHTJOYMAINWEAPON, 0},
    {Gamepad::BUTTON_A,       XboxOG::SB::Buttons0::RIGHTJOYMAINWEAPON, 0},
    {Gamepad::BUTTON_SYS,     XboxOG::SB::Buttons0::EJECT,              0},
    {Gamepad::BUTTON_L3,      XboxOG::SB::Buttons2::LEFTJOYSIGHTCHANGE, 2},
    {Gamepad::BUTTON_Y,       XboxOG::SB::Buttons1::CHAFF,              1}
}};

static constexpr std::array<XboxOGSBDevice::ButtonMap, 19> CHATPAD_MAP =
{{
    {XInput::Chatpad::CODE_0,      XboxOG::SB::Buttons0::EJECT,                0},
    {XInput::Chatpad::CODE_D,      XboxOG::SB::Buttons1::WASHING,              1},
    {XInput::Chatpad::CODE_F,      XboxOG::SB::Buttons1::EXTINGUISHER,         1},
    {XInput::Chatpad::CODE_G,      XboxOG::SB::Buttons1::CHAFF,                1},
    {XInput::Chatpad::CODE_X,      XboxOG::SB::Buttons1::WEAPONCONMAIN,        1},
    {XInput::Chatpad::CODE_RIGHT,  XboxOG::SB::Buttons1::WEAPONCONMAIN,        1},
    {XInput::Chatpad::CODE_C,      XboxOG::SB::Buttons1::WEAPONCONSUB,         1},
    {XInput::Chatpad::CODE_LEFT,   XboxOG::SB::Buttons1::WEAPONCONSUB,         1},
    {XInput::Chatpad::CODE_V,      XboxOG::SB::Buttons1::WEAPONCONMAGAZINE,    1},
    {XInput::Chatpad::CODE_SPACE,  XboxOG::SB::Buttons1::WEAPONCONMAGAZINE,    1},
    {XInput::Chatpad::CODE_U,      XboxOG::SB::Buttons0::MULTIMONOPENCLOSE,    0},
    {XInput::Chatpad::CODE_J,      XboxOG::SB::Buttons0::MULTIMONMODESELECT,   0},
    {XInput::Chatpad::CODE_N,      XboxOG::SB::Buttons0::MAINMONZOOMIN,        0},
    {XInput::Chatpad::CODE_I,      XboxOG::SB::Buttons0::MULTIMONMAPZOOMINOUT, 0},
    {XInput::Chatpad::CODE_K,      XboxOG::SB::Buttons0::MULTIMONSUBMONITOR,   0},
    {XInput::Chatpad::CODE_M,      XboxOG::SB::Buttons0::MAINMONZOOMOUT,       0},
    {XInput::Chatpad::CODE_ENTER,  XboxOG::SB::Buttons0::START,                0},
    {XInput::Chatpad::CODE_P,      XboxOG::SB::Buttons0::COCKPITHATCH,         0},
    {XInput::Chatpad::CODE_COMMA,  XboxOG::SB::Buttons0::IGNITION,             0}
}};

static constexpr std::array<XboxOGSBDevice::ButtonMap, 5> CHATPAD_MAP_ALT1 =
{{
    {XInput::Chatpad::CODE_1, XboxOG::SB::Buttons1::COMM1, 1},
    {XInput::Chatpad::CODE_2, XboxOG::SB::Buttons1::COMM2, 1},
    {XInput::Chatpad::CODE_3, XboxOG::SB::Buttons1::COMM3, 1},
    {XInput::Chatpad::CODE_4, XboxOG::SB::Buttons1::COMM4, 1},
    {XInput::Chatpad::CODE_5, XboxOG::SB::Buttons2::COMM5, 2}
}};

static constexpr std::array<XboxOGSBDevice::ButtonMap, 9> CHATPAD_MAP_ALT2=
{{
    {XInput::Chatpad::CODE_1, XboxOG::SB::Buttons1::FUNCTIONF1,               1},
    {XInput::Chatpad::CODE_2, XboxOG::SB::Buttons1::FUNCTIONTANKDETACH,       1},
    {XInput::Chatpad::CODE_3, XboxOG::SB::Buttons0::FUNCTIONFSS,              0},
    {XInput::Chatpad::CODE_4, XboxOG::SB::Buttons1::FUNCTIONF2,               1},
    {XInput::Chatpad::CODE_5, XboxOG::SB::Buttons1::FUNCTIONOVERRIDE,         1},
    {XInput::Chatpad::CODE_6, XboxOG::SB::Buttons0::FUNCTIONMANIPULATOR,      0},
    {XInput::Chatpad::CODE_7, XboxOG::SB::Buttons1::FUNCTIONF3,               1},
    {XInput::Chatpad::CODE_8, XboxOG::SB::Buttons1::FUNCTIONNIGHTSCOPE,       1},
    {XInput::Chatpad::CODE_9, XboxOG::SB::Buttons0::FUNCTIONLINECOLORCHANGE,  0}
}};

static constexpr std::array<XboxOGSBDevice::ButtonMap, 5> CHATPAD_TOGGLE_MAP =
{{
    {XInput::Chatpad::CODE_Q, XboxOG::SB::Buttons2::TOGGLEOXYGENSUPPLY,   2},
    {XInput::Chatpad::CODE_A, XboxOG::SB::Buttons2::TOGGLEFILTERCONTROL,  2},
    {XInput::Chatpad::CODE_W, XboxOG::SB::Buttons2::TOGGLEVTLOCATION,     2},
    {XInput::Chatpad::CODE_S, XboxOG::SB::Buttons2::TOGGLEBUFFREMATERIAL, 2},
    {XInput::Chatpad::CODE_Z, XboxOG::SB::Buttons2::TOGGLEFUELFLOWRATE,   2}
}};

void XboxOGSBDevice::initialize() 
{
    tud_xid::initialize(tud_xid::Type::STEELBATTALION);
    class_driver_ = *tud_xid::class_driver();

    std::memset(&in_report_, 0, sizeof(XboxOG::SB::InReport));
    in_report_.bLength = sizeof(XboxOG::SB::InReport);
    in_report_.gearLever = XboxOG::SB::Gear::N;

    prev_in_report_ = in_report_;
}

void XboxOGSBDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    Gamepad::PadIn gp_in = gamepad.get_pad_in();
    Gamepad::ChatpadIn gp_in_chatpad = gamepad.get_chatpad_in();

    in_report_.dButtons[0] = 0;
    in_report_.dButtons[1] = 0;
    in_report_.dButtons[2] &= XboxOG::SB::BUTTONS2_TOGGLE_MID;

    for (const auto& map : GP_MAP)
    {
        if (gp_in.buttons & map.gp_mask)
        {
            in_report_.dButtons[map.button_offset] |= map.sb_mask;
        }
    }

    for (const auto& map : CHATPAD_MAP)
    {
        if (chatpad_pressed(gp_in_chatpad, map.gp_mask))
        {
            in_report_.dButtons[map.button_offset] |= map.sb_mask;
        }
    }

    static std::array<bool, CHATPAD_TOGGLE_MAP.size() + 1> toggle_pressed{false};

    for (uint8_t i = 0; i < CHATPAD_TOGGLE_MAP.size(); i++)
    {
        if (chatpad_pressed(gp_in_chatpad, CHATPAD_TOGGLE_MAP[i].gp_mask))
        {
            if (!toggle_pressed[i])
            {
                in_report_.dButtons[CHATPAD_TOGGLE_MAP[i].button_offset] ^= CHATPAD_TOGGLE_MAP[i].sb_mask;
                toggle_pressed[i] = true;
            }
        }
        else
        {
            toggle_pressed[i] = false;
        }
    }

    if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_SHIFT))
    {
        if (!toggle_pressed.back())
        {
            if (in_report_.dButtons[2] & XboxOG::SB::BUTTONS2_TOGGLE_MID)
            {
                in_report_.dButtons[2] &= ~XboxOG::SB::BUTTONS2_TOGGLE_MID;
            }
            else
            {
                in_report_.dButtons[2] |= XboxOG::SB::BUTTONS2_TOGGLE_MID;
            }
            toggle_pressed.back() = true;
        }
    }
    else
    {
        toggle_pressed.back() = false;
    }

    if (gp_in.buttons & Gamepad::BUTTON_X)
    {
        if (out_report_.Chaff_Extinguisher & 0x0F)
        {
            in_report_.dButtons[1] |= XboxOG::SB::Buttons1::EXTINGUISHER;
        }
        if (out_report_.Comm1_MagazineChange & 0x0F)
        {
            in_report_.dButtons[1] |= XboxOG::SB::Buttons1::WEAPONCONMAGAZINE;
        }
        if (out_report_.Washing_LineColorChange & 0xF0)
        {
            in_report_.dButtons[1] |= XboxOG::SB::Buttons1::WASHING;
        }
    }

    if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_MESSENGER) || gp_in.buttons & Gamepad::BUTTON_BACK)
    {
        for (const auto& map : CHATPAD_MAP_ALT1)
        {
            if (chatpad_pressed(gp_in_chatpad, map.gp_mask))
            {
                in_report_.dButtons[map.button_offset] |= map.sb_mask;
            }
        }

        if (gp_in.dpad & Gamepad::DPAD_UP && dpad_reset_)
        {
            in_report_.tunerDial = (in_report_.tunerDial + 1) % 16;
            dpad_reset_ = false;
        }
        else if (gp_in.dpad & Gamepad::DPAD_DOWN && dpad_reset_)
        {
            in_report_.tunerDial = (in_report_.tunerDial + 15) % 16;
            dpad_reset_ = false;
        }
        else if (!(gp_in.dpad & Gamepad::DPAD_DOWN) && !(gp_in.dpad & Gamepad::DPAD_UP))
        {
            dpad_reset_ = true;
        }
    }
    else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_ORANGE))
    {
        for (const auto& map : CHATPAD_MAP_ALT2)
        {
            if (chatpad_pressed(gp_in_chatpad, map.gp_mask))
            {
                in_report_.dButtons[map.button_offset] |= map.sb_mask;
            }
        }

        // if (!(gp_in.dpad & Gamepad::DPAD_LEFT) && !(gp_in.dpad & Gamepad::DPAD_RIGHT))
        // {
            if (gp_in.dpad & Gamepad::DPAD_UP && dpad_reset_)
            {
                if (in_report_.gearLever < XboxOG::SB::Gear::G5)
                {
                    in_report_.gearLever++;
                }
                dpad_reset_ = false;
            }
            else if (gp_in.dpad & Gamepad::DPAD_DOWN && dpad_reset_)
            {
                if (in_report_.gearLever > XboxOG::SB::Gear::R)
                {
                    in_report_.gearLever--;
                }
                dpad_reset_ = false;
            }
            else if (!(gp_in.dpad & Gamepad::DPAD_DOWN) && !(gp_in.dpad & Gamepad::DPAD_UP))
            {
                dpad_reset_ = true;
            }
        // }
    }
    else
    {
        dpad_reset_ = true;
    }

    in_report_.leftPedal    = Scale::uint8_to_uint16(gp_in.trigger_l);
    in_report_.rightPedal   = Scale::uint8_to_uint16(gp_in.trigger_r);
    in_report_.middlePedal  = chatpad_pressed(  gp_in_chatpad, XInput::Chatpad::CODE_BACK) ? 0xFF00 : 0x0000;
    in_report_.rotationLever= chatpad_pressed(  gp_in_chatpad, XInput::Chatpad::CODE_MESSENGER) 
                                                    ? 0 : (gp_in.buttons & Gamepad::BUTTON_BACK) 
                                                        ? 0 : (gp_in.dpad & Gamepad::DPAD_LEFT)  
                                                            ? Range::MIN<int16_t> : (gp_in.dpad & Gamepad::DPAD_RIGHT) 
                                                                ? Range::MAX<int16_t> : 0;

    in_report_.sightChangeX = gp_in.joystick_lx;
    in_report_.sightChangeY = gp_in.joystick_ly;

    int32_t axis_value_x = static_cast<int32_t>(gp_in.joystick_rx);
    if (std::abs(axis_value_x) > DEFAULT_DEADZONE)
    {
        vmouse_x_ += axis_value_x / sensitivity_;
    }

    int32_t axis_value_y = static_cast<int32_t>(Range::invert(gp_in.joystick_ry));
    if (std::abs(axis_value_y) > DEFAULT_DEADZONE)
    {
        vmouse_y_ -= axis_value_y / sensitivity_;
    }

    if (vmouse_x_ < 0) vmouse_x_ = 0;
    if (vmouse_x_ > Range::MAX<uint16_t>) vmouse_x_ = Range::MAX<uint16_t>;
    if (vmouse_y_ > Range::MAX<uint16_t>) vmouse_y_ = Range::MAX<uint16_t>;
    if (vmouse_y_ < 0) vmouse_y_ = 0;

    if (gp_in.buttons & Gamepad::BUTTON_L3)
    {
        if ((time_us_32() / 1000) - aim_reset_timer_ > 500)
        {
            vmouse_x_ = XboxOG::SB::AIMING_MID;
            vmouse_y_ = XboxOG::SB::AIMING_MID;
        }
    }
    else
    {
        aim_reset_timer_ = time_us_32() / 1000;
    }

    in_report_.aimingX = static_cast<uint16_t>(vmouse_x_);
    in_report_.aimingY = static_cast<uint16_t>(vmouse_y_);

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }
    if (tud_xid::send_report_ready(0) &&
        std::memcmp(&prev_in_report_, &in_report_, sizeof(XboxOG::SB::InReport)) &&
        tud_xid::send_report(0, reinterpret_cast<uint8_t*>(&in_report_), sizeof(XboxOG::SB::InReport)))
    {
        std::memcpy(&prev_in_report_, &in_report_, sizeof(XboxOG::SB::InReport));
    }

    if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_ORANGE))
    {
        uint16_t new_sense = 0;

        if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_9))
        {
            new_sense = 200;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_8))
        {
            new_sense = 250;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_7))
        {
            new_sense = 300;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_6))
        {
            new_sense = 350;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_5))
        {
            new_sense = 400;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_4))
        {
            new_sense = 650;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_3))
        {
            new_sense = 800;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_2))
        {
            new_sense = 1000;
        }
        else if (chatpad_pressed(gp_in_chatpad, XInput::Chatpad::CODE_1))
        {
            new_sense = 1200;
        }

        if (new_sense != 0)
        {
            sensitivity_ = new_sense;
        }
    }

    if (tud_xid::receive_report(0, reinterpret_cast<uint8_t*>(&out_report_), sizeof(XboxOG::SB::OutReport)) &&
        out_report_.bLength == sizeof(XboxOG::SB::OutReport))
    {
        Gamepad::PadOut gp_out;
        gp_out.rumble_l = out_report_.Chaff_Extinguisher;
        gp_out.rumble_l |= out_report_.Chaff_Extinguisher << 4;
        gp_out.rumble_l |= out_report_.Comm1_MagazineChange << 4;
        gp_out.rumble_l |= out_report_.CockpitHatch_EmergencyEject << 4;
        gp_out.rumble_r = gp_out.rumble_l;

        gamepad.set_pad_out(gp_out);
    }
}

uint16_t XboxOGSBDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    std::memcpy(buffer, &in_report_, sizeof(in_report_));
	return sizeof(XboxOG::SB::InReport);
}

void XboxOGSBDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool XboxOGSBDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return tud_xid::class_driver()->control_xfer_cb(rhport, stage, request);
}

const uint16_t* XboxOGSBDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(XboxOG::SB::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* XboxOGSBDevice::get_descriptor_device_cb() 
{
    return reinterpret_cast<const uint8_t*>(&XboxOG::GP::DEVICE_DESCRIPTORS);
}

const uint8_t* XboxOGSBDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t* XboxOGSBDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return XboxOG::SB::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* XboxOGSBDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}