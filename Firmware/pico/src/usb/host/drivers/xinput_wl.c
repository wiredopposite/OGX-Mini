#include <string.h>
#include <pico/time.h>
// #include "tusb.h"
// #include "class/hid/hid.h"

#include "gamepad/gamepad.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/descriptors/xinput.h"
#include "usb/host/host_private.h"

#define CHATPAD_KEEPALIVE_US  ((uint32_t)1*1000*1000)
#define CHATPAD_KEEPALIVE_MS  (CHATPAD_KEEPALIVE_US / 1000U)

/* Gamepad and chatpad are on the same interface for Xinput wireless */

static const uint8_t CMD_INQUIRE_PRESENT[]  = { 0x08, 0x00, 0x0F, 0xC0 };
static const uint8_t CMD_LED[]              = { 0x00, 0x00, 0x08, 0x40 };
//Sending 0x00, 0x00, 0x08, 0x00 will permanently disable rumble until you do this:
static const uint8_t CMD_RUMBLE_ENABLE[]    = { 0x00, 0x00, 0x08, 0x01 };
static const uint8_t CMD_RUMBLE[]           = { 0x00, 0x01, 0x0F, 0xC0, 0x00, 0x00, 0x00 };
static const uint8_t CMD_CONTROLLER_INFO[]  = { 0x00, 0x00, 0x00, 0x40 };
static const uint8_t CMD_UNKNOWN[]          = { 0x00, 0x00, 0x02, 0x80 };
static const uint8_t CMD_POWER_OFF[]        = { 0x00, 0x00, 0x08, 0xC0 };

static const uint8_t CMD_CHATPAD_INIT[]             = { 0x00, 0x00, 0x0C, 0x1B };
static const uint8_t CMD_CHATPAD_KEEPALIVE_1[]      = { 0x00, 0x00, 0x0C, 0x1F };
static const uint8_t CMD_CHATPAD_KEEPALIVE_2[]      = { 0x00, 0x00, 0x0C, 0x1E };
static const uint8_t CMD_CHATPAD_LEDS_ON_KEYPRESS[] = { 0x00, 0x00, 0x0C, 0x1B };

#define LED_CAPSLOCK    ((uint8_t)0x20)
#define LED_GREEN       ((uint8_t)0x08)
#define LED_ORANGE      ((uint8_t)0x10)
#define LED_MESSENGER   ((uint8_t)0x01)
static const uint8_t CMD_CHATPAD_LED_CTRL[] = { 0x00, 0x00, 0x0C, 0x00 };
static const uint8_t CMD_CHATPAF_LED_MOD[]  = { LED_CAPSLOCK, LED_GREEN, LED_ORANGE, LED_MESSENGER };
static const uint8_t CMD_CHATPAD_LED_ON[]   = { 0x08, 0x09, 0x0A, 0x0B };
static const uint8_t CMD_CHATPAD_LED_OFF[]  = { 0x00, 0x01, 0x02, 0x03 };

typedef enum {
    XINPUT_INIT_GET_INFO = 0,
    XINPUT_INIT_ENABLE_RUMBLE,
    XINPUT_INIT_SET_LED,
    XINPUT_INIT_ENABLE_CHATPAD,
    XINPUT_INIT_ENABLE_CHATPAD_LEDS,
    XINPUT_INIT_CHATPAD_KEEPALIVE_1,
    XINPUT_INIT_DONE,
} init_state_t;

typedef enum {
    CHAT_KEEPALIVE_1 = 0,
    CHAT_KEEPALIVE_2,
} chat_ka_state_t;

typedef struct {
    uint8_t             index;
    bool                connected;
    init_state_t        init_state;
    chat_ka_state_t     chat_ka_state;
    uint8_t             buf_out[32U] __attribute__((aligned(4)));
    user_profile_t      profile;
    gamepad_pad_t       gp_report;
    gamepad_pad_t       prev_gp_report;
    volatile uint8_t    chat_keepalive_count;
    volatile bool       chat_keepalive_pending;
    repeating_timer_t   chatpad_timer;
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} xinput_wl_state_t;
_Static_assert(sizeof(xinput_wl_state_t) <= USBH_STATE_BUFFER_SIZE, "xinput_wl_state_t size exceeds USBH_EPSIZE_MAX");

static xinput_wl_state_t* xin_wl_state[GAMEPADS_MAX] = { NULL };

static bool keepalive_cb(repeating_timer_t *rt) {
    xinput_wl_state_t* xin_wl = (xinput_wl_state_t*)rt->user_data;
    xin_wl->chat_keepalive_pending = true;
    return true;
}

static void xinput_wl_init_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, 
                              uint16_t len, bool success, void* context) {
    (void)data;
    (void)len;
    xinput_wl_state_t* xin_wl = (xinput_wl_state_t*)context;
    // tuh_task();

    switch (xin_wl->init_state) {
        case XINPUT_INIT_GET_INFO:
            tuh_hxx_send_report_with_cb(daddr, itf_num, CMD_CONTROLLER_INFO, 
                                        sizeof(CMD_CONTROLLER_INFO), xinput_wl_init_cb, xin_wl);
            xin_wl->init_state = XINPUT_INIT_ENABLE_RUMBLE;
            break;
        case XINPUT_INIT_ENABLE_RUMBLE:
            tuh_hxx_send_report_with_cb(daddr, itf_num, CMD_RUMBLE_ENABLE, 
                                        sizeof(CMD_RUMBLE_ENABLE), xinput_wl_init_cb, xin_wl);
            xin_wl->init_state = XINPUT_INIT_SET_LED;
            break;
        case XINPUT_INIT_SET_LED:
            {
            uint8_t led[] = { 0x00, 0x00, 0x08, 0x40 };
            led[3] |= ((xin_wl->index + 1) + 5);
            tuh_hxx_send_report_with_cb(daddr, itf_num, led, sizeof(led), 
                                        xinput_wl_init_cb, xin_wl);
            xin_wl->init_state = XINPUT_INIT_ENABLE_CHATPAD;
            }
            break;
        case XINPUT_INIT_ENABLE_CHATPAD:
            tuh_hxx_send_report_with_cb(daddr, itf_num, CMD_CHATPAD_INIT, 
                                        sizeof(CMD_CHATPAD_INIT), xinput_wl_init_cb, xin_wl);
            xin_wl->init_state = XINPUT_INIT_ENABLE_CHATPAD_LEDS;
            break;
        case XINPUT_INIT_ENABLE_CHATPAD_LEDS:
            {
            uint8_t led_ctrl[4] = { 0 };
            led_ctrl[2] = LED_CAPSLOCK;
            tuh_hxx_send_report_with_cb(daddr, itf_num, led_ctrl, 
                                        sizeof(led_ctrl), xinput_wl_init_cb, xin_wl);
            }
            xin_wl->init_state = XINPUT_INIT_DONE;
            break;
        default:
            break;
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xinput_wl_mounted(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                              const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    (void)desc_report;
    (void)desc_len;
    xin_wl_state[index] = (xinput_wl_state_t*)state_buffer;
    xinput_wl_state_t* xin_wl = xin_wl_state[index];
    memset(xin_wl, 0, sizeof(xinput_wl_state_t));

    xin_wl->index = index;

    settings_get_profile_by_index(index, &xin_wl->profile);
    xin_wl->map.joy_l = !settings_is_default_joystick(&xin_wl->profile.joystick_l);
    xin_wl->map.joy_r = !settings_is_default_joystick(&xin_wl->profile.joystick_r);
    xin_wl->map.trig_l = !settings_is_default_trigger(&xin_wl->profile.trigger_l);
    xin_wl->map.trig_r = !settings_is_default_trigger(&xin_wl->profile.trigger_r);

    tuh_hxx_send_report(daddr, itf_num, CMD_INQUIRE_PRESENT, sizeof(CMD_INQUIRE_PRESENT));
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xinput_wl_unmounted(uint8_t index, uint8_t daddr, uint8_t itf_num) {
    (void)daddr;
    (void)itf_num;
    xinput_wl_state_t* xin_wl = xin_wl_state[index];
    cancel_repeating_timer(&xin_wl->chatpad_timer);
}

static void xinput_wl_report_received(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                                      uint8_t itf_num, const uint8_t* data, uint16_t len) {
    (void)subtype;
    xinput_wl_state_t* xin_wl = xin_wl_state[index];
    xinput_wl_event_t* event = (xinput_wl_event_t*)data;
    if (event->flags & XINPUT_WL_EVENT_CONNECTION) {
        if (event->status & XINPUT_WL_STATUS_CONTROLLER_PRESENT) {
            xin_wl->connected = true;
            xinput_wl_init_cb(daddr, itf_num, NULL, 0, true, xin_wl);
            add_repeating_timer_ms(CHATPAD_KEEPALIVE_MS, keepalive_cb, 
                                   xin_wl, &xin_wl->chatpad_timer);
            usb_host_driver_connect_cb(xin_wl->index, USBH_TYPE_XINPUT_WL, true);
        } else {
            if (xin_wl->connected) {
                xin_wl->init_state = XINPUT_INIT_GET_INFO;
                xin_wl->connected = false;
                cancel_repeating_timer(&xin_wl->chatpad_timer);
                usb_host_driver_connect_cb(xin_wl->index, USBH_TYPE_XINPUT_WL, false);
            }
        }
    }
    if ((event->status & (XINPUT_WL_STATUS_PAD_STATE | XINPUT_WL_STATUS_CHATPAD_STATE)) == 0) {
        tuh_hxx_receive_report(daddr, itf_num);
        return;
    }
    gamepad_pad_t* gp_report = &xin_wl->gp_report;
    if (event->status & XINPUT_WL_STATUS_PAD_STATE) {
        const xinput_wl_report_in_t* wl_report = (const xinput_wl_report_in_t*)data; 
        const xinput_report_in_t* report_in = &wl_report->report;
        gp_report->flags |= GAMEPAD_FLAG_PAD;
        memset(gp_report, 0, offsetof(gamepad_pad_t, chatpad));

        if (report_in->buttons & XINPUT_BUTTON_UP)    { gp_report->dpad |= GP_BIT(xin_wl->profile.btn_up); }
        if (report_in->buttons & XINPUT_BUTTON_DOWN)  { gp_report->dpad |= GP_BIT(xin_wl->profile.btn_down); }
        if (report_in->buttons & XINPUT_BUTTON_LEFT)  { gp_report->dpad |= GP_BIT(xin_wl->profile.btn_left); }
        if (report_in->buttons & XINPUT_BUTTON_RIGHT) { gp_report->dpad |= GP_BIT(xin_wl->profile.btn_right); }

        if (report_in->buttons & XINPUT_BUTTON_START)  { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_start); }
        if (report_in->buttons & XINPUT_BUTTON_BACK)   { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_back); }
        if (report_in->buttons & XINPUT_BUTTON_L3)     { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_l3); }
        if (report_in->buttons & XINPUT_BUTTON_R3)     { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_r3); }
        if (report_in->buttons & XINPUT_BUTTON_LB)     { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_lb); }
        if (report_in->buttons & XINPUT_BUTTON_RB)     { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_rb); }
        if (report_in->buttons & XINPUT_BUTTON_HOME)   { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_sys); }
        if (report_in->buttons & XINPUT_BUTTON_A)      { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_a); }
        if (report_in->buttons & XINPUT_BUTTON_B)      { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_b); }
        if (report_in->buttons & XINPUT_BUTTON_X)      { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_x); }
        if (report_in->buttons & XINPUT_BUTTON_Y)      { gp_report->buttons |= GP_BIT(xin_wl->profile.btn_y); }

        gp_report->trigger_l = report_in->trigger_l;
        gp_report->trigger_r = report_in->trigger_r;

        gp_report->joystick_lx = report_in->joystick_lx;
        gp_report->joystick_ly = report_in->joystick_ly;
        gp_report->joystick_rx = report_in->joystick_rx;
        gp_report->joystick_ry = report_in->joystick_ry;

        if (xin_wl->map.joy_l) {
            settings_scale_joysticks(&xin_wl->profile.joystick_l, &gp_report->joystick_lx, 
                                     &gp_report->joystick_ly);
        }
        if (xin_wl->map.joy_r) {
            settings_scale_joysticks(&xin_wl->profile.joystick_r, &gp_report->joystick_rx, 
                                     &gp_report->joystick_ry);
        }
        if (xin_wl->map.trig_l) {
            settings_scale_trigger(&xin_wl->profile.trigger_l, &gp_report->trigger_l);
        }
        if (xin_wl->map.trig_r) {
            settings_scale_trigger(&xin_wl->profile.trigger_r, &gp_report->trigger_r);
        }
    } 
    if (event->status & XINPUT_WL_STATUS_CHATPAD_STATE) {
        const xinput_wl_report_in_t* wl_report = (const xinput_wl_report_in_t*)data; 
        const xinput_chatpad_report_t* chatpad = &wl_report->chatpad;
        gp_report->flags |= GAMEPAD_FLAG_CHATPAD;
        if (chatpad->status == XINPUT_CHATPAD_STATUS_PRESSED) {
            gp_report->chatpad[0] = chatpad->buttons[0];
            gp_report->chatpad[1] = chatpad->buttons[1];
            gp_report->chatpad[2] = chatpad->buttons[2];
        } else {
            memset(gp_report->chatpad, 0, sizeof(gp_report->chatpad));
        }
    }
    if (memcmp(&xin_wl->prev_gp_report, gp_report, sizeof(gamepad_pad_t)) != 0) {
        usb_host_driver_pad_cb(index, gp_report);
        memcpy(&xin_wl->prev_gp_report, gp_report, sizeof(gamepad_pad_t));
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

static void xinput_wl_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    xinput_wl_state_t* xin_wl = xin_wl_state[index];
    xin_wl->buf_out[0]  = 0x00; // Event
    xin_wl->buf_out[1]  = XINPUT_WL_STATUS_PAD_STATE;
    xin_wl->buf_out[2]  = 0x0F; // ?
    xin_wl->buf_out[3]  = 0xC0; // ?
    xin_wl->buf_out[4]  = 0x00;
    xin_wl->buf_out[5]  = (rumble->l / 255U) * 100U; // Left rumble
    xin_wl->buf_out[6]  = (rumble->r / 255U) * 100U; // Right rumble
    xin_wl->buf_out[7]  = 0x00;
    xin_wl->buf_out[8]  = 0x00;
    xin_wl->buf_out[9]  = 0x00;
    xin_wl->buf_out[10] = 0x00;
    xin_wl->buf_out[11] = 0x00;
    tuh_hxx_send_report(daddr, itf_num, xin_wl->buf_out, 12);
}

static void xinput_wl_task(uint8_t index, uint8_t daddr, uint8_t itf_num) {
    xinput_wl_state_t* xin_wl = xin_wl_state[index];
    if (xin_wl->init_state == XINPUT_INIT_DONE) {
        /*  Sometimes initializing the chatpad immediately 
            on connect doesn't work, just send some init
            commands every so often */
        if (xin_wl->chat_keepalive_count >= 5) {
            xin_wl->chat_keepalive_count = 0;
            xin_wl->init_state = XINPUT_INIT_ENABLE_CHATPAD;
            xinput_wl_init_cb(daddr, itf_num, NULL, 0, true, xin_wl);
        } else if (xin_wl->chat_keepalive_pending) {
            xin_wl->chat_keepalive_pending = false;
            xin_wl->chat_keepalive_count++;
            switch (xin_wl->chat_ka_state) {
            case CHAT_KEEPALIVE_1:
                tuh_hxx_send_report(daddr, itf_num, CMD_CHATPAD_KEEPALIVE_1, 
                                    sizeof(CMD_CHATPAD_KEEPALIVE_1));
                xin_wl->chat_ka_state = CHAT_KEEPALIVE_2;
                break;
            case CHAT_KEEPALIVE_2:
                tuh_hxx_send_report(daddr, itf_num, CMD_CHATPAD_KEEPALIVE_2, 
                                    sizeof(CMD_CHATPAD_KEEPALIVE_2));
                xin_wl->chat_ka_state = CHAT_KEEPALIVE_1;
                break;
            }
        }

    }
}

const usb_host_driver_t USBH_DRIVER_XINPUT_WL = {
    .name = "XInput Wireless",
    .mounted_cb = xinput_wl_mounted,
    .unmounted_cb = xinput_wl_unmounted,
    .task_cb = xinput_wl_task,
    .report_cb = xinput_wl_report_received,
    .report_ctrl_cb = NULL,
    .send_rumble = xinput_wl_send_rumble,
    .send_audio = NULL,
};