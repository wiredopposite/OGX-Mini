#include <stdbool.h>
#include <string.h>
#include "usbd/usbd.h"

#include "gamepad/range.h"
#include "usb/device/device.h"
#include "usb/descriptors/switch.h"
#include "usb/device/device_private.h"

typedef struct {
    switch_report_in_t  report_in;
    uint8_t             idle_rate;
    uint8_t             protocol;
    gamepad_pad_t       gp_pad;
} switch_state_t;
_Static_assert(sizeof(switch_state_t) <= USBD_STATUS_BUF_SIZE, "Switch state size exceeds buffer size");

static switch_state_t* switch_state[USBD_DEVICES_MAX] = { NULL };

static void switch_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void switch_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool switch_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &SWITCH_DESC_DEVICE, 
                                    sizeof(SWITCH_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &SWITCH_DESC_CONFIG, 
                                    sizeof(SWITCH_DESC_CONFIG), NULL);
    case USB_DTYPE_STRING:
        {
        uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (USB_DESC_INDEX(req->wValue) < ARRAY_SIZE(SWITCH_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, SWITCH_DESC_STRING[idx], 
                                       SWITCH_DESC_STRING[idx]->bLength, NULL);
        }
        }
        break;
    case USB_DTYPE_HID_REPORT:
        return usbd_send_ctrl_resp(handle, SWITCH_DESC_REPORT, 
                                    sizeof(SWITCH_DESC_REPORT), NULL);
    case USB_DTYPE_HID:
        return usbd_send_ctrl_resp(handle, &SWITCH_DESC_CONFIG.hid, 
                                    sizeof(SWITCH_DESC_CONFIG.hid), NULL);
    default:
        break;
    }
    return false;
}

static bool switch_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    if ((req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) !=
        (USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE)) {
        return false;
    }
    switch_state_t* sw = switch_state[handle->port];
    switch (req->bRequest) {
    case USB_REQ_HID_SET_IDLE:
        sw->idle_rate = (req->wValue >> 8) & 0xFF;
        return true;
    case USB_REQ_HID_GET_IDLE:
        return usbd_send_ctrl_resp(handle, &sw->idle_rate, 
                                   sizeof(sw->idle_rate), NULL);
    case USB_REQ_HID_SET_PROTOCOL:
        sw->protocol = req->wValue;
        return true;
    case USB_REQ_HID_GET_PROTOCOL:
        return usbd_send_ctrl_resp(handle, &sw->protocol, 
                                   sizeof(sw->protocol), NULL);
    case USB_REQ_HID_SET_REPORT:
        return true;
    case USB_REQ_HID_GET_REPORT:
        return usbd_send_ctrl_resp(handle, &sw->report_in, 
                                   sizeof(switch_report_in_t), NULL);
    default:
        break;
    }
    return false;
}

static bool switch_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &SWITCH_DESC_CONFIG);
}

static void switch_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    switch_state_t* sw = switch_state[handle->port];
    /* Report In */
    memset(&sw->report_in, 0, sizeof(sw->report_in));
    sw->report_in.dpad          = SWITCH_DPAD_CENTER;
    sw->report_in.joystick_lx   = R_UINT8_MID;
    sw->report_in.joystick_ly   = R_UINT8_MID;
    sw->report_in.joystick_rx   = R_UINT8_MID;
    sw->report_in.joystick_ry   = R_UINT8_MID;
}

static void switch_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    (void)handle;
    (void)epaddr;
}

static usbd_handle_t* switch_init(const usb_device_driver_cfg_t* cfg) {
    usbd_driver_t driver = {
        .init_cb = switch_init_cb,
        .deinit_cb = switch_deinit_cb,
        .get_desc_cb = switch_get_desc_cb,
        .set_config_cb = switch_set_config_cb,
        .configured_cb = switch_configured_cb,
        .ctrl_xfer_cb = switch_ctrl_xfer_cb,
        .ep_xfer_cb = switch_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, SWITCH_EPSIZE_CTRL);
    if (handle != NULL) {
        switch_state[handle->port] = (switch_state_t*)cfg->usb.status_buffer;
    }
    return handle;
}

static void switch_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad, uint32_t flags) {
    if (!usbd_ep_ready(handle, SWITCH_EPADDR_IN) || !(flags & GAMEPAD_FLAG_IN_PAD)) {
        return;
    }
    switch_state_t* sw = switch_state[handle->port];
    memset(&sw->report_in.buttons, 0, sizeof(sw->report_in.buttons));

    switch (pad->dpad) {
    case GAMEPAD_D_UP:      
        sw->report_in.dpad = SWITCH_DPAD_UP;     
        break;
    case GAMEPAD_D_DOWN:    
        sw->report_in.dpad = SWITCH_DPAD_DOWN;   
        break;
    case GAMEPAD_D_LEFT:    
        sw->report_in.dpad = SWITCH_DPAD_LEFT;   
        break;
    case GAMEPAD_D_RIGHT:   
        sw->report_in.dpad = SWITCH_DPAD_RIGHT;  
        break;
    case GAMEPAD_D_UP | GAMEPAD_D_LEFT:    
        sw->report_in.dpad = SWITCH_DPAD_UP_LEFT; 
        break;
    case GAMEPAD_D_UP | GAMEPAD_D_RIGHT:   
        sw->report_in.dpad = SWITCH_DPAD_UP_RIGHT; 
        break;
    case GAMEPAD_D_DOWN | GAMEPAD_D_LEFT:    
        sw->report_in.dpad = SWITCH_DPAD_DOWN_LEFT; 
        break;
    case GAMEPAD_D_DOWN | GAMEPAD_D_RIGHT:   
        sw->report_in.dpad = SWITCH_DPAD_DOWN_RIGHT; 
        break;
    default:    
        sw->report_in.dpad = SWITCH_DPAD_CENTER; 
        break;
    }

    if (pad->buttons & GAMEPAD_BTN_START)  { sw->report_in.buttons |= SWITCH_BUTTON_PLUS; }
    if (pad->buttons & GAMEPAD_BTN_BACK)   { sw->report_in.buttons |= SWITCH_BUTTON_MINUS; }
    if (pad->buttons & GAMEPAD_BTN_L3)     { sw->report_in.buttons |= SWITCH_BUTTON_L3; }
    if (pad->buttons & GAMEPAD_BTN_R3)     { sw->report_in.buttons |= SWITCH_BUTTON_R3; }
    if (pad->buttons & GAMEPAD_BTN_LB)     { sw->report_in.buttons |= SWITCH_BUTTON_L; }
    if (pad->buttons & GAMEPAD_BTN_RB)     { sw->report_in.buttons |= SWITCH_BUTTON_R; }
    if (pad->buttons & GAMEPAD_BTN_A)      { sw->report_in.buttons |= SWITCH_BUTTON_B; }
    if (pad->buttons & GAMEPAD_BTN_B)      { sw->report_in.buttons |= SWITCH_BUTTON_A; }
    if (pad->buttons & GAMEPAD_BTN_X)      { sw->report_in.buttons |= SWITCH_BUTTON_Y; }
    if (pad->buttons & GAMEPAD_BTN_Y)      { sw->report_in.buttons |= SWITCH_BUTTON_X; }
    if (pad->buttons & GAMEPAD_BTN_SYS)    { sw->report_in.buttons |= SWITCH_BUTTON_HOME; }
    if (pad->buttons & GAMEPAD_BTN_MISC)   { sw->report_in.buttons |= SWITCH_BUTTON_CAPTURE; }

    if (pad->trigger_l) { sw->report_in.buttons |= SWITCH_BUTTON_ZL; }
    if (pad->trigger_r) { sw->report_in.buttons |= SWITCH_BUTTON_ZR; }

    sw->report_in.joystick_lx = range_int16_to_uint8(pad->joystick_lx);
    sw->report_in.joystick_ly = range_int16_to_uint8(pad->joystick_ly);
    sw->report_in.joystick_rx = range_int16_to_uint8(pad->joystick_rx);
    sw->report_in.joystick_ry = range_int16_to_uint8(pad->joystick_ry);

    usbd_ep_write(handle, SWITCH_EPADDR_IN, &sw->report_in, sizeof(switch_report_in_t));
}

const usb_device_driver_t USBD_DRIVER_SWITCH = {
    .name = "Switch",
    .init = switch_init,
    .task = NULL,
    .set_audio = NULL,
    .set_pad = switch_set_pad,   
};