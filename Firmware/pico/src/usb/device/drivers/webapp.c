#include <stdbool.h>
#include <string.h>
#include "usbd/usbd.h"

#include "log/log.h"
#include "gamepad/gamepad.h"
#include "settings/settings.h"
#include "usb/device/device.h"
#include "usb/descriptors/webapp.h"
#include "usb/device/device_private.h"

typedef struct {
    gamepad_pad_t       report_in;
    gamepad_rumble_t    report_out;
    uint8_t             idle_rate;
    uint8_t             protocol;
    user_profile_t      profile;
} webapp_state_t;

static webapp_state_t* webapp_state[USBD_DEVICES_MAX] = { NULL };

static void webapp_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void webapp_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool webapp_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &WEBAPP_DESC_DEVICE, 
                                    sizeof(WEBAPP_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &WEBAPP_DESC_CONFIG, 
                                    sizeof(WEBAPP_DESC_CONFIG), NULL);
    case USB_DTYPE_STRING:
        {
        uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (idx < ARRAY_SIZE(WEBAPP_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, WEBAPP_DESC_STRING[idx], 
                                       WEBAPP_DESC_STRING[idx]->bLength, NULL);
        } else if (idx == WEBAPP_DESC_DEVICE.iSerialNumber) {
            const usb_desc_string_t* serial = usbd_get_desc_string_serial(handle);
            return usbd_send_ctrl_resp(handle, serial, 
                                       serial->bLength, NULL);
        }
        }
        break;
    case USB_DTYPE_HID_REPORT:
        return usbd_send_ctrl_resp(handle, WEBAPP_DESC_REPORT, 
                                    sizeof(WEBAPP_DESC_REPORT), NULL);
    case USB_DTYPE_HID:
        return usbd_send_ctrl_resp(handle, &WEBAPP_DESC_CONFIG.hid, 
                                    sizeof(WEBAPP_DESC_CONFIG.hid), NULL);
    default:
        break;
    }
    return false;
}

static bool webapp_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    webapp_state_t* webapp = webapp_state[handle->port];

    switch (req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) {
    case USB_REQ_TYPE_VENDOR | USB_REQ_RECIP_INTERFACE:
        {
        const uint8_t report_id = req->wValue & 0xFF;
        const uint8_t report_type = req->wValue >> 8;
        switch (req->bRequest) {
        case WEBAPP_REQ_GET_PROFILE:
            switch (report_id) {
            case WEBAPP_GET_PROFILE_REPORT_ID_BY_INDEX:
                {
                const uint8_t index = req->wIndex & 0xFF;
                if (index >= USER_PROFILES_MAX) {
                    ogxm_loge("Invalid profile index requested: %d", index);
                    return false;
                }
                settings_get_profile_by_index(index, &webapp->profile);
                return usbd_send_ctrl_resp(handle, &webapp->profile, 
                                           sizeof(user_profile_t), NULL);
                }
                break;
            case WEBAPP_GET_PROFILE_REPORT_ID_BY_ID:
                {
                const uint8_t id = req->wIndex & 0xFF;
                if ((id == 0) || (id > USER_PROFILES_MAX)) {
                    ogxm_loge("Invalid profile ID requested: %d", id);
                    return false;
                }
                settings_get_profile_by_id(id, &webapp->profile);
                return usbd_send_ctrl_resp(handle, &webapp->profile, 
                                           sizeof(user_profile_t), NULL);
                }
                break;
            default:
                break;
            }
            break;
        case WEBAPP_REQ_SET_PROFILE:
            {
            const uint8_t index = req->wIndex & 0xFF;
            if ((index >= GAMEPADS_MAX) || (req->wLength != sizeof(user_profile_t))) {
                ogxm_loge("Invalid profile size for set request: %d", req->wLength);
                return false;
            }
            settings_store_profile(index, (const user_profile_t*)req->data);
            return true;
            }
        case WEBAPP_REQ_SET_DEVICE_TYPE:
            {
            const usbd_type_t type = (usbd_type_t)(req->wValue & 0xFF);
            if (type > USBD_TYPE_COUNT) {
                ogxm_loge("Invalid device type requested: %d", type);
                return false;
            }
            settings_store_device_type(type);
            return true;
            }
        case WEBAPP_REQ_GET_DEVICE_TYPE:
            {
            uint8_t type = (uint8_t)(settings_get_device_type() & 0xFF);
            return usbd_send_ctrl_resp(handle, &type, sizeof(type), NULL);
            }
        case WEBAPP_REQ_GET_GAMEPAD_INFO:
            {
            uint8_t info[GAMEPADS_MAX + 1] = {0};
            info[0] = (uint8_t)GAMEPADS_MAX;
            for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
                info[i + 1] = settings_get_active_profile_id(i);
            }
            return usbd_send_ctrl_resp(handle, &info, sizeof(info), NULL);
            }
        default:
            break;
        }
        }
        break;
    case USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE:
        switch (req->bRequest) {
        case USB_REQ_HID_SET_IDLE:
            webapp->idle_rate = req->wValue >> 8;
            return true;
        case USB_REQ_HID_GET_IDLE:
            return usbd_send_ctrl_resp(handle, &webapp->idle_rate, 
                                       sizeof(webapp->idle_rate), NULL);
        case USB_REQ_HID_SET_PROTOCOL:
            webapp->protocol = req->wValue;
            return true;
        case USB_REQ_HID_GET_PROTOCOL:
            return usbd_send_ctrl_resp(handle, &webapp->protocol, 
                                       sizeof(webapp->protocol), NULL);
        case USB_REQ_HID_SET_REPORT:
            return true;
        case USB_REQ_HID_GET_REPORT:
            return usbd_send_ctrl_resp(handle, &webapp->report_in, 
                                       sizeof(gamepad_pad_t), NULL);
        default:
            break;
        }
        break;
    }
    return false;
}

static bool webapp_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &WEBAPP_DESC_CONFIG);
}

static void webapp_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)handle;
    (void)config;
}

static void webapp_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    webapp_state_t* webapp = webapp_state[handle->port];
    if (epaddr == WEBAPP_EPADDR_OUT) {
        int32_t len = usbd_ep_read(handle, epaddr, &webapp->report_out, sizeof(webapp->report_out));
        if (len != sizeof(webapp->report_out)) {
            ogxm_loge("Unexpected length read from OUT endpoint: %d", len);
            return;
        }
        usb_device_rumble_cb(handle, &webapp->report_out);
    }
}

static usbd_handle_t* webapp_init(const usb_device_driver_cfg_t* cfg) {
    usbd_driver_t driver = {
        .init_cb = webapp_init_cb,
        .deinit_cb = webapp_deinit_cb,
        .get_desc_cb = webapp_get_desc_cb,
        .set_config_cb = webapp_set_config_cb,
        .configured_cb = webapp_configured_cb,
        .ctrl_xfer_cb = webapp_ctrl_xfer_cb,
        .ep_xfer_cb = webapp_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, WEBAPP_EPSIZE_CTRL);
    if (handle != NULL) {
        webapp_state[handle->port] = (webapp_state_t*)cfg->usb.status_buffer;
        memset(webapp_state[handle->port], 0, sizeof(webapp_state_t));
    }
    return handle;
}

static void webapp_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad, uint32_t flags) {
    webapp_state_t* webapp = webapp_state[handle->port];
    if (!usbd_ep_ready(handle, WEBAPP_EPADDR_IN)) {
        return;
    }
    memcpy(&webapp->report_in, pad, sizeof(gamepad_pad_t));
    usbd_ep_write(handle, WEBAPP_EPADDR_IN, &webapp->report_in, sizeof(gamepad_pad_t));
}

const usb_device_driver_t USBD_DRIVER_WEBAPP = {
    .name = "WebApp",
    .init = webapp_init,
    .task = NULL,
    .set_pad = webapp_set_pad,
    .set_audio = NULL,
};