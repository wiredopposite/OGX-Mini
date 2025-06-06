#include <string.h>
#include "common/usb_util.h"
#include "common/class/hub_def.h"
#include "usbd/usbd.h"
#include "usb/descriptors/generic_hub.h"
#include "usb/descriptors/xboxog_hub.h"
#include "usb/device/drivers/generic_hub.h"
#include "assert_compat.h"

typedef struct {
    usbd_handle_t* handle;
    uint32_t       status;
} ds_device_t;

typedef struct {
    uint8_t      ds_status;
    uint8_t      desc_hub_buf[sizeof(usb_desc_hub_t) + 1]; /* +1 = up to 7 ports */
    ds_device_t  devices[USBD_DEVICES_MAX];
} hub_state_t;
_STATIC_ASSERT(sizeof(hub_state_t) <= USBD_STATUS_BUF_SIZE, "HUB state buffer too large");

static hub_state_t* hub_state = NULL;

static void hub_reset_port(ds_device_t* device) {
    device->status = (1U << USB_FEATURE_PORT_CONNECTION)  |
                     (1U << USB_FEATURE_C_PORT_CONNECTION);
}

static bool hub_handle_feature_req(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    const uint8_t feature = req->wValue & 0xFF;
    const uint8_t ds_port = req->wIndex & 0xFF;
    const uint8_t prev_ds_status = hub_state->ds_status;
    ds_device_t* devices = hub_state->devices;

    if ((feature >= 32) || (ds_port >= USBD_DEVICES_MAX)) {
        return false;
    }
    
    switch (req->bRequest) {
    case USB_REQ_STD_SET_FEATURE:
        // printf("HUB: Set feature %d, port: %d\n", feature, ds_port);
        if (devices[ds_port].status & (1U << feature)) {
            break; /* No change */
        }
        devices[ds_port].status |= (1U << feature);
        if (feature < USB_FEATURE_C_PORT_Pos) {
            /* Set port bit in status change mask */
            hub_state->ds_status |= (1U << ds_port);
        }
        if (feature == USB_FEATURE_PORT_RESET) {
            usbd_reset_device(devices[ds_port].handle); /* Reset downstream device */
            devices[ds_port].status &= ~(1U << USB_FEATURE_PORT_RESET);    /* Clear PORT reset*/
            devices[ds_port].status |=  (1U << USB_FEATURE_C_PORT_RESET) | /* Set C_PORT reset, will be cleared by host */
                                        (1U << USB_FEATURE_PORT_ENABLE);   /* Set PORT enable */    
        }
        break;
    case USB_REQ_STD_CLEAR_FEATURE:
        // printf("HUB: Clear feature %d, port: %d\n", feature, ds_port);
        if (!(devices[ds_port].status & (1U << feature))) {
            break; /* No change */
        }
        devices[ds_port].status &= ~(1U << feature);
        if (feature < USB_FEATURE_C_PORT_Pos) {
            /* Set port bit in status change mask */
            hub_state->ds_status |= (1U << ds_port);
        }
        break;
    default:
        return false;
    }
    if (hub_state->ds_status != prev_ds_status) {
        /*  Report downstream status change to host. 
            If we've already written the EP and the host 
            hasn't read it yet, flush the stale data  */
        usbd_ep_flush(handle, HUB_GEN_EPADDR_IN);
        usbd_ep_write(
            handle, 
            HUB_GEN_EPADDR_IN, 
            &hub_state->ds_status, 
            sizeof(hub_state->ds_status)
        );
    }
    return true;
}

static void hub_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void hub_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool hub_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return  usbd_send_ctrl_resp(handle, &HUB_GEN_DESC_DEVICE, 
                                    sizeof(HUB_GEN_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return  usbd_send_ctrl_resp(handle, &HUB_GEN_DESC_CONFIG, 
                                    sizeof(HUB_GEN_DESC_CONFIG),NULL);
    case USB_DTYPE_STRING:
        {
        const uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (idx < ARRAY_SIZE(HUB_GEN_DESC_STR)) {
            return  usbd_send_ctrl_resp( 
                        handle, 
                        HUB_GEN_DESC_STR[idx], 
                        HUB_GEN_DESC_STR[idx]->bLength,
                        NULL
                    );
        } else if (idx == HUB_GEN_DESC_DEVICE.iSerialNumber) {
            const usb_desc_string_t* serial = usbd_get_desc_string_serial(handle);
            return usbd_send_ctrl_resp(handle, serial, serial->bLength, NULL);
        }
        }
        break;
    case USB_DTYPE_HUB:
        return  usbd_send_ctrl_resp(
                    handle, 
                    hub_state->desc_hub_buf, 
                    ((const usb_desc_hub_t*)hub_state->desc_hub_buf)->bLength, 
                    NULL
                );
    default:
        break;
    }
    return false;
}

static bool hub_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)handle;
    (void)config;
    return usbd_configure_all_eps(handle, &HUB_GEN_DESC_CONFIG);
}

static void hub_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)handle;
    (void)config;
    for (uint8_t port = 1; port < USBD_DEVICES_MAX; port++) {
        hub_reset_port(&hub_state->devices[port]);
    }
}

static bool hub_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (req->bmRequestType & USB_REQ_TYPE_Msk) {
    case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_STD_GET_STATUS:
            {
            uint16_t status = 0; // Bus powered (0) | remote wakeup disabled (0)
            return usbd_send_ctrl_resp(handle, &status, sizeof(status), NULL);
            }
        default:
            break;
        }
    case USB_REQ_TYPE_CLASS:
        switch (req->bRequest) {
        case USB_REQ_STD_SET_FEATURE:
        case USB_REQ_STD_CLEAR_FEATURE:
            return hub_handle_feature_req(handle, req);
        case USB_REQ_STD_GET_STATUS:
            if (req->wIndex >= USBD_DEVICES_MAX) {
                break;
            }
            return  usbd_send_ctrl_resp(
                        handle, 
                        &hub_state->devices[req->wIndex].status, 
                        sizeof(hub_state->devices[req->wIndex].status), 
                        NULL
                    );
        default:
            break;
        }
        break;
    default:
        break;
    }
    return false;
}

static void hub_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    (void)handle;
    (void)epaddr;
    hub_state->ds_status = 0;
}

void generic_hub_register_ds_port(usbd_handle_t* handle) {
    if (hub_state == NULL) {
        return;
    }
    usb_desc_hub_t* desc_hub = (usb_desc_hub_t*)hub_state->desc_hub_buf;
    for (uint8_t port = 1; port < (desc_hub->bNumberOfPorts + 1); port++) {
        if (hub_state->devices[port].handle == NULL) {
            hub_state->devices[port].handle = handle;
            hub_reset_port(&hub_state->devices[port]);
            /* Signifies non-removable ports */
            desc_hub->bRemoveAndPowerMask[0] |= (uint8_t)(1 << port);
            break;
        }
    }
}

usbd_handle_t* generic_hub_init(const usb_device_driver_cfg_t* config) {
    if ((config == NULL) || 
        (config->usb.status_buffer == NULL)) {
        return NULL;
    }
    usbd_driver_t hub_driver = {
        .init_cb = hub_init_cb,
        .deinit_cb = hub_deinit_cb,
        .get_desc_cb = hub_get_desc_cb,
        .set_config_cb = hub_set_config_cb,
        .configured_cb = hub_configured_cb,
        .ctrl_xfer_cb = hub_ctrl_xfer_cb,
        .ep_xfer_cb = hub_ep_xfer_cb,
    };

    usbd_handle_t* handle = usbd_init(config->usb.hw_type, &hub_driver, HUB_GEN_EPSIZE_CTRL);
    if (handle == NULL) {
        return NULL;
    }
    hub_state = (hub_state_t*)config->usb.status_buffer;
    memset(hub_state, 0, sizeof(hub_state_t));

    usb_desc_hub_t* desc_hub = (usb_desc_hub_t*)hub_state->desc_hub_buf;
    desc_hub->bLength             = sizeof(usb_desc_hub_t) + 1;
    desc_hub->bDescriptorType     = USB_DTYPE_HUB;
    desc_hub->bNumberOfPorts      = MIN(7, USBD_DEVICES_MAX - 1);
    desc_hub->wHubCharacteristics = USB_HUB_CHAR_POWER_SWITCH_INDIVIDUAL  |
                                    USB_HUB_CHAR_COMPOUND_DEVICE          |
                                    USB_HUB_CHAR_THINKTIME_8_FS_BITS      ;
    desc_hub->bPowerOnToPowerGood = 0x16;
    desc_hub->bHubControlCurrent  = 0xFA;
    // for (uint8_t i = 1; i < desc_hub->bNumberOfPorts + 1; i++) {
    //     /* Signifies non-removable ports, indexed from 1 */
    //     desc_hub->bRemoveAndPowerMask[0] |= (1U << i);
    // }
    return handle;
}