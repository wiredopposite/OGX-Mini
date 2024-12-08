#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/PS3/PS3.h"

const tusb_control_request_t PS3Host::RUMBLE_REQUEST = 
{
    .bmRequestType = 0x21,
    .bRequest = 0x09, // SET_REPORT
    .wValue = 0x0201,
    .wIndex = 0x0000, 
    .wLength = sizeof(PS3::OutReport)
};

void PS3Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    gamepad.set_analog_enabled(true);
    
    std::memcpy(&out_report_, PS3::DEFAULT_OUT_REPORT, std::min(sizeof(PS3::OutReport), sizeof(PS3::DEFAULT_OUT_REPORT)));

    out_report_.leds_bitmap = 0x1 << (idx_ + 1);
    out_report_.led[idx_].time_enabled = 0xFF;

    init_state_.out_report = &out_report_;
    init_state_.dev_addr = address;
    init_state_.init_buffer.fill(0);

    tusb_control_request_t init_request =
    {
        .bmRequestType = 0xA1,
        .bRequest = 0x01, // GET_REPORT
        .wValue = (HID_REPORT_TYPE_FEATURE << 8) | 0xF2,
        .wIndex = 0x0000,
        .wLength = 17
    };

    send_control_xfer(address, &init_request, init_state_.init_buffer.data(), get_report_complete_cb, reinterpret_cast<uintptr_t>(&init_state_));

    tuh_hid_receive_report(address, instance);
}

bool PS3Host::send_control_xfer(uint8_t dev_addr, const tusb_control_request_t* request, uint8_t* buffer, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
    tuh_xfer_s transfer = 
    {
        .daddr = dev_addr,
        .ep_addr = 0x00,
        .setup = request, 
        .buffer = buffer,
        .complete_cb = complete_cb, 
        .user_data = user_data
    };
    return tuh_control_xfer(&transfer);
}

void PS3Host::get_report_complete_cb(tuh_xfer_s *xfer)
{
    InitState* init_state = reinterpret_cast<InitState*>(xfer->user_data);
    if (init_state == nullptr || init_state->stage == InitStage::DONE)
    {
        return;
    }

    tusb_control_request_t init_request =
    {
        .bmRequestType = 0xA1,
        .bRequest = 0x01, // GET_REPORT
        .wValue = (HID_REPORT_TYPE_FEATURE << 8) | 0xF2,
        .wIndex = 0x0000,
        .wLength = 17
    };

    switch (init_state->stage)
    {
        case InitStage::RESP1:
            init_state->stage = InitStage::RESP2;
            send_control_xfer(init_state->dev_addr, &init_request, init_state->init_buffer.data(), get_report_complete_cb, xfer->user_data);
            break;
        case InitStage::RESP2:
            init_state->stage = InitStage::RESP3;
            init_request.wLength = 8;
            send_control_xfer(init_state->dev_addr, &init_request, init_state->init_buffer.data(), get_report_complete_cb, xfer->user_data);
            break;
        case InitStage::RESP3:
            init_state->stage = InitStage::DONE;
            init_state->reports_enabled = true;
            send_control_xfer(init_state->dev_addr, &PS3Host::RUMBLE_REQUEST, reinterpret_cast<uint8_t*>(init_state->out_report), nullptr, 0);
            break;
        default:
            break;
    }
}

void PS3Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
    const PS3::InReport* in_report = reinterpret_cast<const PS3::InReport*>(report);
    if (std::memcmp(&prev_in_report_, in_report, std::min(static_cast<size_t>(len), static_cast<size_t>(26))) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    gamepad.reset_buttons();

    if (in_report->buttons[0] & PS3::Buttons0::DPAD_UP)    gamepad.set_dpad_up();
    if (in_report->buttons[0] & PS3::Buttons0::DPAD_DOWN)  gamepad.set_dpad_down();
    if (in_report->buttons[0] & PS3::Buttons0::DPAD_LEFT)  gamepad.set_dpad_left();
    if (in_report->buttons[0] & PS3::Buttons0::DPAD_RIGHT) gamepad.set_dpad_right();

    if (in_report->buttons[0] & PS3::Buttons0::SELECT)      gamepad.set_button_back();
    if (in_report->buttons[0] & PS3::Buttons0::START)       gamepad.set_button_start();
    if (in_report->buttons[0] & PS3::Buttons0::L3)          gamepad.set_button_l3();
    if (in_report->buttons[0] & PS3::Buttons0::R3)          gamepad.set_button_r3();
    if (in_report->buttons[1] & PS3::Buttons1::L1)          gamepad.set_button_lb();
    if (in_report->buttons[1] & PS3::Buttons1::R1)          gamepad.set_button_rb();
    if (in_report->buttons[1] & PS3::Buttons1::TRIANGLE)    gamepad.set_button_y();
    if (in_report->buttons[1] & PS3::Buttons1::CIRCLE)      gamepad.set_button_b();
    if (in_report->buttons[1] & PS3::Buttons1::CROSS)       gamepad.set_button_a();
    if (in_report->buttons[1] & PS3::Buttons1::SQUARE)      gamepad.set_button_x(); 
    if (in_report->buttons[2] & PS3::Buttons2::PS)          gamepad.set_button_sys();

    if (gamepad.analog_enabled())
    {
        gamepad.set_analog_up(in_report->up_axis);
        gamepad.set_analog_down(in_report->down_axis);
        gamepad.set_analog_left(in_report->left_axis);
        gamepad.set_analog_right(in_report->right_axis);
        gamepad.set_analog_a(in_report->cross_axis);
        gamepad.set_analog_b(in_report->circle_axis);
        gamepad.set_analog_x(in_report->square_axis);
        gamepad.set_analog_y(in_report->triangle_axis);
        gamepad.set_analog_lb(in_report->l1_axis);
        gamepad.set_analog_rb(in_report->r1_axis);
    }

    gamepad.set_trigger_l(in_report->l2_axis);
    gamepad.set_trigger_r(in_report->r2_axis);

    gamepad.set_joystick_lx(in_report->joystick_lx);
    gamepad.set_joystick_ly(in_report->joystick_ly);
    gamepad.set_joystick_rx(in_report->joystick_rx);
    gamepad.set_joystick_ry(in_report->joystick_ry);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(PS3::InReport));
}

bool PS3Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    static uint32_t last_rumble_ms = 0;
    uint32_t current_ms = time_us_32() / 1000;
    
    if (init_state_.reports_enabled &&
        current_ms - last_rumble_ms >= 300)
    {
        uint8_t rumble_l = gamepad.get_rumble_l().uint8();
        uint8_t rumble_r = gamepad.get_rumble_r().uint8();

        out_report_.rumble.right_duration    = (rumble_r > 0) ? 20 : 0;
        out_report_.rumble.right_motor_on    = (rumble_r > 0) ? 1  : 0;

        out_report_.rumble.left_duration     = (rumble_l > 0) ? 20 : 0;
        out_report_.rumble.left_motor_force  = rumble_l;

        last_rumble_ms = current_ms;

        return send_control_xfer(address, &PS3Host::RUMBLE_REQUEST, reinterpret_cast<uint8_t*>(&out_report_), nullptr, 0);
    }
    return true;
}