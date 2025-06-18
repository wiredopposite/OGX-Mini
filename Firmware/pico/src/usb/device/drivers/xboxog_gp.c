#include <string.h>
#include "common/usb_util.h"
#include "common/class/hid_def.h"
#include "usbd/usbd.h"
#include "gamepad/range.h"
#include "usb/descriptors/xboxog_gp.h"
#include "usb/device/device_private.h"
#include "usb/device/device.h"

typedef struct {
    xboxog_gp_report_in_t   report_in;
    xboxog_gp_report_out_t  report_out;
    gamepad_rumble_t        gp_rumble;
    gamepad_pad_t           gp_pad;
    gamepad_pad_t           prev_gp_pad;
} xboxog_gp_state_t;
_Static_assert(sizeof(xboxog_gp_state_t) <= USBD_STATUS_BUF_SIZE, "XBOXOG GP state size exceeds buffer size");

static xboxog_gp_state_t* xboxog_gp_state[USBD_DEVICES_MAX] = { NULL };

static void xboxog_gp_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void xboxog_gp_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool xboxog_gp_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &XBOXOG_GP_DESC_DEVICE,
                                   sizeof(XBOXOG_GP_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &XBOXOG_GP_DESC_CONFIG,
                                   sizeof(XBOXOG_GP_DESC_CONFIG), NULL);
    case USB_DTYPE_XID:
        return usbd_send_ctrl_resp(handle, &XBOXOG_GP_DESC_XID,
                                   sizeof(XBOXOG_GP_DESC_XID), NULL);
    default:
        break;
    }
    return false;
}

static bool xboxog_gp_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    xboxog_gp_state_t* xboxog = xboxog_gp_state[handle->port];
    switch (req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) {
    case (USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE):
        {
        // const uint8_t itf_num = req->wIndex & 0xFF;
        // const uint8_t report_id = req->wValue >> 8;
        // const uint8_t report_type = req->wValue & 0xFF;
        switch (req->bRequest) {
        case USB_REQ_HID_GET_REPORT:
            return usbd_send_ctrl_resp(handle, &xboxog->report_in,
                                       sizeof(xboxog->report_in), NULL);
        case USB_REQ_HID_SET_REPORT:
            if (req->wLength >= 4) {
                const xboxog_gp_report_out_t* report_out = 
                    (const xboxog_gp_report_out_t*)req->data;
                xboxog->gp_rumble.l = range_uint16_to_uint8(report_out->rumble_l);
                xboxog->gp_rumble.r = range_uint16_to_uint8(report_out->rumble_r);
                usb_device_rumble_cb(handle, &xboxog->gp_rumble);
            }
            return true;
        default:
            break;
        }
        }
        break;
    case (USB_REQ_TYPE_VENDOR | USB_REQ_RECIP_INTERFACE):
        if (req->bRequest == USB_REQ_XID_GET_CAPABILITIES) {
            const uint8_t report_id = req->wValue >> 8;
            switch (report_id) {
            case USB_XID_REPORT_CAPABILITIES_IN:
                return usbd_send_ctrl_resp(handle, &XBOXOG_GP_CAPABILITIES_IN,
                                           sizeof(XBOXOG_GP_CAPABILITIES_IN), NULL);
            case USB_XID_REPORT_CAPABILITIES_OUT:
                return usbd_send_ctrl_resp(handle, &XBOXOG_GP_CAPABILITIES_OUT,
                                           sizeof(XBOXOG_GP_CAPABILITIES_OUT), NULL);
            default:
                break;
            }
        }
        break;
    case (USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE):
        switch (req->bRequest) {
        case USB_REQ_STD_SET_INTERFACE:
            return true;
        case USB_REQ_STD_GET_STATUS:
            {
            static const uint8_t status[2] = { 0x00, 0x00 };
            return usbd_send_ctrl_resp(handle, status, sizeof(status), NULL);
            }
        default:
            break;
        }
        break;
    default:
        break;
    }
    return false;
}

static bool xboxog_gp_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &XBOXOG_GP_DESC_CONFIG);
}

static void xboxog_gp_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    xboxog_gp_state_t* xboxog = xboxog_gp_state[handle->port];
    memset(&xboxog->report_in, 0, sizeof(xboxog->report_in));
    xboxog->report_in.length = sizeof(xboxog->report_in);
}

static void xboxog_gp_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    if (epaddr == XBOXOG_GP_EPADDR_OUT) {
        xboxog_gp_state_t* xboxog = xboxog_gp_state[handle->port];
        int32_t len = usbd_ep_read(handle, epaddr, &xboxog->report_out,
                                    sizeof(xboxog->report_out));
        if ((len >= 4) && (xboxog->report_out.report_id == 0)) {
            xboxog->gp_rumble.l = xboxog->report_out.rumble_l;
            xboxog->gp_rumble.r = xboxog->report_out.rumble_r;
            usb_device_rumble_cb(handle, &xboxog->gp_rumble);
        }
    }
}

static usbd_handle_t* xboxog_gp_init(const usb_device_driver_cfg_t* cfg) {
    if ((cfg == NULL) || (cfg->usb.status_buffer == NULL)) {
        return NULL;
    }
    usbd_driver_t driver = {
        .init_cb = xboxog_gp_init_cb,
        .deinit_cb = xboxog_gp_deinit_cb,
        .get_desc_cb = xboxog_gp_get_desc_cb,
        .set_config_cb = xboxog_gp_set_config_cb,
        .configured_cb = xboxog_gp_configured_cb,
        .ctrl_xfer_cb = xboxog_gp_ctrl_xfer_cb,
        .ep_xfer_cb = xboxog_gp_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, XBOXOG_GP_EP0_SIZE);
    if (handle != NULL) {
        xboxog_gp_state[handle->port] = (xboxog_gp_state_t*)cfg->usb.status_buffer;
    }
    return handle;
}

static void xboxog_gp_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad) {
    if (!usbd_ep_ready(handle, XBOXOG_GP_EPADDR_IN) || !(pad->flags & GAMEPAD_FLAG_PAD)) {
        return;
    }
    xboxog_gp_state_t* xb = xboxog_gp_state[handle->port];

    xb->report_in.buttons = 0;

    if (pad->buttons & GAMEPAD_BUTTON_UP)       { xb->report_in.buttons |= XBOXOG_GP_BUTTON_UP; }
    if (pad->buttons & GAMEPAD_BUTTON_DOWN)     { xb->report_in.buttons |= XBOXOG_GP_BUTTON_DOWN; }
    if (pad->buttons & GAMEPAD_BUTTON_LEFT)     { xb->report_in.buttons |= XBOXOG_GP_BUTTON_LEFT; }
    if (pad->buttons & GAMEPAD_BUTTON_RIGHT)    { xb->report_in.buttons |= XBOXOG_GP_BUTTON_RIGHT; }
    if (pad->buttons & GAMEPAD_BUTTON_START)    { xb->report_in.buttons |= XBOXOG_GP_BUTTON_START; }
    if (pad->buttons & GAMEPAD_BUTTON_BACK)     { xb->report_in.buttons |= XBOXOG_GP_BUTTON_BACK; }
    if (pad->buttons & GAMEPAD_BUTTON_L3)       { xb->report_in.buttons |= XBOXOG_GP_BUTTON_L3; }
    if (pad->buttons & GAMEPAD_BUTTON_R3)       { xb->report_in.buttons |= XBOXOG_GP_BUTTON_R3; }

    xb->report_in.trigger_l = pad->trigger_l;
    xb->report_in.trigger_r = pad->trigger_r;

    xb->report_in.joystick_lx = pad->joystick_lx;
    xb->report_in.joystick_ly = pad->joystick_ly;
    xb->report_in.joystick_rx = pad->joystick_rx;
    xb->report_in.joystick_ry = pad->joystick_ry;

    if (pad->flags & GAMEPAD_FLAG_ANALOG) {
        xb->report_in.a     = pad->analog[GAMEPAD_ANALOG_A];
        xb->report_in.b     = pad->analog[GAMEPAD_ANALOG_B];
        xb->report_in.x     = pad->analog[GAMEPAD_ANALOG_X];
        xb->report_in.y     = pad->analog[GAMEPAD_ANALOG_Y];
        xb->report_in.white = pad->analog[GAMEPAD_ANALOG_LB];
        xb->report_in.black = pad->analog[GAMEPAD_ANALOG_RB];
    } else {
        xb->report_in.a     =   (pad->buttons & GAMEPAD_BUTTON_A) ? 0xFFU : 0x00U;
        xb->report_in.b     =   (pad->buttons & GAMEPAD_BUTTON_B) ? 0xFFU : 0x00U;
        xb->report_in.x     =   (pad->buttons & GAMEPAD_BUTTON_X) ? 0xFFU : 0x00U;
        xb->report_in.y     =   (pad->buttons & GAMEPAD_BUTTON_Y) ? 0xFFU : 0x00U;
        xb->report_in.white =   (pad->buttons & GAMEPAD_BUTTON_LB) ? 0xFFU : 0x00U;
        xb->report_in.black =   (pad->buttons & GAMEPAD_BUTTON_RB) ? 0xFFU : 0x00U;
    }
    usbd_ep_write(handle, XBOXOG_GP_EPADDR_IN, &xb->report_in, sizeof(xb->report_in));
}

const usb_device_driver_t USBD_DRIVER_XBOXOG_GP = {
    .name = "Xbox OG Gamepad",
    .init = xboxog_gp_init,
    .task = NULL,
    .set_audio = NULL,
    .set_pad = xboxog_gp_set_pad,
};