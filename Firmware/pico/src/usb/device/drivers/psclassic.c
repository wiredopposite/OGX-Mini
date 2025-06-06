#include <stdbool.h>
#include <string.h>
#include "usbd/usbd.h"
#include "gamepad/range.h"
#include "usb/device/device.h"
#include "usb/descriptors/psclassic.h"
#include "usb/device/device_private.h"
#include "assert_compat.h"

#define JOY_POS_THRESHOLD       ((int16_t)10000)
#define JOY_NEG_THRESHOLD       ((int16_t)-10000)
#define JOY_POS_45_THRESHOLD    ((int16_t)JOY_POS_THRESHOLD * 2)
#define JOY_NEG_45_THRESHOLD    ((int16_t)JOY_NEG_THRESHOLD * 2)

typedef struct {
    psclassic_report_in_t   report_in;
    uint8_t                 idle_rate;
    uint8_t                 protocol;
    gamepad_pad_t           gp_pad;
} psclassic_state_t;
_STATIC_ASSERT(sizeof(psclassic_state_t) <= USBD_STATUS_BUF_SIZE, "PSClassic state size exceeds buffer size");

static psclassic_state_t* psclassic_state[USBD_DEVICES_MAX] = { NULL };

static inline bool meets_pos_threshold(int16_t joy_l, int16_t joy_r) { 
    return (joy_l >= JOY_POS_THRESHOLD) || (joy_r >= JOY_POS_THRESHOLD); 
}

static inline bool meets_neg_threshold(int16_t joy_l, int16_t joy_r) { 
    return (joy_l <= JOY_NEG_THRESHOLD) || (joy_r <= JOY_NEG_THRESHOLD); 
}

static inline bool meets_pos_45_threshold(int16_t joy_l, int16_t joy_r) { 
    return (joy_l >= JOY_POS_45_THRESHOLD) || (joy_r >= JOY_POS_45_THRESHOLD); 
}

static inline bool meets_neg_45_threshold(int16_t joy_l, int16_t joy_r) { 
    return (joy_l <= JOY_NEG_45_THRESHOLD) || (joy_r <= JOY_NEG_45_THRESHOLD); 
}

static void psclassic_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void psclassic_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool psclassic_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &PSCLASSIC_DESC_DEVICE, 
                                   sizeof(PSCLASSIC_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &PSCLASSIC_DESC_CONFIG, 
                                   sizeof(PSCLASSIC_DESC_CONFIG), NULL);
    case USB_DTYPE_STRING:
        {
        uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (idx < ARRAY_SIZE(PSCLASSIC_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, PSCLASSIC_DESC_STRING[idx], 
                                       PSCLASSIC_DESC_STRING[idx]->bLength, NULL);
        }
        }
        break;
    case USB_DTYPE_HID_REPORT:
        return usbd_send_ctrl_resp(handle, PSCLASSIC_DESC_REPORT, 
                                   sizeof(PSCLASSIC_DESC_REPORT), NULL);
    case USB_DTYPE_HID:
        return usbd_send_ctrl_resp(handle, &PSCLASSIC_DESC_CONFIG.hid, 
                                   sizeof(PSCLASSIC_DESC_CONFIG.hid), NULL);
    default:
        break;
    }
    return false;
}

static bool psclassic_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    if ((req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) !=
        (USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE)) {
        return false;
    }
    psclassic_state_t* psc = psclassic_state[handle->port];
    switch (req->bRequest) {
    case USB_REQ_HID_SET_IDLE:
        psc->idle_rate = (req->wValue >> 8) & 0xFF;
        return true;
    case USB_REQ_HID_GET_IDLE:
        return usbd_send_ctrl_resp(handle, &psc->idle_rate, 
                                   sizeof(psc->idle_rate), NULL);
    case USB_REQ_HID_SET_PROTOCOL:
        psc->protocol = req->wValue;
        return true;
    case USB_REQ_HID_GET_PROTOCOL:
        return usbd_send_ctrl_resp(handle, &psc->protocol, 
                                   sizeof(psc->protocol), NULL);
    case USB_REQ_HID_SET_REPORT:
        return true;
    case USB_REQ_HID_GET_REPORT:
        {
        const uint8_t report_type = req->wValue & 0xFF;
        if (report_type == USB_REQ_HID_REPORT_TYPE_INPUT) {
            return usbd_send_ctrl_resp(handle, &psc->report_in, 
                                        sizeof(psclassic_report_in_t), NULL);
        }
        }
    default:
        break;
    }
    return false;
}

static bool psclassic_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &PSCLASSIC_DESC_CONFIG);
}

static void psclassic_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    psclassic_state[handle->port]->report_in.buttons = PSCLASSIC_DPAD_CENTER;
}

static void psclassic_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    (void)handle;
    (void)epaddr;
}

static usbd_handle_t* psclassic_init(const usb_device_driver_cfg_t* cfg) {
    usbd_driver_t driver = {
        .init_cb = psclassic_init_cb,
        .deinit_cb = psclassic_deinit_cb,
        .get_desc_cb = psclassic_get_desc_cb,
        .set_config_cb = psclassic_set_config_cb,
        .configured_cb = psclassic_configured_cb,
        .ctrl_xfer_cb = psclassic_ctrl_xfer_cb,
        .ep_xfer_cb = psclassic_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, PSCLASSIC_EPSIZE_CTRL);
    if (handle != NULL) {
        psclassic_state[handle->port] = (psclassic_state_t*)cfg->usb.status_buffer;
    }
    return handle;
}

static void psclassic_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad, uint32_t flags) {
    if (!usbd_ep_ready(handle, PSCLASSIC_EPADDR_IN) || !(flags & GAMEPAD_FLAG_IN_PAD)) {
        return;
    }
    psclassic_state_t* psc = psclassic_state[handle->port];
    memset(&psc->report_in, 0, sizeof(psc->report_in));

    switch (pad->dpad) {
    case GAMEPAD_D_UP:      
        psc->report_in.buttons = PSCLASSIC_DPAD_UP;     
        break;
    case GAMEPAD_D_DOWN:    
        psc->report_in.buttons = PSCLASSIC_DPAD_DOWN;   
        break;
    case GAMEPAD_D_LEFT:    
        psc->report_in.buttons = PSCLASSIC_DPAD_LEFT;   
        break;
    case GAMEPAD_D_RIGHT:   
        psc->report_in.buttons = PSCLASSIC_DPAD_RIGHT;  
        break;
    case GAMEPAD_D_UP | GAMEPAD_D_LEFT:    
        psc->report_in.buttons = PSCLASSIC_DPAD_UP_LEFT; 
        break;
    case GAMEPAD_D_UP | GAMEPAD_D_RIGHT:   
        psc->report_in.buttons = PSCLASSIC_DPAD_UP_RIGHT; 
        break;
    case GAMEPAD_D_DOWN | GAMEPAD_D_LEFT:    
        psc->report_in.buttons = PSCLASSIC_DPAD_DOWN_LEFT; 
        break;
    case GAMEPAD_D_DOWN | GAMEPAD_D_RIGHT:   
        psc->report_in.buttons = PSCLASSIC_DPAD_DOWN_RIGHT; 
        break;
    default:    
        psc->report_in.buttons = PSCLASSIC_DPAD_CENTER; 
        break;
    }

    int16_t joy_lx = pad->joystick_lx;
    int16_t joy_ly = range_invert_int16(pad->joystick_ly);
    int16_t joy_rx = pad->joystick_rx;
    int16_t joy_ry = range_invert_int16(pad->joystick_ry);

    if (meets_pos_threshold(joy_lx, joy_rx)) {
        if (meets_neg_45_threshold(joy_ly, joy_ry)) {
            psc->report_in.buttons = PSCLASSIC_DPAD_DOWN_RIGHT;
        } else if (meets_pos_45_threshold(joy_ly, joy_ry)) {
            psc->report_in.buttons = PSCLASSIC_DPAD_UP_RIGHT;
        } else {
            psc->report_in.buttons = PSCLASSIC_DPAD_RIGHT;
        }
    } else if (meets_neg_threshold(joy_lx, joy_rx)) {
        if (meets_neg_45_threshold(joy_ly, joy_ry)) {
            psc->report_in.buttons = PSCLASSIC_DPAD_DOWN_LEFT;
        } else if (meets_pos_45_threshold(joy_ly, joy_ry)) {
            psc->report_in.buttons = PSCLASSIC_DPAD_UP_LEFT;
        } else {
            psc->report_in.buttons = PSCLASSIC_DPAD_LEFT;
        }
    } else if (meets_neg_threshold(joy_ly, joy_ry)) {
        psc->report_in.buttons = PSCLASSIC_DPAD_DOWN;
    } else if (meets_pos_threshold(joy_ly, joy_ry)) {
        psc->report_in.buttons = PSCLASSIC_DPAD_UP;
    }

    if (pad->buttons & GAMEPAD_BTN_A)       { psc->report_in.buttons |= PSCLASSIC_BTN_CROSS; }
    if (pad->buttons & GAMEPAD_BTN_B)       { psc->report_in.buttons |= PSCLASSIC_BTN_CIRCLE; }
    if (pad->buttons & GAMEPAD_BTN_X)       { psc->report_in.buttons |= PSCLASSIC_BTN_SQUARE; }
    if (pad->buttons & GAMEPAD_BTN_Y)       { psc->report_in.buttons |= PSCLASSIC_BTN_TRIANGLE; }
    if (pad->buttons & GAMEPAD_BTN_LB)      { psc->report_in.buttons |= PSCLASSIC_BTN_L1; }
    if (pad->buttons & GAMEPAD_BTN_RB)      { psc->report_in.buttons |= PSCLASSIC_BTN_R1; }
    if (pad->buttons & GAMEPAD_BTN_BACK)    { psc->report_in.buttons |= PSCLASSIC_BTN_SELECT; }
    if (pad->buttons & GAMEPAD_BTN_START)   { psc->report_in.buttons |= PSCLASSIC_BTN_START; }
    
    if (pad->trigger_l) { psc->report_in.buttons |= PSCLASSIC_BTN_L2; }
    if (pad->trigger_r) { psc->report_in.buttons |= PSCLASSIC_BTN_R2; }

    usbd_ep_write(handle, PSCLASSIC_EPADDR_IN, &psc->report_in, sizeof(psc->report_in));
}

const usb_device_driver_t USBD_DRIVER_PSCLASSIC = {
    .name = "PSClassic",
    .init = psclassic_init,
    .task = NULL,
    .set_audio = NULL,
    .set_pad = psclassic_set_pad,
};