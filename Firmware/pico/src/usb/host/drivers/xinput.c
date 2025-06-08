#include <string.h>
#include <pico/time.h>
#include "tusb.h"
#include "class/hid/hid.h"

#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/xinput.h"
#include "usb/host/host_private.h"

// typedef enum {
//     XINPUT_INIT_GET_INFO = 0,
//     XINPUT_INIT_ENABLE_RUMBLE,
//     XINPUT_INIT_SET_LED,
//     XINPUT_INIT_ENABLE_CHATPAD,
//     XINPUT_INIT_ENABLE_CHATPAD_LEDS,
//     XINPUT_INIT_CHATPAD_KEEPALIVE_1,
//     XINPUT_INIT_DONE,
// } init_state_t;

// typedef enum {
//     CHAT_KEEPALIVE_1 = 0,
//     CHAT_KEEPALIVE_2,
// } chat_ka_state_t;

typedef struct {
    uint8_t             index;
    // init_state_t        init_state;
    // chat_ka_state_t     chat_ka_state;
    uint8_t             buf_out[32U] __attribute__((aligned(4)));
    user_profile_t      profile;
    gamepad_pad_t       gp_report;
    gamepad_pad_t       prev_gp_report;
    // volatile uint8_t    chat_keepalive_count;
    // volatile bool       chat_keepalive_pending;
    // repeating_timer_t   chatpad_timer;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} xinput_state_t;
_Static_assert(sizeof(xinput_state_t) <= USBH_STATE_BUFFER_SIZE, "xinput_state_t size exceeds USBH_EPSIZE_MAX");

static xinput_state_t* xin_state[GAMEPADS_MAX] = { NULL };

static void xinput_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                              const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;
    xin_state[index] = (xinput_state_t*)state_buffer;
    xinput_state_t* xin = xin_state[index];
    memset(xin, 0, sizeof(xinput_state_t));

    xin->index = index;

    settings_get_profile_by_index(index, &xin->profile);
    xin->map.joy_l = !settings_is_default_joystick(&xin->profile.joystick_l);
    xin->map.joy_r = !settings_is_default_joystick(&xin->profile.joystick_r);
    xin->map.trig_l = !settings_is_default_trigger(&xin->profile.trigger_l);
    xin->map.trig_r = !settings_is_default_trigger(&xin->profile.trigger_r);

    xinput_report_out_t* report_out = (xinput_report_out_t*)xin->buf_out;
    report_out->report_id = XINPUT_REPORT_ID_OUT_LED;
    report_out->length = 3;
    report_out->led = (index + 1) + 5;
    tuh_hxx_send_report(daddr, itf_num, xin->buf_out, sizeof(xinput_report_out_t));
    usb_host_driver_connect_cb(index, type, true);
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xinput_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                   uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;

    xinput_state_t* xin = xin_state[index];
    gamepad_pad_t* gp_report = &xin->gp_report;
    const xinput_report_in_t* report_in = (const xinput_report_in_t*)data;
    memset(gp_report, 0, sizeof(gamepad_pad_t));

    if (report_in->buttons & XINPUT_BUTTON_UP)    { gp_report->dpad |= GP_BIT8(xin->profile.d_up); }
    if (report_in->buttons & XINPUT_BUTTON_DOWN)  { gp_report->dpad |= GP_BIT8(xin->profile.d_down); }
    if (report_in->buttons & XINPUT_BUTTON_LEFT)  { gp_report->dpad |= GP_BIT8(xin->profile.d_left); }
    if (report_in->buttons & XINPUT_BUTTON_RIGHT) { gp_report->dpad |= GP_BIT8(xin->profile.d_right); }

    if (report_in->buttons & XINPUT_BUTTON_START)  { gp_report->buttons |= GP_BIT16(xin->profile.btn_start); }
    if (report_in->buttons & XINPUT_BUTTON_BACK)   { gp_report->buttons |= GP_BIT16(xin->profile.btn_back); }
    if (report_in->buttons & XINPUT_BUTTON_L3)     { gp_report->buttons |= GP_BIT16(xin->profile.btn_l3); }
    if (report_in->buttons & XINPUT_BUTTON_R3)     { gp_report->buttons |= GP_BIT16(xin->profile.btn_r3); }
    if (report_in->buttons & XINPUT_BUTTON_LB)     { gp_report->buttons |= GP_BIT16(xin->profile.btn_lb); }
    if (report_in->buttons & XINPUT_BUTTON_RB)     { gp_report->buttons |= GP_BIT16(xin->profile.btn_rb); }
    if (report_in->buttons & XINPUT_BUTTON_HOME)   { gp_report->buttons |= GP_BIT16(xin->profile.btn_sys); }
    if (report_in->buttons & XINPUT_BUTTON_A)      { gp_report->buttons |= GP_BIT16(xin->profile.btn_a); }
    if (report_in->buttons & XINPUT_BUTTON_B)      { gp_report->buttons |= GP_BIT16(xin->profile.btn_b); }
    if (report_in->buttons & XINPUT_BUTTON_X)      { gp_report->buttons |= GP_BIT16(xin->profile.btn_x); }
    if (report_in->buttons & XINPUT_BUTTON_Y)      { gp_report->buttons |= GP_BIT16(xin->profile.btn_y); }

    gp_report->trigger_l = report_in->trigger_l;
    gp_report->trigger_r = report_in->trigger_r;

    gp_report->joystick_lx = report_in->joystick_lx;
    gp_report->joystick_ly = report_in->joystick_ly;
    gp_report->joystick_rx = report_in->joystick_rx;
    gp_report->joystick_ry = report_in->joystick_ry;

    if (xin->map.joy_l) {
        settings_scale_joysticks(&xin->profile.joystick_l, &gp_report->joystick_lx, 
                                 &gp_report->joystick_ly);
    }
    if (xin->map.joy_r) {
        settings_scale_joysticks(&xin->profile.joystick_r, &gp_report->joystick_rx, 
                                 &gp_report->joystick_ry);
    }
    if (xin->map.trig_l) {
        settings_scale_trigger(&xin->profile.trigger_l, &gp_report->trigger_l);
    }
    if (xin->map.trig_r) {
        settings_scale_trigger(&xin->profile.trigger_r, &gp_report->trigger_r);
    }
    if (memcmp(&xin->prev_gp_report, gp_report, sizeof(gamepad_pad_t)) != 0) {
        usb_host_driver_pad_cb(index, gp_report, 0);
        memcpy(&xin->prev_gp_report, gp_report, sizeof(gamepad_pad_t));
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xinput_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    xinput_state_t* xin = xin_state[index];
    xinput_report_out_t* report_out = (xinput_report_out_t*)xin->buf_out;
    memset(report_out, 0, sizeof(xinput_report_out_t));
    report_out->report_id = XINPUT_REPORT_ID_OUT_RUMBLE;
    report_out->length = sizeof(xinput_report_out_t);
    report_out->rumble_l = (rumble->l / 255U) * 100U;
    report_out->rumble_r = (rumble->r / 255U) * 100U;
    tuh_hxx_send_report(daddr, itf_num, xin->buf_out, sizeof(xinput_report_out_t));
}

const usb_host_driver_t USBH_DRIVER_XINPUT = {
    .name = "XInput",
    .mounted_cb = xinput_mounted,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = xinput_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = xinput_send_rumble,
    .send_audio = NULL,
};