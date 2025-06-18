#include <string.h>
#include "common/usb_def.h"
#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/drivers/hid/hid_usage.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/ps5.h"
#include "usb/host/host_private.h"

typedef struct {
    ps5_report_out_t        report_out;
    user_profile_t          profile;
    gamepad_pad_t           gp_report;
    gamepad_pad_t           prev_gp_report;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} ps5_state_t;
_Static_assert(sizeof(ps5_state_t) <= USBH_STATE_BUFFER_SIZE, "ps5_state_t size exceeds USBH_EPSIZE_MAX");

static ps5_state_t* ps5_state[GAMEPADS_MAX] = { NULL };

static void ps5_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                        const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;

    ps5_state[index] = (ps5_state_t*)state_buffer;
    ps5_state_t* state = ps5_state[index];
    memset(state, 0, sizeof(ps5_state_t));

    settings_get_profile_by_index(index, &state->profile);

    state->map.joy_l = !settings_is_default_joystick(&state->profile.joystick_l);
    state->map.joy_r = !settings_is_default_joystick(&state->profile.joystick_r);
    state->map.trig_l = !settings_is_default_trigger(&state->profile.trigger_l);
    state->map.trig_r = !settings_is_default_trigger(&state->profile.trigger_r);

    state->report_out.report_id = PS5_REPORT_ID_OUT_CTRL;
    state->report_out.control_flag[0] = 2;
    state->report_out.control_flag[1] = 2;
    state->report_out.led_control_flag = 0x01 | 0x02;
    state->report_out.pulse_option = 1;
    state->report_out.led_brightness = 0xFF;
    state->report_out.player_number = index + 1;
    state->report_out.lightbar_blue = 0xFF;

    tuh_hxx_send_report(daddr, itf_num, (const uint8_t*)&state->report_out, sizeof(ps5_report_out_t));
    usb_host_driver_connect_cb(index, type, true);
    tuh_hxx_receive_report(daddr, itf_num);
}

static void ps5_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;
    ps5_state_t* ps5 = ps5_state[index];
    const ps5_report_in_t* report_in = (const ps5_report_in_t*)data;
    gamepad_pad_t* gp_report = &ps5->gp_report;

    memset(gp_report, 0, sizeof(gamepad_pad_t));

    gp_report->flags = GAMEPAD_FLAG_PAD;

    switch (report_in->hat) {
    case HID_HAT_UP:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_up);
        break;
    case HID_HAT_UP_RIGHT:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_up) | GP_BIT(ps5->profile.btn_right);
        break;
    case HID_HAT_RIGHT:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_right);
        break;
    case HID_HAT_DOWN_RIGHT:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_down) | GP_BIT(ps5->profile.btn_right);
        break;
    case HID_HAT_DOWN:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_down);
        break;
    case HID_HAT_DOWN_LEFT:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_down) | GP_BIT(ps5->profile.btn_left);
        break;
    case HID_HAT_LEFT:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_left);
        break;
    case HID_HAT_UP_LEFT:
        gp_report->buttons |= GP_BIT(ps5->profile.btn_up) | GP_BIT(ps5->profile.btn_left);
        break;
    default:
        break;
    }

    if (report_in->buttons0 & PS5_BUTTON0_SQUARE)       { gp_report->buttons |= GP_BIT(ps5->profile.btn_x); }
    if (report_in->buttons0 & PS5_BUTTON0_CIRCLE)       { gp_report->buttons |= GP_BIT(ps5->profile.btn_b); }
    if (report_in->buttons0 & PS5_BUTTON0_CROSS)        { gp_report->buttons |= GP_BIT(ps5->profile.btn_a); }
    if (report_in->buttons0 & PS5_BUTTON0_TRIANGLE)     { gp_report->buttons |= GP_BIT(ps5->profile.btn_y); }
    if (report_in->buttons1 & PS5_BUTTON1_L1)           { gp_report->buttons |= GP_BIT(ps5->profile.btn_lb); }
    if (report_in->buttons1 & PS5_BUTTON1_R1)           { gp_report->buttons |= GP_BIT(ps5->profile.btn_rb); }
    if (report_in->buttons1 & PS5_BUTTON1_L2)           { gp_report->buttons |= GP_BIT(ps5->profile.btn_lt); }
    if (report_in->buttons1 & PS5_BUTTON1_R2)           { gp_report->buttons |= GP_BIT(ps5->profile.btn_rt); }
    if (report_in->buttons1 & PS5_BUTTON1_SHARE)        { gp_report->buttons |= GP_BIT(ps5->profile.btn_back); }
    if (report_in->buttons1 & PS5_BUTTON1_OPTIONS)      { gp_report->buttons |= GP_BIT(ps5->profile.btn_start); }
    if (report_in->buttons1 & PS5_BUTTON1_L3)           { gp_report->buttons |= GP_BIT(ps5->profile.btn_l3); }
    if (report_in->buttons1 & PS5_BUTTON1_R3)           { gp_report->buttons |= GP_BIT(ps5->profile.btn_r3); }
    if (report_in->buttons2 & PS5_BUTTON2_SYS)          { gp_report->buttons |= GP_BIT(ps5->profile.btn_sys); }
    if (report_in->buttons2 & PS5_BUTTON2_TP)           { gp_report->buttons |= GP_BIT(ps5->profile.btn_misc); }

    gp_report->trigger_l = report_in->trigger_l;
    gp_report->trigger_r = report_in->trigger_r;

    gp_report->joystick_lx = range_uint8_to_int16(report_in->joystick_lx);
    gp_report->joystick_ly = range_invert_int16(range_uint8_to_int16(report_in->joystick_ly));
    gp_report->joystick_rx = range_uint8_to_int16(report_in->joystick_rx);
    gp_report->joystick_ry = range_invert_int16(range_uint8_to_int16(report_in->joystick_ry));

    if (memcmp(&ps5->prev_gp_report, gp_report, sizeof(gamepad_pad_t)) == 0) {
        tuh_hxx_receive_report(daddr, itf_num);
        return;
    }
    memcpy(&ps5->prev_gp_report, gp_report, sizeof(gamepad_pad_t));

    if (ps5->map.joy_l) {
        settings_scale_joysticks(&ps5->profile.joystick_l, &gp_report->joystick_lx, &gp_report->joystick_ly);
    }
    if (ps5->map.joy_r) {
        settings_scale_joysticks(&ps5->profile.joystick_r, &gp_report->joystick_rx, &gp_report->joystick_ry);
    }
    if (ps5->map.trig_l) {
        settings_scale_trigger(&ps5->profile.trigger_l, &gp_report->trigger_l);
    }
    if (ps5->map.trig_r) {
        settings_scale_trigger(&ps5->profile.trigger_r, &gp_report->trigger_r);
    }

    usb_host_driver_pad_cb(index, gp_report);
    tuh_hxx_receive_report(daddr, itf_num);
}

static void ps5_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    ps5_state_t* ps5 = ps5_state[index];
    ps5->report_out.report_id = PS5_REPORT_ID_OUT_RUMBLE;
    ps5->report_out.motor_left  = rumble->l;
    ps5->report_out.motor_right = rumble->r;
    tuh_hxx_send_report(daddr, itf_num, (const uint8_t*)&ps5->report_out, sizeof(ps5_report_out_t));
}

const usb_host_driver_t USBH_DRIVER_PS5 = {
    .name = "PS5",
    .mounted_cb = ps5_mounted,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = ps5_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = ps5_send_rumble,
    .send_audio = NULL,
};