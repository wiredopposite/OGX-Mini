#include <stdbool.h>
#include <string.h>
#include "pico_usbd.h"

#include "gamepad/range.h"
#include "usb/device/device.h"
#include "usb/descriptors/dinput.h"
#include "usb/device/device_private.h"

#define REPORT_ID_MAGIC ((uint8_t)0x03)

typedef struct {
    dinput_report_in_t  report_in;
    uint8_t             idle_rate;
    uint8_t             protocol;
    gamepad_pad_t       gp_pad;
} dinput_state_t;
_Static_assert(sizeof(dinput_state_t) <= USBD_STATUS_BUF_SIZE, "DInput state size exceeds buffer size");

static const uint8_t GUIDE_BTN_MAGIC[] = { 
    0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00 
};

static dinput_state_t* dinput_state[USBD_DEVICES_MAX] = { NULL };

static void dinput_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void dinput_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool dinput_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &DINPUT_DESC_DEVICE, 
                                   sizeof(DINPUT_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &DINPUT_DESC_CONFIG, 
                                   sizeof(DINPUT_DESC_CONFIG), NULL);
    case USB_DTYPE_STRING:
        {
        uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (idx < ARRAY_SIZE(DINPUT_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, DINPUT_DESC_STRING[idx], 
                                       DINPUT_DESC_STRING[idx]->bLength, NULL);
        }
        }
        break;
    case USB_DTYPE_HID_REPORT:
        return usbd_send_ctrl_resp(handle, DINPUT_DESC_REPORT, 
                                   sizeof(DINPUT_DESC_REPORT), NULL);
    case USB_DTYPE_HID:
        return usbd_send_ctrl_resp(handle, &DINPUT_DESC_CONFIG.hid, 
                                   sizeof(DINPUT_DESC_CONFIG.hid), NULL);
    default:
        break;
    }
    return false;
}

static bool dinput_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    if ((req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) !=
        (USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE)) {
        return false;
    }
    dinput_state_t* dinput = dinput_state[handle->port];
    switch (req->bRequest) {
    case USB_REQ_HID_SET_IDLE:
        dinput->idle_rate = (req->wValue >> 8) & 0xFF;
        return true;
    case USB_REQ_HID_GET_IDLE:
        return usbd_send_ctrl_resp(handle, &dinput->idle_rate, 
                                   sizeof(dinput->idle_rate), NULL);
    case USB_REQ_HID_SET_PROTOCOL:
        dinput->protocol = req->wValue;
        return true;
    case USB_REQ_HID_GET_PROTOCOL:
        return usbd_send_ctrl_resp(handle, &dinput->protocol, 
                                   sizeof(dinput->protocol), NULL);
    case USB_REQ_HID_SET_REPORT:
        return true;
    case USB_REQ_HID_GET_REPORT:
        {
        const uint8_t report_id = req->wValue >> 8;
        const uint8_t report_type = req->wValue & 0xFF;
        if (report_id == REPORT_ID_MAGIC) {
            return usbd_send_ctrl_resp(handle, GUIDE_BTN_MAGIC, 
                                       sizeof(GUIDE_BTN_MAGIC), NULL);
        } else if (report_type == USB_REQ_HID_REPORT_TYPE_INPUT) {
            return usbd_send_ctrl_resp(handle, &dinput->report_in, 
                                        sizeof(dinput_report_in_t), NULL);
        }
        }
    default:
        break;
    }
    return false;
}

static bool dinput_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &DINPUT_DESC_CONFIG);
}

static void dinput_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    dinput_state_t* dinput = dinput_state[handle->port];
    memset(&dinput->report_in, 0, sizeof(dinput->report_in));
    dinput->report_in.joystick_lx   = R_UINT8_MID;
    dinput->report_in.joystick_ly   = R_UINT8_MID;
    dinput->report_in.joystick_rx   = R_UINT8_MID;
    dinput->report_in.joystick_ry   = R_UINT8_MID;
    dinput->report_in.dpad          = DINPUT_DPAD_CENTER;
}

static void dinput_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    (void)handle;
    (void)epaddr;
}

static usbd_handle_t* dinput_init(const usb_device_driver_cfg_t* cfg) {
    usbd_driver_t driver = {
        .init_cb = dinput_init_cb,
        .deinit_cb = dinput_deinit_cb,
        .get_desc_cb = dinput_get_desc_cb,
        .set_config_cb = dinput_set_config_cb,
        .configured_cb = dinput_configured_cb,
        .ctrl_xfer_cb = dinput_ctrl_xfer_cb,
        .ep_xfer_cb = dinput_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, DINPUT_EPSIZE_CTRL);
    if (handle != NULL) {
        dinput_state[handle->port] = (dinput_state_t*)cfg->usb.status_buffer;
    }
    return handle;
}

static void dinput_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad, uint32_t flags) {
    if (!usbd_ep_ready(handle, DINPUT_EPADDR_IN) || !(flags & GAMEPAD_FLAG_IN_PAD)) {
        return;
    }
    dinput_state_t* dinput = dinput_state[handle->port];
    memset(&dinput->report_in.buttons, 0, sizeof(dinput->report_in.buttons));

    switch (pad->dpad) {
    case GAMEPAD_D_UP:      
        dinput->report_in.dpad = DINPUT_DPAD_UP;     
        break;
    case GAMEPAD_D_DOWN:    
        dinput->report_in.dpad = DINPUT_DPAD_DOWN;   
        break;
    case GAMEPAD_D_LEFT:    
        dinput->report_in.dpad = DINPUT_DPAD_LEFT;   
        break;
    case GAMEPAD_D_RIGHT:   
        dinput->report_in.dpad = DINPUT_DPAD_RIGHT;  
        break;
    case GAMEPAD_D_UP | GAMEPAD_D_LEFT:    
        dinput->report_in.dpad = DINPUT_DPAD_UP_LEFT; 
        break;
    case GAMEPAD_D_UP | GAMEPAD_D_RIGHT:   
        dinput->report_in.dpad = DINPUT_DPAD_UP_RIGHT; 
        break;
    case GAMEPAD_D_DOWN | GAMEPAD_D_LEFT:    
        dinput->report_in.dpad = DINPUT_DPAD_DOWN_LEFT; 
        break;
    case GAMEPAD_D_DOWN | GAMEPAD_D_RIGHT:   
        dinput->report_in.dpad = DINPUT_DPAD_DOWN_RIGHT; 
        break;
    default:    
        dinput->report_in.dpad = DINPUT_DPAD_CENTER; 
        break;
    }

    if (pad->buttons & GAMEPAD_BTN_A)      { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_CROSS; }
    if (pad->buttons & GAMEPAD_BTN_B)      { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_CIRCLE; }
    if (pad->buttons & GAMEPAD_BTN_X)      { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_SQUARE; }
    if (pad->buttons & GAMEPAD_BTN_Y)      { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_TRIANGLE; }
    if (pad->buttons & GAMEPAD_BTN_LB)     { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_L1; }
    if (pad->buttons & GAMEPAD_BTN_RB)     { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_R1; }
    
    if (pad->buttons & GAMEPAD_BTN_START)  { dinput->report_in.buttons[1] |= DINPUT_BUTTONS1_START; }
    if (pad->buttons & GAMEPAD_BTN_BACK)   { dinput->report_in.buttons[1] |= DINPUT_BUTTONS1_SELECT; }
    if (pad->buttons & GAMEPAD_BTN_L3)     { dinput->report_in.buttons[1] |= DINPUT_BUTTONS1_L3; }
    if (pad->buttons & GAMEPAD_BTN_R3)     { dinput->report_in.buttons[1] |= DINPUT_BUTTONS1_R3; }
    if (pad->buttons & GAMEPAD_BTN_SYS)    { dinput->report_in.buttons[1] |= DINPUT_BUTTONS1_SYS; }
    if (pad->buttons & GAMEPAD_BTN_MISC)   { dinput->report_in.buttons[1] |= DINPUT_BUTTONS1_MISC; }
    
    if (pad->trigger_l) { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_L2; }
    if (pad->trigger_r) { dinput->report_in.buttons[0] |= DINPUT_BUTTONS0_R2; }

    dinput->report_in.joystick_lx = range_int16_to_uint8(pad->joystick_lx);
    dinput->report_in.joystick_ly = range_int16_to_uint8(pad->joystick_ly);
    dinput->report_in.joystick_rx = range_int16_to_uint8(pad->joystick_rx);
    dinput->report_in.joystick_ry = range_int16_to_uint8(pad->joystick_ry);

    if (flags & GAMEPAD_FLAG_IN_PAD_ANALOG) {
        dinput->report_in.up_axis       = pad->analog[GAMEPAD_ANALOG_UP];
        dinput->report_in.right_axis    = pad->analog[GAMEPAD_ANALOG_RIGHT];
        dinput->report_in.down_axis     = pad->analog[GAMEPAD_ANALOG_DOWN];
        dinput->report_in.left_axis     = pad->analog[GAMEPAD_ANALOG_LEFT];
        dinput->report_in.l1_axis       = pad->analog[GAMEPAD_ANALOG_LB];
        dinput->report_in.r1_axis       = pad->analog[GAMEPAD_ANALOG_RB];
        dinput->report_in.triangle_axis = pad->analog[GAMEPAD_ANALOG_Y];
        dinput->report_in.circle_axis   = pad->analog[GAMEPAD_ANALOG_B];
        dinput->report_in.cross_axis    = pad->analog[GAMEPAD_ANALOG_A];
        dinput->report_in.square_axis   = pad->analog[GAMEPAD_ANALOG_X];
    } else {
        dinput->report_in.up_axis       = (pad->dpad & GAMEPAD_D_UP)    ? 0xFF : 0x00;
        dinput->report_in.right_axis    = (pad->dpad & GAMEPAD_D_RIGHT) ? 0xFF : 0x00;
        dinput->report_in.down_axis     = (pad->dpad & GAMEPAD_D_DOWN)  ? 0xFF : 0x00;
        dinput->report_in.left_axis     = (pad->dpad & GAMEPAD_D_LEFT)  ? 0xFF : 0x00;
        dinput->report_in.l1_axis       = (pad->buttons & GAMEPAD_BTN_LB)  ? 0xFF : 0x00;
        dinput->report_in.r1_axis       = (pad->buttons & GAMEPAD_BTN_RB)  ? 0xFF : 0x00;
        dinput->report_in.triangle_axis = (pad->buttons & GAMEPAD_BTN_Y)   ? 0xFF : 0x00;
        dinput->report_in.circle_axis   = (pad->buttons & GAMEPAD_BTN_B)   ? 0xFF : 0x00;
        dinput->report_in.cross_axis    = (pad->buttons & GAMEPAD_BTN_A)   ? 0xFF : 0x00;
        dinput->report_in.square_axis   = (pad->buttons & GAMEPAD_BTN_X)   ? 0xFF : 0x00;
    }
    usbd_ep_write(handle, DINPUT_EPADDR_IN, &dinput->report_in, sizeof(dinput->report_in));
}

const usb_device_driver_t USBD_DRIVER_DINPUT = {
    .name = "DInput",
    .init = dinput_init,
    .task = NULL,
    .set_audio = NULL,
    .set_pad = dinput_set_pad,
};