#include <string.h>
#include "class/hid/hid.h"
#include "common/usb_def.h"
#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/drivers/hid/hid_usage.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/ps4.h"
#include "usb/host/host_private.h"
#include "assert_compat.h"

typedef struct {
    ps4_report_out_t        report_out;
    user_profile_t          profile;
    gamepad_pad_t           gp_report;
    gamepad_pad_t           prev_gp_report;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} ps4_state_t;
_STATIC_ASSERT(sizeof(ps4_state_t) <= USBH_STATE_BUFFER_SIZE, "ps4_state_t size exceeds USBH_EPSIZE_MAX");

static ps4_state_t* ps4_state[GAMEPADS_MAX] = { NULL };

static void ps4_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num,
                        const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;

    ps4_state[index] = (ps4_state_t*)state_buffer;
    ps4_state_t* state = ps4_state[index];
    memset(state, 0, sizeof(ps4_state_t));

    settings_get_profile_by_index(index, &state->profile);
    state->map.joy_l = !settings_is_default_joystick(&state->profile.joystick_l);
    state->map.joy_r = !settings_is_default_joystick(&state->profile.joystick_r);
    state->map.trig_l = !settings_is_default_trigger(&state->profile.trigger_l);
    state->map.trig_r = !settings_is_default_trigger(&state->profile.trigger_r);

    state->report_out.report_id = PS4_REPORT_ID_OUT;
    state->report_out.flags.led = 1;
    state->report_out.lightbar_blue = 0xFF / 2;

    tuh_hxx_send_report(daddr, itf_num, (const uint8_t*)&state->report_out, sizeof(ps4_report_out_t));
    usb_host_driver_connect_cb(index, type, true);
    tuh_hxx_receive_report(daddr, itf_num);
}

static void ps4_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;
    ps4_state_t* ps4 = ps4_state[index];
    const ps4_report_in_t* report_in = (const ps4_report_in_t*)data;
    gamepad_pad_t* gp_report = &ps4->gp_report;

    memset(gp_report, 0, sizeof(gamepad_pad_t));

    switch (report_in->hat) {
    case HID_HAT_UP:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_up);
        break;
    case HID_HAT_UP_RIGHT:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_up) | GP_BIT8(ps4->profile.d_right);
        break;
    case HID_HAT_RIGHT:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_right);
        break;
    case HID_HAT_DOWN_RIGHT:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_down) | GP_BIT8(ps4->profile.d_right);
        break;
    case HID_HAT_DOWN:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_down);
        break;
    case HID_HAT_DOWN_LEFT:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_down) | GP_BIT8(ps4->profile.d_left);
        break;
    case HID_HAT_LEFT:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_left);
        break;
    case HID_HAT_UP_LEFT:
        gp_report->dpad |= GP_BIT8(ps4->profile.d_up) | GP_BIT8(ps4->profile.d_left);
        break;
    default:
        break;
    }

    if (report_in->buttons0 & PS4_BUTTON0_SQUARE)       { gp_report->buttons |= GP_BIT16(ps4->profile.btn_x); }
    if (report_in->buttons0 & PS4_BUTTON0_CIRCLE)       { gp_report->buttons |= GP_BIT16(ps4->profile.btn_b); }
    if (report_in->buttons0 & PS4_BUTTON0_CROSS)        { gp_report->buttons |= GP_BIT16(ps4->profile.btn_a); }
    if (report_in->buttons0 & PS4_BUTTON0_TRIANGLE)     { gp_report->buttons |= GP_BIT16(ps4->profile.btn_y); }
    if (report_in->buttons1 & PS4_BUTTON1_L1)           { gp_report->buttons |= GP_BIT16(ps4->profile.btn_lb); }
    if (report_in->buttons1 & PS4_BUTTON1_R1)           { gp_report->buttons |= GP_BIT16(ps4->profile.btn_rb); }
    if (report_in->buttons1 & PS4_BUTTON1_L2)           { gp_report->buttons |= GP_BIT16(ps4->profile.btn_lt); }
    if (report_in->buttons1 & PS4_BUTTON1_R2)           { gp_report->buttons |= GP_BIT16(ps4->profile.btn_rt); }
    if (report_in->buttons1 & PS4_BUTTON1_SHARE)        { gp_report->buttons |= GP_BIT16(ps4->profile.btn_back); }
    if (report_in->buttons1 & PS4_BUTTON1_OPTIONS)      { gp_report->buttons |= GP_BIT16(ps4->profile.btn_start); }
    if (report_in->buttons1 & PS4_BUTTON1_L3)           { gp_report->buttons |= GP_BIT16(ps4->profile.btn_l3); }
    if (report_in->buttons1 & PS4_BUTTON1_R3)           { gp_report->buttons |= GP_BIT16(ps4->profile.btn_r3); }
    if (report_in->buttons2 & PS4_BUTTON2_SYS)          { gp_report->buttons |= GP_BIT16(ps4->profile.btn_sys); }
    if (report_in->buttons2 & PS4_BUTTON2_TP)           { gp_report->buttons |= GP_BIT16(ps4->profile.btn_misc); }

    gp_report->trigger_l = report_in->trigger_l;
    gp_report->trigger_r = report_in->trigger_r;

    gp_report->joystick_lx = range_uint8_to_int16(report_in->joystick_lx);
    gp_report->joystick_ly = range_uint8_to_int16(report_in->joystick_ly);
    gp_report->joystick_rx = range_uint8_to_int16(report_in->joystick_rx);
    gp_report->joystick_ry = range_uint8_to_int16(report_in->joystick_ry);

    if (ps4->map.joy_l) {
        settings_scale_joysticks(&ps4->profile.joystick_l, &gp_report->joystick_lx, &gp_report->joystick_ly, false);
    }
    if (ps4->map.joy_r) {
        settings_scale_joysticks(&ps4->profile.joystick_r, &gp_report->joystick_rx, &gp_report->joystick_ry, false);
    }
    if (ps4->map.trig_l) {
        settings_scale_trigger(&ps4->profile.trigger_l, &gp_report->trigger_l);
    }
    if (ps4->map.trig_r) {
        settings_scale_trigger(&ps4->profile.trigger_r, &gp_report->trigger_r);
    }
    if (memcmp(&ps4->prev_gp_report, gp_report, sizeof(gamepad_pad_t)) != 0) {
        usb_host_driver_pad_cb(index, gp_report, GAMEPAD_FLAG_IN_PAD);
        memcpy(&ps4->prev_gp_report, gp_report, sizeof(gamepad_pad_t));
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

static void ps4_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    ps4_state_t* ps4 = ps4_state[index];
    ps4->report_out.motor_left  = rumble->l;
    ps4->report_out.motor_right = rumble->r;
    ps4->report_out.flags.rumble = (rumble->l || rumble->r) ? 1 : 0;
    tuh_hxx_send_report(daddr, itf_num, (const uint8_t*)&ps4->report_out, sizeof(ps4_report_out_t));
}

const usb_host_driver_t USBH_DRIVER_PS4 = {
    .name = "PS4",
    .mounted_cb = ps4_mounted,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = ps4_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = ps4_send_rumble,
    .send_audio = NULL,
};