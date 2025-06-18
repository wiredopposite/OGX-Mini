#include <string.h>
#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/xboxog_gp.h"
#include "usb/host/host_private.h"

typedef struct {
    xboxog_gp_report_in_t   prev_report_in;
    xboxog_gp_report_out_t  report_out;
    user_profile_t          profile;
    gamepad_pad_t           gp_report;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} xid_gp_state_t;
_Static_assert(sizeof(xid_gp_state_t) <= USBH_STATE_BUFFER_SIZE, "xid_gp_state_t size exceeds USBH_EPSIZE_MAX");

static xid_gp_state_t* xid_gp_state[GAMEPADS_MAX] = { NULL };

static void xid_gp_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                           const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;

    xid_gp_state[index] = (xid_gp_state_t*)state_buffer;
    xid_gp_state_t* state = xid_gp_state[index];
    memset(state, 0, sizeof(xid_gp_state_t));

    settings_get_profile_by_index(index, &state->profile);
    state->map.joy_l = !settings_is_default_joystick(&state->profile.joystick_l);
    state->map.joy_r = !settings_is_default_joystick(&state->profile.joystick_r);
    state->map.trig_l = !settings_is_default_trigger(&state->profile.trigger_l);
    state->map.trig_r = !settings_is_default_trigger(&state->profile.trigger_r);

    usb_host_driver_connect_cb(index, type, true);
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xid_gp_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                 uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;  

    xid_gp_state_t* state = xid_gp_state[index];
    const xboxog_gp_report_in_t* report = (const xboxog_gp_report_in_t*)data;

    if ((len < sizeof(xboxog_gp_report_in_t)) || 
        (memcmp(report, &state->prev_report_in, sizeof(xboxog_gp_report_in_t)) == 0)) {
        tuh_hxx_receive_report(daddr, itf_num);
        return;
    }
    memset(&state->gp_report, 0, sizeof(gamepad_pad_t));

    state->gp_report.flags = GAMEPAD_FLAG_PAD | GAMEPAD_FLAG_ANALOG;

    if (report->buttons & XBOXOG_GP_BUTTON_UP)      { state->gp_report.buttons |= GP_BIT(state->profile.btn_up); }
    if (report->buttons & XBOXOG_GP_BUTTON_DOWN)    { state->gp_report.buttons |= GP_BIT(state->profile.btn_down); }
    if (report->buttons & XBOXOG_GP_BUTTON_LEFT)    { state->gp_report.buttons |= GP_BIT(state->profile.btn_left); }
    if (report->buttons & XBOXOG_GP_BUTTON_RIGHT)   { state->gp_report.buttons |= GP_BIT(state->profile.btn_right); }
    if (report->buttons & XBOXOG_GP_BUTTON_START)   { state->gp_report.buttons |= GP_BIT(state->profile.btn_start); }
    if (report->buttons & XBOXOG_GP_BUTTON_BACK)    { state->gp_report.buttons |= GP_BIT(state->profile.btn_back); }
    if (report->buttons & XBOXOG_GP_BUTTON_L3)      { state->gp_report.buttons |= GP_BIT(state->profile.btn_l3); }
    if (report->buttons & XBOXOG_GP_BUTTON_R3)      { state->gp_report.buttons |= GP_BIT(state->profile.btn_r3); }

    state->gp_report.buttons |= (report->a ? GP_BIT(state->profile.btn_a) : 0U);
    state->gp_report.buttons |= (report->b ? GP_BIT(state->profile.btn_b) : 0U);
    state->gp_report.buttons |= (report->x ? GP_BIT(state->profile.btn_x) : 0U);
    state->gp_report.buttons |= (report->y ? GP_BIT(state->profile.btn_y) : 0U);
    state->gp_report.buttons |= (report->white ? GP_BIT(state->profile.btn_lb) : 0U);
    state->gp_report.buttons |= (report->black ? GP_BIT(state->profile.btn_rb) : 0U);

    state->gp_report.analog[state->profile.a_a]     = report->a;
    state->gp_report.analog[state->profile.a_b]     = report->b;
    state->gp_report.analog[state->profile.a_x]     = report->x;
    state->gp_report.analog[state->profile.a_y]     = report->y;
    state->gp_report.analog[state->profile.a_lb]    = report->white;
    state->gp_report.analog[state->profile.a_rb]    = report->black;
    state->gp_report.analog[state->profile.a_up]    = (report->buttons & XBOXOG_GP_BUTTON_UP)    ? 0xFFU : 0U;
    state->gp_report.analog[state->profile.a_down]  = (report->buttons & XBOXOG_GP_BUTTON_DOWN)  ? 0xFFU : 0U;
    state->gp_report.analog[state->profile.a_left]  = (report->buttons & XBOXOG_GP_BUTTON_LEFT)  ? 0xFFU : 0U;
    state->gp_report.analog[state->profile.a_right] = (report->buttons & XBOXOG_GP_BUTTON_RIGHT) ? 0xFFU : 0U;

    state->gp_report.trigger_l = report->trigger_l;
    state->gp_report.trigger_r = report->trigger_r;

    state->gp_report.joystick_lx = report->joystick_lx;
    state->gp_report.joystick_ly = report->joystick_ly;
    state->gp_report.joystick_rx = report->joystick_rx;
    state->gp_report.joystick_ry = report->joystick_ry;

    if (state->map.trig_l) {
        settings_scale_trigger(&state->profile.trigger_l, &state->gp_report.trigger_l);
    }
    if (state->map.trig_r) {
        settings_scale_trigger(&state->profile.trigger_r, &state->gp_report.trigger_r);
    }
    if (state->map.joy_l) {
        settings_scale_joysticks(&state->profile.joystick_l, &state->gp_report.joystick_lx, 
                                 &state->gp_report.joystick_ly);
    }
    if (state->map.joy_r) {
        settings_scale_joysticks(&state->profile.joystick_r, &state->gp_report.joystick_rx, 
                                 &state->gp_report.joystick_ry);
    }

    usb_host_driver_pad_cb(index, &state->gp_report);
    memcpy(&state->prev_report_in, report, sizeof(xboxog_gp_report_in_t));
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xid_gp_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    xboxog_gp_report_out_t* report_out = &xid_gp_state[index]->report_out;
    report_out->report_id = 0;
    report_out->length = sizeof(xboxog_gp_report_out_t);
    report_out->rumble_l = range_uint8_to_uint16(rumble->l);
    report_out->rumble_r = range_uint8_to_uint16(rumble->r);
    tuh_hxx_send_report(daddr, itf_num, (uint8_t*)report_out, sizeof(xboxog_gp_report_out_t));
}

const usb_host_driver_t USBH_DRIVER_XBOXOG_GP = {
    .name = "Xbox OG Gamepad",
    .mounted_cb = xid_gp_mounted,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = xid_gp_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = xid_gp_send_rumble,
    .send_audio = NULL,
};