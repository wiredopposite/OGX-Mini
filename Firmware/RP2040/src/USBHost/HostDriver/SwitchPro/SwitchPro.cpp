#include <cstring>
#include <array>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/SwitchPro/SwitchPro.h"

void SwitchProHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    std::memset(&out_report_, 0, sizeof(out_report_));
    state_.handshake_sent = send_handshake(address, instance);
    tuh_hid_receive_report(address, instance);
}

bool SwitchProHost::send_handshake(uint8_t address, uint8_t instance)
{
    std::array<uint8_t, 2> handshake = { SwitchPro::CMD::HID, SwitchPro::CMD::HANDSHAKE };
    return tuh_hid_send_report(address, instance, 0, handshake.data(), handshake.size());
}

bool SwitchProHost::disable_timeout(uint8_t address, uint8_t instance)
{
    std::array<uint8_t, 2> timeout = { SwitchPro::CMD::HID, SwitchPro::CMD::DISABLE_TIMEOUT };
    return tuh_hid_send_report(address, instance, 0, timeout.data(), timeout.size());
}

uint8_t SwitchProHost::get_output_sequence_counter()
{
    uint8_t counter = state_.sequence_counter;
    state_.sequence_counter = (state_.sequence_counter + 1) & 0x0F;
    return counter;
}

void SwitchProHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    if (!state_.handshake_sent)
    {
        return;
    }
    else if (!state_.timeout_disabled)
    {
        state_.timeout_disabled = disable_timeout(address, instance);
        tuh_hid_receive_report(address, instance);
        return;
    }

    const SwitchPro::InReport* in_report = reinterpret_cast<const SwitchPro::InReport*>(report);

    if (std::memcmp(&prev_in_report_.buttons, in_report->buttons, 9) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    if (in_report->buttons[0] & SwitchPro::Buttons0::Y)  gamepad.set_button_x();    
    if (in_report->buttons[0] & SwitchPro::Buttons0::B)  gamepad.set_button_a();
    if (in_report->buttons[0] & SwitchPro::Buttons0::A)  gamepad.set_button_b();
    if (in_report->buttons[0] & SwitchPro::Buttons0::X)  gamepad.set_button_y();
    if (in_report->buttons[2] & SwitchPro::Buttons2::L)  gamepad.set_button_lb();
    if (in_report->buttons[0] & SwitchPro::Buttons0::R)  gamepad.set_button_rb();
    if (in_report->buttons[1] & SwitchPro::Buttons1::L3) gamepad.set_button_l3();
    if (in_report->buttons[1] & SwitchPro::Buttons1::R3) gamepad.set_button_r3();
    if (in_report->buttons[1] & SwitchPro::Buttons1::MINUS)     gamepad.set_button_back();
    if (in_report->buttons[1] & SwitchPro::Buttons1::PLUS)      gamepad.set_button_start();
    if (in_report->buttons[1] & SwitchPro::Buttons1::HOME)      gamepad.set_button_sys();
    if (in_report->buttons[1] & SwitchPro::Buttons1::CAPTURE)   gamepad.set_button_misc();

    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_UP)    gamepad.set_dpad_up();
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_DOWN)  gamepad.set_dpad_down();
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_LEFT)  gamepad.set_dpad_left();
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_RIGHT) gamepad.set_dpad_right();
    
    gamepad.set_trigger_l(in_report->buttons[2] & SwitchPro::Buttons2::ZL ? UINT_8::MAX : UINT_8::MIN);
    gamepad.set_trigger_r(in_report->buttons[0] & SwitchPro::Buttons0::ZR ? UINT_8::MAX : UINT_8::MIN);

    int16_t joy_lx = normalize_axis(in_report->joysticks[0] | ((in_report->joysticks[1] & 0xF) << 8));
    int16_t joy_ly = normalize_axis((in_report->joysticks[1] >> 4) | (in_report->joysticks[2] << 4));
    int16_t joy_rx = normalize_axis(in_report->joysticks[3] | ((in_report->joysticks[4] & 0xF) << 8));
    int16_t joy_ry = normalize_axis((in_report->joysticks[4] >> 4) | (in_report->joysticks[5] << 4));

    gamepad.set_joystick_lx(joy_lx);
    gamepad.set_joystick_ly(joy_ly, true);
    gamepad.set_joystick_rx(joy_rx);
    gamepad.set_joystick_ry(joy_ry, true);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(SwitchPro::InReport));
}

bool SwitchProHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    if (!state_.handshake_sent)
    {
        state_.handshake_sent = send_handshake(address, instance);
    }
    else if (!state_.timeout_disabled)
    {
        return false;
    }

    // See: https://github.com/Dan611/hid-procon
    //      https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering
    //      https://github.com/HisashiKato/USB_Host_Shield_Library_2.0

    std::memset(&out_report_, 0, sizeof(out_report_));

    uint8_t report_size = 10;

    out_report_.command = SwitchPro::CMD::RUMBLE_ONLY;
    out_report_.sequence_counter = get_output_sequence_counter();

    uint8_t gp_rumble_l = gamepad.get_rumble_l().uint8();
    uint8_t gp_rumble_r = gamepad.get_rumble_r().uint8();

    if (gp_rumble_l > 0) 
    {
        uint8_t amplitude_l = static_cast<uint8_t>(((gp_rumble_l / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        out_report_.rumble_l[0] = amplitude_l;
        out_report_.rumble_l[1] = 0x88;
        out_report_.rumble_l[2] = amplitude_l / 2;
        out_report_.rumble_l[3] = 0x61;  
    } 
    else 
    {
        out_report_.rumble_l[0] = 0x00;
        out_report_.rumble_l[1] = 0x01;
        out_report_.rumble_l[2] = 0x40;
        out_report_.rumble_l[3] = 0x40;           
    }

    if (gp_rumble_r > 0) 
    {
        uint8_t amplitude_r = static_cast<uint8_t>(((gp_rumble_r / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        out_report_.rumble_r[0] = amplitude_r;
        out_report_.rumble_r[1] = 0x88;
        out_report_.rumble_r[2] = amplitude_r / 2;
        out_report_.rumble_r[3] = 0x61;
    } 
    else 
    {
        out_report_.rumble_r[0] = 0x00;
        out_report_.rumble_r[1] = 0x01;
        out_report_.rumble_r[2] = 0x40;
        out_report_.rumble_r[3] = 0x40;   
    }

    if (!state_.commands_sent)
    {
        if (!state_.led_set)
        {
            report_size = 12;

            out_report_.command = SwitchPro::CMD::AND_RUMBLE;
            out_report_.sub_command = SwitchPro::CMD::LED;
            out_report_.sub_command_args[0] = idx_ + 1;

            state_.led_set = tuh_hid_send_report(address, instance, 0, &out_report_, report_size);

            return state_.led_set;
        }
        else if (!state_.led_home_set)
        {
            report_size = 14;

            out_report_.command = SwitchPro::CMD::AND_RUMBLE;
            out_report_.sub_command = SwitchPro::CMD::LED_HOME;
            out_report_.sub_command_args[0] = (0 /* Number of cycles */ << 4) | (true ? 0xF : 0);
            out_report_.sub_command_args[1] = (0xF /* LED start intensity */ << 4) | 0x0 /* Number of full cycles */;
            out_report_.sub_command_args[2] = (0xF /* Mini Cycle 1 LED intensity */ << 4) | 0x0 /* Mini Cycle 2 LED intensity */;

            state_.led_home_set = tuh_hid_send_report(address, instance, 0, &out_report_, report_size);
            
            return state_.led_home_set;
        }
        else if (!state_.full_report_enabled)
        {
            report_size = 12;

            out_report_.command = SwitchPro::CMD::AND_RUMBLE;
            out_report_.sub_command = SwitchPro::CMD::MODE;
            out_report_.sub_command_args[0] = SwitchPro::CMD::FULL_REPORT_MODE;
            
            state_.full_report_enabled = tuh_hid_send_report(address, instance, 0, &out_report_, report_size);
            
            return state_.full_report_enabled;
        }
        else if (!state_.imu_enabled)
        {
            report_size = 12;
            
            out_report_.command = SwitchPro::CMD::AND_RUMBLE;
            out_report_.sub_command = SwitchPro::CMD::GYRO;
            out_report_.sub_command_args[0] = 1 ? 1 : 0;
            
            state_.imu_enabled = tuh_hid_send_report(address, instance, 0, &out_report_, report_size);
            
            return state_.imu_enabled;
        }
        else
        {
            state_.commands_sent = true;
        }
    }

    return tuh_hid_send_report(address, instance, 0, &out_report_, report_size);
}