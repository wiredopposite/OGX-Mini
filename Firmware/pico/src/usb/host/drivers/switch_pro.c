#include <string.h>
#include "class/hid/hid.h"
#include "common/usb_def.h"
#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/drivers/hid/hid_usage.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/switch_pro.h"
#include "usb/host/host_private.h"
#include "assert_compat.h"

typedef enum {
    SW_PRO_1_HANDSHAKE = 0,
    SW_PRO_1_DISABLE_TIMEOUT,
    SW_PRO_2_SET_LED,
    SW_PRO_2_SET_HOME_LED,
    SW_PRO_2_ENABLE_FULL_REPORT,
    SW_PRO_2_SET_IMU,
    SW_PRO_2_INIT_DONE,
} switch_pro_init_state_t;

typedef struct {
    switch_pro_init_state_t init_state;
    uint8_t                 index;
    uint8_t                 sequest_counter;
    uint8_t                 buf_out[32U] __attribute__((aligned(4)));
    user_profile_t          profile;
    gamepad_pad_t           gp_report;
    gamepad_pad_t           prev_gp_report;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} switch_pro_state_t;
_STATIC_ASSERT(sizeof(switch_pro_state_t) <= USBH_STATE_BUFFER_SIZE, "switch_pro_state_t size exceeds USBH_EPSIZE_MAX");

static switch_pro_state_t* switch_pro_state[GAMEPADS_MAX] = { NULL };

static inline int16_t normalize_axis(uint16_t value) {
    int32_t normalized_value = (value - 2047) * 22;
    if (normalized_value < R_INT16_MIN) {
        return R_INT16_MIN;
    } else if (normalized_value > R_INT16_MAX) {
        return R_INT16_MAX;
    }
    return (int16_t)normalized_value;
}

static inline uint8_t get_sequence_counter(switch_pro_state_t* sw_pro) {
    uint8_t count = sw_pro->sequest_counter;
    sw_pro->sequest_counter = ((sw_pro->sequest_counter + 1) & 0x0F);
    return count;
}

static void init_2_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len, bool success, void* context) {
    switch_pro_state_t* sw_pro = (switch_pro_state_t*)context;
    switch_pro_report_out_t* report_out = (switch_pro_report_out_t*)sw_pro->buf_out;
    memset(report_out, 0, sizeof(switch_pro_report_out_t));

    report_out->sequence_counter = get_sequence_counter(sw_pro);

    report_out->rumble_l[0] = 0x00;
    report_out->rumble_l[1] = 0x01;
    report_out->rumble_l[2] = 0x40;
    report_out->rumble_l[3] = 0x40;      

    report_out->rumble_r[0] = 0x00;
    report_out->rumble_r[1] = 0x01;
    report_out->rumble_r[2] = 0x40;
    report_out->rumble_r[3] = 0x40;   

    uint8_t count = 0;

    switch (sw_pro->init_state) {
    case SW_PRO_2_SET_LED:
        report_out->report_id = SWITCH_PRO_REPORT_ID_CMD_AND_RUMBLE;
        report_out->commands[count++] = SWITCH_PRO_COMMAND_LED;
        report_out->commands[count++] = sw_pro->index + 1;
        tuh_hxx_send_report_with_cb(daddr, itf_num, (const uint8_t*)report_out, 
                                    sizeof(switch_pro_report_out_t) + count, init_2_cb, sw_pro);
        sw_pro->init_state = SW_PRO_2_SET_HOME_LED;
        break;
    case SW_PRO_2_SET_HOME_LED:
        report_out->report_id = SWITCH_PRO_REPORT_ID_CMD_AND_RUMBLE;
        report_out->commands[count++] = SWITCH_PRO_COMMAND_HOME_LED;
        report_out->commands[count++] = (0 /* Number of cycles */ << 4) | (true ? 0xF : 0);
        report_out->commands[count++] = (0xF /* LED start intensity */ << 4) | 0x0 /* Number of full cycles */;
        report_out->commands[count++] = (0xF /* Mini Cycle 1 LED intensity */ << 4) | 0x0 /* Mini Cycle 2 LED intensity */;
        tuh_hxx_send_report_with_cb(daddr, itf_num, (const uint8_t*)report_out, 
                                    sizeof(switch_pro_report_out_t) + count, init_2_cb, sw_pro);
        sw_pro->init_state = SW_PRO_2_ENABLE_FULL_REPORT;
        break;
    case SW_PRO_2_ENABLE_FULL_REPORT:
        report_out->report_id = SWITCH_PRO_REPORT_ID_CMD_AND_RUMBLE;
        report_out->commands[count++] = SWITCH_PRO_COMMAND_MODE;
        report_out->commands[count++] = SWITCH_PRO_COMMAND_MODE_FULL_REPORT;
        tuh_hxx_send_report_with_cb(daddr, itf_num, (const uint8_t*)report_out, 
                                    sizeof(switch_pro_report_out_t) + count, init_2_cb, sw_pro);
        sw_pro->init_state = SW_PRO_2_SET_IMU;
        break;
    case SW_PRO_2_SET_IMU:
        report_out->report_id = SWITCH_PRO_REPORT_ID_CMD_AND_RUMBLE;
        report_out->commands[count++] = SWITCH_PRO_COMMAND_GYRO;
        report_out->commands[count++] = SWITCH_PRO_COMMAND_GYRO_ENABLE;
        tuh_hxx_send_report_with_cb(daddr, itf_num, (const uint8_t*)report_out, 
                                    sizeof(switch_pro_report_out_t) + count, init_2_cb, sw_pro);
        sw_pro->init_state = SW_PRO_2_INIT_DONE;
        break;
    default:
        usb_host_driver_connect_cb(sw_pro->index, USBH_TYPE_HID_SWITCH_PRO, true);
        tuh_hxx_receive_report(daddr, itf_num);
        break;
    }
}

static void init_1_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len, bool success, void* context) {
    switch_pro_state_t* sw_pro = (switch_pro_state_t*)context;
    switch_pro_command_t* cmd = (switch_pro_command_t*)sw_pro->buf_out;
    cmd->report_id = SWITCH_PRO_REPORT_ID_HID;

    switch (sw_pro->init_state) {
    case SW_PRO_1_HANDSHAKE:
        cmd->command = SWITCH_PRO_COMMAND_HANDSHAKE;
        tuh_hxx_send_report_with_cb(daddr, itf_num, (const uint8_t*)cmd, 
                                    sizeof(switch_pro_command_t), init_1_cb, sw_pro);
        sw_pro->init_state = SW_PRO_1_DISABLE_TIMEOUT;
        break;
    case SW_PRO_1_DISABLE_TIMEOUT:
        cmd->command = SWITCH_PRO_COMMAND_DISABLE_TIMEOUT;
        tuh_hxx_send_report_with_cb(daddr, itf_num, (const uint8_t*)cmd, 
                                    sizeof(switch_pro_command_t), init_2_cb, sw_pro);
        sw_pro->init_state = SW_PRO_2_ENABLE_FULL_REPORT;
        break;
    default:
        break;
    }
}

static void switch_pro_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                               const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;

    switch_pro_state[index] = (switch_pro_state_t*)state_buffer;
    switch_pro_state_t* sw_pro = switch_pro_state[index];
    memset(sw_pro, 0, sizeof(switch_pro_state_t));

    settings_get_profile_by_index(index, &sw_pro->profile);
    sw_pro->map.joy_l = !settings_is_default_joystick(&sw_pro->profile.joystick_l);
    sw_pro->map.joy_r = !settings_is_default_joystick(&sw_pro->profile.joystick_r);
    sw_pro->map.trig_l = !settings_is_default_trigger(&sw_pro->profile.trigger_l);
    sw_pro->map.trig_r = !settings_is_default_trigger(&sw_pro->profile.trigger_r);

    sw_pro->index = index;

    init_1_cb(daddr, itf_num, NULL, 0, true, sw_pro);
}

static void switch_pro_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                       uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;
    switch_pro_state_t* sw_pro = switch_pro_state[index];
    const switch_pro_report_in_t* report_in = (const switch_pro_report_in_t*)data;
    gamepad_pad_t* gp_report = &sw_pro->gp_report;

    memset(gp_report, 0, sizeof(gamepad_pad_t));

    if (report_in->buttons[0] & SWITCH_PRO_BUTTON0_Y)   { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_x); }
    if (report_in->buttons[0] & SWITCH_PRO_BUTTON0_A)   { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_b); }
    if (report_in->buttons[0] & SWITCH_PRO_BUTTON0_B)   { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_a); }
    if (report_in->buttons[0] & SWITCH_PRO_BUTTON0_X)   { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_rb); }
    if (report_in->buttons[0] & SWITCH_PRO_BUTTON0_R)   { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_a); }
    if (report_in->buttons[0] & SWITCH_PRO_BUTTON0_ZR)  { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_rt); }

    if (report_in->buttons[1] & SWITCH_PRO_BUTTON1_MINUS)   { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_back); }
    if (report_in->buttons[1] & SWITCH_PRO_BUTTON1_PLUS)    { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_start); }
    if (report_in->buttons[1] & SWITCH_PRO_BUTTON1_L3)      { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_l3); }
    if (report_in->buttons[1] & SWITCH_PRO_BUTTON1_R3)      { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_r3); }
    if (report_in->buttons[1] & SWITCH_PRO_BUTTON1_HOME)    { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_sys); }
    if (report_in->buttons[1] & SWITCH_PRO_BUTTON1_CAPTURE) { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_misc); }

    if (report_in->buttons[2] & SWITCH_PRO_BUTTON2_DOWN)    { gp_report->dpad |= GP_BIT8(sw_pro->profile.d_down); }
    if (report_in->buttons[2] & SWITCH_PRO_BUTTON2_UP)      { gp_report->dpad |= GP_BIT8(sw_pro->profile.d_up); }
    if (report_in->buttons[2] & SWITCH_PRO_BUTTON2_RIGHT)   { gp_report->dpad |= GP_BIT8(sw_pro->profile.d_right); }
    if (report_in->buttons[2] & SWITCH_PRO_BUTTON2_LEFT)    { gp_report->dpad |= GP_BIT8(sw_pro->profile.d_left); }
    if (report_in->buttons[2] & SWITCH_PRO_BUTTON2_L)       { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_lb); }
    if (report_in->buttons[2] & SWITCH_PRO_BUTTON2_ZL)      { gp_report->buttons |= GP_BIT16(sw_pro->profile.btn_lt); }

    gp_report->trigger_l = ((gp_report->buttons & GAMEPAD_BTN_LT) ? 0xFFU : 0x00U);
    gp_report->trigger_r = ((gp_report->buttons & GAMEPAD_BTN_RT) ? 0xFFU : 0x00U);

    uint16_t joy_lx = report_in->joysticks[0] | ((report_in->joysticks[1] & 0xF) << 8);
    uint16_t joy_ly = (report_in->joysticks[1] >> 4) | (report_in->joysticks[2] << 4);
    uint16_t joy_rx = report_in->joysticks[3] | ((report_in->joysticks[4] & 0xF) << 8);
    uint16_t joy_ry = (report_in->joysticks[4] >> 4) | (report_in->joysticks[5] << 4);

    gp_report->joystick_lx = normalize_axis(joy_lx);
    gp_report->joystick_ly = normalize_axis(joy_ly);
    gp_report->joystick_rx = normalize_axis(joy_rx);
    gp_report->joystick_ry = normalize_axis(joy_ry);

    if (sw_pro->map.joy_l) {
        settings_scale_joysticks(&sw_pro->profile.joystick_l, &gp_report->joystick_lx, &gp_report->joystick_ly, false);
    }
    if (sw_pro->map.joy_r) {
        settings_scale_joysticks(&sw_pro->profile.joystick_r, &gp_report->joystick_rx, &gp_report->joystick_ry, false);
    }
    if (sw_pro->map.trig_l) {
        settings_scale_trigger(&sw_pro->profile.trigger_l, &gp_report->trigger_l);
    }
    if (sw_pro->map.trig_r) {
        settings_scale_trigger(&sw_pro->profile.trigger_r, &gp_report->trigger_r);
    }
    if (memcmp(&sw_pro->prev_gp_report, gp_report, sizeof(gamepad_pad_t)) != 0) {
        usb_host_driver_pad_cb(index, gp_report, GAMEPAD_FLAG_IN_PAD);
        memcpy(&sw_pro->prev_gp_report, gp_report, sizeof(gamepad_pad_t));
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

static void switch_pro_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    switch_pro_state_t* sw_pro = switch_pro_state[index];
    if (sw_pro->init_state < SW_PRO_2_INIT_DONE) {
        return;
    }
    switch_pro_report_out_t* report_out = (switch_pro_report_out_t*)sw_pro->buf_out;
    memset(report_out, 0, sizeof(switch_pro_report_out_t));

    uint8_t report_size = 10;

    report_out->report_id = SWITCH_PRO_REPORT_ID_RUMBLE_ONLY;
    report_out->sequence_counter = get_sequence_counter(sw_pro);

    if (rumble->l > 0) {
        uint8_t amplitude_l = ((rumble->l / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40;
        report_out->rumble_l[0] = amplitude_l;
        report_out->rumble_l[1] = 0x88;
        report_out->rumble_l[2] = amplitude_l / 2;
        report_out->rumble_l[3] = 0x61;  
    } else {
        report_out->rumble_l[0] = 0x00;
        report_out->rumble_l[1] = 0x01;
        report_out->rumble_l[2] = 0x40;
        report_out->rumble_l[3] = 0x40;           
    }

    if (rumble->r > 0) {
        uint8_t amplitude_r = ((rumble->r / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40;
        report_out->rumble_r[0] = amplitude_r;
        report_out->rumble_r[1] = 0x88;
        report_out->rumble_r[2] = amplitude_r / 2;
        report_out->rumble_r[3] = 0x61;
    } else {
        report_out->rumble_r[0] = 0x00;
        report_out->rumble_r[1] = 0x01;
        report_out->rumble_r[2] = 0x40;
        report_out->rumble_r[3] = 0x40;   
    }
    tuh_hxx_send_report(daddr, itf_num, (const uint8_t*)report_out, sizeof(switch_pro_report_out_t));
}

const usb_host_driver_t USBH_DRIVER_SWITCH_PRO = {
    .name = "Switch Pro",
    .mounted_cb = switch_pro_mounted,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = switch_pro_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = switch_pro_send_rumble,
    .send_audio = NULL,
};