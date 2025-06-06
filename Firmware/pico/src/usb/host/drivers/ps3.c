#include <string.h>
#include "class/hid/hid.h"
#include "common/usb_def.h"
#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/ps3.h"
#include "usb/host/host_private.h"
#include "assert_compat.h"

typedef enum {
    PS3_INIT_BT_INFO = 0,
    PS3_INIT_ENABLE_REPORTS,
    PS3_INIT_ENABLE_SENSORS,
    PS3_INIT_SET_LED,
    PS3_INIT_DONE,
} ps3_init_state_t;

typedef struct {
    uint8_t                 index;
    ps3_init_state_t        init_state;
    ps3_report_out_t        report_out;
    user_profile_t          profile;
    gamepad_pad_t           gp_report;
    gamepad_pad_t           prev_gp_report;
    tusb_control_request_t  rumble_req;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} ps3_state_t;
_STATIC_ASSERT(sizeof(ps3_state_t) <= USBH_STATE_BUFFER_SIZE, "ps3_state_t size exceeds USBH_EPSIZE_MAX");

static ps3_state_t* ps3_state[GAMEPADS_MAX] = { NULL };

static void ps3_init_cb(uint8_t daddr, const uint8_t* data, 
                        uint16_t len, bool success, void* context) {
    ps3_state_t* ps3 = (ps3_state_t*)context;
    tusb_control_request_t request = {
        .bmRequestType = 0 | USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE,
        .bRequest = 0,
        .wValue = (HID_REPORT_TYPE_FEATURE << 8),
        .wIndex = 0,
        .wLength = 0,
    };

    switch (ps3->init_state) {
    case PS3_INIT_BT_INFO:
        request.bmRequestType |= USB_REQ_DIR_DEVTOHOST;
        request.bRequest = HID_REQ_CONTROL_GET_REPORT;
        request.wValue |= PS3_REQ_FEATURE_REPORT_ID_BT_INFO;
        request.wLength = sizeof(ps3_bt_info_t);
        /* If we ever end up using this, move to device state */
        static ps3_bt_info_t bt_info = {0};
        tuh_hxx_ctrl_xfer(daddr, &request, (uint8_t*)&bt_info, ps3_init_cb, ps3);
        ps3->init_state = PS3_INIT_ENABLE_REPORTS;
        break;
    case PS3_INIT_ENABLE_REPORTS:
        {
        request.bmRequestType |= USB_REQ_DIR_HOSTTODEV;
        request.bRequest = HID_REQ_CONTROL_SET_REPORT;
        request.wValue |= PS3_REQ_FEATURE_REPORT_ID_COMMAND;
        request.wIndex = 0x0000; 
        request.wLength = sizeof(ps3_cmd_header_t);
        ps3_cmd_header_t cmd_header = {0};
        cmd_header.report_id = PS3_COMMAND_REPORT_ID;
        cmd_header.command = PS3_COMMAND_INPUT_ENABLE;
        tuh_hxx_ctrl_xfer(daddr, &request, (uint8_t*)&cmd_header, ps3_init_cb, ps3);
        ps3->init_state = PS3_INIT_ENABLE_SENSORS;
        }
        break;
    case PS3_INIT_ENABLE_SENSORS:
        {
        request.bmRequestType |= USB_REQ_DIR_HOSTTODEV;
        request.bRequest = HID_REQ_CONTROL_SET_REPORT;
        request.wValue |= PS3_REQ_FEATURE_REPORT_ID_COMMAND;
        request.wIndex = 0x0000; 
        request.wLength = sizeof(ps3_cmd_header_t);
        ps3_cmd_header_t cmd_header = {0};
        cmd_header.report_id = PS3_COMMAND_REPORT_ID;
        cmd_header.command = PS3_COMMAND_SENSORS_ENABLE;
        tuh_hxx_ctrl_xfer(daddr, &request, (uint8_t*)&cmd_header, ps3_init_cb, ps3);
        ps3->init_state = PS3_INIT_SET_LED;
        }
        break;
    case PS3_INIT_SET_LED:
        tuh_hxx_ctrl_xfer(daddr, &ps3->rumble_req, (uint8_t*)&ps3->report_out, ps3_init_cb, ps3);
        ps3->init_state = PS3_INIT_DONE;
        break;
    default:
        usb_host_driver_connect_cb(ps3->index, USBH_TYPE_HID_PS3, true);
        tuh_hxx_receive_report(daddr, 0);
        break;
    }
}

static void ps3_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                        const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;

    ps3_state[index] = (ps3_state_t*)state_buffer;
    ps3_state_t* ps3 = ps3_state[index];
    memset(ps3, 0, sizeof(ps3_state_t));

    ps3->index = index;

    settings_get_profile_by_index(index, &ps3->profile);
    ps3->map.joy_l = !settings_is_default_joystick(&ps3->profile.joystick_l);
    ps3->map.joy_r = !settings_is_default_joystick(&ps3->profile.joystick_r);
    ps3->map.trig_l = !settings_is_default_trigger(&ps3->profile.trigger_l);
    ps3->map.trig_r = !settings_is_default_trigger(&ps3->profile.trigger_r);

    ps3->report_out.report_id = PS3_REPORT_ID_OUT;
    for (uint8_t i = 0; i < 4; i++) {
        ps3->report_out.leds[i].time_enabled  = 0xFF;
        ps3->report_out.leds[i].duty_length   = 0x27;
        ps3->report_out.leds[i].enabled       = 0x10;
        ps3->report_out.leds[i].duty_off      = 0x00;
        ps3->report_out.leds[i].duty_on       = 0x32;
    }
    ps3->report_out.leds_bitmap = 1U << (index + 1);

    ps3->rumble_req.bmRequestType = USB_REQ_DIR_HOSTTODEV | USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE;
    ps3->rumble_req.bRequest = HID_REQ_CONTROL_SET_REPORT;
    ps3->rumble_req.wValue = (HID_REPORT_TYPE_OUTPUT << 8) | PS3_REQ_OUTPUT_REPORT_ID;
    ps3->rumble_req.wIndex = 0x0000;
    ps3->rumble_req.wLength = sizeof(ps3_report_out_t);

    ps3_init_cb(daddr, NULL, 0, true, ps3);
}

static void ps3_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;
    ps3_state_t* ps3 = ps3_state[index];
    ps3_sixaxis_report_in_t* report_in = (ps3_sixaxis_report_in_t*)data;
    gamepad_pad_t* gp_report = &ps3->gp_report;

    if ((len < PS3_REPORT_IN_SIZE) || (report_in->report_id != PS3_REPORT_ID_IN)) {
        tuh_hxx_receive_report(daddr, itf_num);
        return;
    }
    memset(&ps3->gp_report, 0, sizeof(gamepad_pad_t));

    if (report_in->buttons[0] & PS3_BTN0_DPAD_UP)       { gp_report->dpad |= GP_BIT8(ps3->profile.d_up); }
    if (report_in->buttons[0] & PS3_BTN0_DPAD_DOWN)     { gp_report->dpad |= GP_BIT8(ps3->profile.d_down); }
    if (report_in->buttons[0] & PS3_BTN0_DPAD_LEFT)     { gp_report->dpad |= GP_BIT8(ps3->profile.d_left); }
    if (report_in->buttons[0] & PS3_BTN0_DPAD_RIGHT)    { gp_report->dpad |= GP_BIT8(ps3->profile.d_right); }

    if (report_in->buttons[0] & PS3_BTN0_SELECT)   { gp_report->buttons |= GP_BIT16(ps3->profile.btn_back); }
    if (report_in->buttons[0] & PS3_BTN0_START)    { gp_report->buttons |= GP_BIT16(ps3->profile.btn_start); }
    if (report_in->buttons[0] & PS3_BTN0_L3)       { gp_report->buttons |= GP_BIT16(ps3->profile.btn_l3); }
    if (report_in->buttons[0] & PS3_BTN0_R3)       { gp_report->buttons |= GP_BIT16(ps3->profile.btn_r3); }

    if (report_in->buttons[1] & PS3_BTN1_SQUARE)   { gp_report->buttons |= GP_BIT16(ps3->profile.btn_x); }
    if (report_in->buttons[1] & PS3_BTN1_CIRCLE)   { gp_report->buttons |= GP_BIT16(ps3->profile.btn_b); }
    if (report_in->buttons[1] & PS3_BTN1_CROSS)    { gp_report->buttons |= GP_BIT16(ps3->profile.btn_a); }
    if (report_in->buttons[1] & PS3_BTN1_TRIANGLE) { gp_report->buttons |= GP_BIT16(ps3->profile.btn_y); }
    if (report_in->buttons[1] & PS3_BTN1_L1)       { gp_report->buttons |= GP_BIT16(ps3->profile.btn_lb); }
    if (report_in->buttons[1] & PS3_BTN1_R1)       { gp_report->buttons |= GP_BIT16(ps3->profile.btn_rb); }
    if (report_in->buttons[2] & PS3_BTN2_SYS)      { gp_report->buttons |= GP_BIT16(ps3->profile.btn_sys); }

    gp_report->analog[ps3->profile.a_a]        = report_in->a_cross;
    gp_report->analog[ps3->profile.a_b]        = report_in->a_circle;
    gp_report->analog[ps3->profile.a_x]        = report_in->a_square;
    gp_report->analog[ps3->profile.a_y]        = report_in->a_triangle;
    gp_report->analog[ps3->profile.a_lb]       = report_in->a_l1;
    gp_report->analog[ps3->profile.a_rb]       = report_in->a_r1;
    gp_report->analog[ps3->profile.a_up]       = report_in->a_up;
    gp_report->analog[ps3->profile.a_down]     = report_in->a_down;
    gp_report->analog[ps3->profile.a_left]     = report_in->a_left;
    gp_report->analog[ps3->profile.a_right]    = report_in->a_right;

    gp_report->trigger_l = report_in->a_l2;
    gp_report->trigger_r = report_in->a_r2;

    gp_report->joystick_lx = range_uint8_to_int16(report_in->joystick_lx);
    gp_report->joystick_ly = range_uint8_to_int16(report_in->joystick_ly);
    gp_report->joystick_rx = range_uint8_to_int16(report_in->joystick_rx);
    gp_report->joystick_ry = range_uint8_to_int16(report_in->joystick_ry);

    if (ps3->map.trig_l) {
        settings_scale_trigger(&ps3->profile.trigger_l, &gp_report->trigger_l);
    }
    if (ps3->map.trig_r) {
        settings_scale_trigger(&ps3->profile.trigger_r, &gp_report->trigger_r);
    }
    if (ps3->map.joy_l) {
        settings_scale_joysticks(&ps3->profile.joystick_l, &gp_report->joystick_lx, &gp_report->joystick_ly, false);
    }
    if (ps3->map.joy_r) {
        settings_scale_joysticks(&ps3->profile.joystick_r, &gp_report->joystick_rx, &gp_report->joystick_ry, false);
    }

    if (memcmp(&ps3->prev_gp_report, gp_report, sizeof(gamepad_pad_t)) != 0) {
        usb_host_driver_pad_cb(index, gp_report, GAMEPAD_FLAG_IN_PAD | GAMEPAD_FLAG_IN_PAD_ANALOG);
        memcpy(&ps3->prev_gp_report, gp_report, sizeof(gamepad_pad_t));
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

static void ps3_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    ps3_state_t* ps3 = ps3_state[index];
    if (ps3->init_state < PS3_INIT_DONE) {
        return;
    }
    ps3->report_out.rumble.r_duration = (rumble->r_duration == 0) ? 20 : rumble->r_duration;
    ps3->report_out.rumble.r_on       = (rumble->r > 0) ? 1 : 0;
    ps3->report_out.rumble.l_duration = (rumble->l_duration == 0) ? 20 : rumble->l_duration;
    ps3->report_out.rumble.l_force    = rumble->l;
    tuh_hxx_ctrl_xfer(daddr, &ps3->rumble_req, (uint8_t*)&ps3->report_out, NULL, NULL);
}

const usb_host_driver_t USBH_DRIVER_PS3 = {
    .name = "PS3",
    .mounted_cb = ps3_mounted,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = ps3_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = ps3_send_rumble,
    .send_audio = NULL,
};