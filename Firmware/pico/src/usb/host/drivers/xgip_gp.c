#include <string.h>
#include "host/usbh.h"

#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/xgip.h"
#include "usb/host/host_private.h"
#include "assert_compat.h"

typedef enum {
    GIP_INIT_0 = 0,
    GIP_INIT_1,
    GIP_INIT_2,
    GIP_INIT_3,
    GIP_INIT_DONE,
} xgip_init_state_t;

typedef struct {
    uint8_t             index;
    xgip_init_state_t   init_state; 
    xgip_report_in_t    prev_report_in;
    uint8_t             buf_out[64U] __attribute__((aligned(4)));
    user_profile_t      profile;
    gamepad_pad_t       gp_report;
    uint8_t             rumble_seq_id;
    uint8_t             audio_seq_id;
    uint8_t             state_seq_id;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} xgip_state_t;
_STATIC_ASSERT(sizeof(xgip_state_t) <= USBH_STATE_BUFFER_SIZE, "xgip_state_t size exceeds USBH_EPSIZE_MAX");

static xgip_state_t* xgip_state[GAMEPADS_MAX] = { NULL };

static inline void increment_seq_id(uint8_t* seq_id) {
    if (*seq_id < 0xFFU) {
        (*seq_id) += 1;
    } else {
        (*seq_id) = 1;
    }
}

static void xgip_init_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, 
                          uint16_t len, bool success, void* context) {
    xgip_state_t* xgip = (xgip_state_t*)context;
    xgip_cmd_set_state_t* cmd = (xgip_cmd_set_state_t*)xgip->buf_out;
    memset(cmd, 0, sizeof(xgip_cmd_set_state_t));
    increment_seq_id(&xgip->state_seq_id);
    printf("XGIP Init Callback: daddr=%u, itf_num=%u, success=%d, len=%u\n", 
           daddr, itf_num, success, len);

    switch (xgip->init_state) {
    case GIP_INIT_0:
        cmd->header.type.raw = XGIP_COMMAND_SET_DEVICE_STATE;
        cmd->header.flags.system_msg = 1;
        cmd->header.sequence_id = 0;
        cmd->payload_len.length = 1;
        cmd->state = XGIP_STATE_START;
        tuh_hxx_send_report_with_cb(daddr, itf_num, (const uint8_t*)cmd, 
                                    sizeof(xgip_cmd_set_state_t), xgip_init_cb, xgip);
        xgip->init_state = GIP_INIT_1;
        break;
    case GIP_INIT_1:
        tuh_hxx_send_report_with_cb(daddr, itf_num, GIP_S_INIT, 
                                    sizeof(GIP_S_INIT), xgip_init_cb, xgip);
        xgip->init_state = GIP_INIT_2;
        break;
    case GIP_INIT_2:
        {
        uint16_t vid, pid;
        tuh_vid_pid_get(daddr, &vid, &pid);
        if (vid == 0x045e) {
            tuh_hxx_send_report_with_cb(daddr, itf_num, GIP_EXTRA_INPUT_PACKET_INIT, 
                                        sizeof(GIP_EXTRA_INPUT_PACKET_INIT), xgip_init_cb, xgip);
            xgip->init_state = GIP_INIT_DONE;
            break;
        } else if (vid == 0x0e6f) {
            tuh_hxx_send_report_with_cb(daddr, itf_num, GIP_PDP_LED_ON, 
                                        sizeof(GIP_PDP_LED_ON), xgip_init_cb, xgip);
            xgip->init_state = GIP_INIT_3;
            break;
        }
        }
    case GIP_INIT_3:
        tuh_hxx_send_report_with_cb(daddr, itf_num, GIP_PDP_AUTH, 
                                    sizeof(GIP_PDP_AUTH), xgip_init_cb, xgip);
        xgip->init_state = GIP_INIT_DONE;
        break;
    default:
        usb_host_driver_connect_cb(xgip->index, USBH_TYPE_XGIP, true);
        tuh_hxx_receive_report(daddr, itf_num);
        break;
    }
}

static void xgip_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                         const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;
    xgip_state[index] = (xgip_state_t*)state_buffer;
    xgip_state_t* state = xgip_state[index];
    memset(state, 0, sizeof(xgip_state_t));

    state->index = index;

    settings_get_profile_by_index(index, &state->profile);
    state->map.joy_l = !settings_is_default_joystick(&state->profile.joystick_l);
    state->map.joy_r = !settings_is_default_joystick(&state->profile.joystick_r);
    state->map.trig_l = !settings_is_default_trigger(&state->profile.trigger_l);
    state->map.trig_r = !settings_is_default_trigger(&state->profile.trigger_r);

    xgip_init_cb(daddr, itf_num, NULL, 0, true, state);
}

static void xgip_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                 uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;

    xgip_state_t* xgip = xgip_state[index];
    xgip_header_t* header = (xgip_header_t*)data;
    printf("XGIP Report Received: type=0x%02x, len=%u\n", header->type.raw, len);

    switch (header->type.raw) {
    case XGIP_LOW_LATENCY_GAMEPAD_INPUT:
    case XGIP_LOW_LATENCY_GAMEPAD_EXT_INPUT:
        {
        xgip_report_in_t* report = (xgip_report_in_t*)data;
        if ((len < sizeof(xgip_report_in_t)) || 
            (memcmp(report + 4, &xgip->prev_report_in + 4, sizeof(xgip_report_in_t) - 4) == 0)) {
            break;
        }
        memset(&xgip->gp_report, 0, sizeof(gamepad_pad_t));

        if (report->buttons[1] & XGIP_BUTTON1_DPAD_DOWN)  { xgip->gp_report.dpad |= GP_BIT8(xgip->profile.d_down); }
        if (report->buttons[1] & XGIP_BUTTON1_DPAD_UP)    { xgip->gp_report.dpad |= GP_BIT8(xgip->profile.d_up); }
        if (report->buttons[1] & XGIP_BUTTON1_DPAD_LEFT)  { xgip->gp_report.dpad |= GP_BIT8(xgip->profile.d_left); }
        if (report->buttons[1] & XGIP_BUTTON1_DPAD_RIGHT) { xgip->gp_report.dpad |= GP_BIT8(xgip->profile.d_right); }

        if (report->buttons[0] & XGIP_BUTTON0_MENU) { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_start); }
        if (report->buttons[0] & XGIP_BUTTON0_VIEW) { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_back); }
        if (report->buttons[0] & XGIP_BUTTON0_A)    { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_a); }
        if (report->buttons[0] & XGIP_BUTTON0_B)    { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_b); }
        if (report->buttons[0] & XGIP_BUTTON0_X)    { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_x); }
        if (report->buttons[0] & XGIP_BUTTON0_Y)    { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_y); }
        if (report->buttons[1] & XGIP_BUTTON1_L3)   { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_l3); }
        if (report->buttons[1] & XGIP_BUTTON1_R3)   { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_r3); }
        if (report->buttons[1] & XGIP_BUTTON1_LB)   { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_lb); }
        if (report->buttons[1] & XGIP_BUTTON1_RB)   { xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_rb); }

        xgip->gp_report.trigger_l = range_uint10_to_uint8(report->trigger_l);
        xgip->gp_report.trigger_r = range_uint10_to_uint8(report->trigger_r);
        
        if (xgip->map.trig_l) {
            settings_scale_trigger(&xgip->profile.trigger_l, &xgip->gp_report.trigger_l);
        }
        if (xgip->map.trig_r) {
            settings_scale_trigger(&xgip->profile.trigger_r, &xgip->gp_report.trigger_r);
        }

        xgip->gp_report.joystick_lx = report->joystick_lx;
        xgip->gp_report.joystick_ly = range_invert_int16(report->joystick_ly);
        xgip->gp_report.joystick_rx = report->joystick_rx;
        xgip->gp_report.joystick_ry = range_invert_int16(report->joystick_ry);

        if (xgip->map.joy_l) {
            settings_scale_joysticks(&xgip->profile.joystick_l, &xgip->gp_report.joystick_lx, 
                                     &xgip->gp_report.joystick_ly, true);
        }
        if (xgip->map.joy_r) {
            settings_scale_joysticks(&xgip->profile.joystick_r, &xgip->gp_report.joystick_rx, 
                                     &xgip->gp_report.joystick_ry, true);
        }

        usb_host_driver_pad_cb(index, &xgip->gp_report, GAMEPAD_FLAG_IN_PAD);
        memcpy(&xgip->prev_report_in, report, sizeof(xgip_report_in_t));
        }
        break;
    case XGIP_COMMAND_GUIDE_BTN_STATUS:
        {
        xgip_guide_button_in_t* guide_btn = (xgip_guide_button_in_t*)data;
        if ((guide_btn->state) && !(xgip->gp_report.buttons & GP_BIT16(xgip->profile.btn_sys))) {
            xgip->gp_report.buttons |= GP_BIT16(xgip->profile.btn_sys);
            usb_host_driver_pad_cb(index, &xgip->gp_report, 0);
        } else if ((!guide_btn->state) && (xgip->gp_report.buttons & GP_BIT16(xgip->profile.btn_sys))) {
            xgip->gp_report.buttons &= ~GP_BIT16(xgip->profile.btn_sys);
            usb_host_driver_pad_cb(index, &xgip->gp_report, 0);
        }
        }
        break;
    // case XGIP_COMMAND_STATUS_DEVICE:
    case XGIP_COMMAND_HELLO_DEVICE:
        xgip->init_state = GIP_INIT_0;
        xgip_init_cb(daddr, itf_num, NULL, 0, true, xgip);
        break;
    default:
        break;
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xgip_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    xgip_state_t* xgip = (xgip_state_t*)xgip_state[index];
    xgip_cmd_vibration_t* vib = (xgip_cmd_vibration_t*)xgip->buf_out;
    increment_seq_id(&xgip->rumble_seq_id);

    memset(vib, 0, sizeof(xgip_cmd_vibration_t));
    bool rumble_on = (rumble->l > 0) || (rumble->r > 0);
    bool duration = (((rumble->l_duration > 0) || (rumble->r_duration > 0)) && rumble_on);

    vib->header.type.raw        = XGIP_COMMAND_GAMEPAD_VIBRATION;
    vib->header.sequence_id     = xgip->rumble_seq_id;
    vib->payload_len.raw        = sizeof(xgip_cmd_vibration_t) - 4;
    vib->motor.right            = (rumble->r > 0) ? 1 : 0;
    vib->motor.left             = (rumble->l > 0) ? 1 : 0;
    // vib->motor.left_impulse     = vib->motor.left;
    // vib->motor.right_impulse    = vib->motor.right;
    vib->vibration_left         = rumble->l / 0xFFU * 100U;
    vib->vibration_right        = rumble->r / 0xFFU * 100U;
    vib->duration_ms            = (duration ? MAX(rumble->l_duration, rumble->r_duration) : (rumble_on ? 100U : 0U));
    tuh_hxx_send_report(daddr, itf_num, xgip_state[index]->buf_out, sizeof(xgip_cmd_vibration_t));
}

const usb_host_driver_t USBH_DRIVER_XGIP_GP = {
    .name = "XGIP",
    .mounted_cb = xgip_mounted,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = xgip_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = xgip_send_rumble,
    .send_audio = NULL,
};