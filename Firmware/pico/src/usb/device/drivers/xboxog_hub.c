#include <string.h>
#include "common/usb_util.h"
#include "common/class/hub_def.h"
#include "usbd/usbd.h"
#include "usb/descriptors/xboxog_hub.h"
#include "usb/device/device_private.h"
#include "usb/device/device.h"

#define HUB_CHAR_XID_VENDOR (1U << 3)

typedef enum {
    HUB_PORT_GP = 1,
    HUB_PORT_XBLC,
    HUB_PORT_XMU,
} hub_port_type_t;

typedef struct {
    usbd_handle_t*              handle;
    const usb_device_driver_t*  driver;
    uint32_t                    status;
} ds_device_t;

typedef struct {
    uint8_t      ds_status;
    uint8_t      desc_hub_buf[sizeof(usb_desc_hub_t) + 1]; /* +1 = up to 7 ports */
    ds_device_t  devices[USBD_DEVICES_MAX];
} xboxog_hub_state_t;
_Static_assert(sizeof(xboxog_hub_state_t) <= USBD_STATUS_BUF_SIZE, "HUB state buffer too large");

static xboxog_hub_state_t* xb_hub_state = NULL;

static void xb_hub_reset_port(ds_device_t* device) {
    device->status = (1U << USB_FEATURE_PORT_CONNECTION)  |
                     (1U << USB_FEATURE_C_PORT_CONNECTION);
}

static bool xb_hub_handle_feature_req(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    const uint8_t feature = req->wValue & 0xFF;
    const uint8_t ds_port = req->wIndex & 0xFF;
    const uint8_t prev_ds_status = xb_hub_state->ds_status;
    ds_device_t* devices = xb_hub_state->devices;

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
            xb_hub_state->ds_status |= (1U << ds_port);
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
            xb_hub_state->ds_status |= (1U << ds_port);
        }
        break;
    default:
        return false;
    }
    if (xb_hub_state->ds_status != prev_ds_status) {
        /*  Report downstream status change to host. 
            If we've already written the EP and the host 
            hasn't read it yet, flush the stale data  */
        usbd_ep_flush(handle, HUB_XID_EPADDR_IN);
        usbd_ep_write(
            handle, 
            HUB_XID_EPADDR_IN, 
            &xb_hub_state->ds_status, 
            sizeof(xb_hub_state->ds_status)
        );
    }
    return true;
}

static void xb_hub_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void xb_hub_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool xb_hub_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return  usbd_send_ctrl_resp(handle, &HUB_XID_DESC_DEVICE, 
                                    sizeof(HUB_XID_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return  usbd_send_ctrl_resp(handle, &HUB_XID_DESC_CONFIG, 
                                    sizeof(HUB_XID_DESC_CONFIG),NULL);
    case USB_DTYPE_STRING:
        if (USB_DESC_INDEX(req->wValue) == 0) {
            return  usbd_send_ctrl_resp( 
                        handle, 
                        &HUB_XID_DESC_STR_LANGUAGE, 
                        HUB_XID_DESC_STR_LANGUAGE.bLength,
                        NULL
                    );
        }
        break;
    case USB_DTYPE_HUB:
        return  usbd_send_ctrl_resp(
                    handle, 
                    xb_hub_state->desc_hub_buf, 
                    ((const usb_desc_hub_t*)xb_hub_state->desc_hub_buf)->bLength, 
                    NULL
                );
    default:
        break;
    }
    return false;
}

static bool xb_hub_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)handle;
    (void)config;
    return usbd_configure_all_eps(handle, &HUB_XID_DESC_CONFIG);
}

static void xb_hub_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)handle;
    (void)config;
    for (uint8_t port = 1; port < USBD_DEVICES_MAX; port++) {
        if (xb_hub_state->devices[port].handle == NULL) {
            continue;
        }
        xb_hub_reset_port(&xb_hub_state->devices[port]);
    }
}

static bool xb_hub_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
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
        break;
    case USB_REQ_TYPE_CLASS:
        switch (req->bRequest) {
        case USB_REQ_STD_SET_FEATURE:
        case USB_REQ_STD_CLEAR_FEATURE:
            return xb_hub_handle_feature_req(handle, req);
        case USB_REQ_STD_GET_STATUS:
            if (req->wIndex >= USBD_DEVICES_MAX) {
                break;
            }
            return  usbd_send_ctrl_resp(
                        handle, 
                        &xb_hub_state->devices[req->wIndex].status, 
                        sizeof(xb_hub_state->devices[req->wIndex].status), 
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

static void xb_hub_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    (void)handle;
    (void)epaddr;
    xb_hub_state->ds_status = 0;
}

static usbd_handle_t* xboxog_hub_init(const usb_device_driver_cfg_t* config) {
    if ((config == NULL) || 
        (config->usb.status_buffer == NULL)) {
        return NULL;
    }
    uint8_t dev_count = 2;
    dev_count += (config->xboxog_hub.addons & USBD_ADDON_HEADSET) ? 1 : 0;
    dev_count += (config->xboxog_hub.addons & USBD_ADDON_MEMORY_CARD) ? 1 : 0;
    if (dev_count > USBD_DEVICES_MAX) {
        return NULL;
    }
    usbd_driver_t hub_driver = {
        .init_cb = xb_hub_init_cb,
        .deinit_cb = xb_hub_deinit_cb,
        .get_desc_cb = xb_hub_get_desc_cb,
        .set_config_cb = xb_hub_set_config_cb,
        .configured_cb = xb_hub_configured_cb,
        .ctrl_xfer_cb = xb_hub_ctrl_xfer_cb,
        .ep_xfer_cb = xb_hub_ep_xfer_cb,
    };

    usbd_handle_t* hub_handle = usbd_init(config->xboxog_hub.hw_type, &hub_driver, HUB_XID_EPSIZE_CTRL);
    if (hub_handle == NULL) {
        return NULL;
    }
    xb_hub_state = (xboxog_hub_state_t*)config->xboxog_hub.status_buffer[0];
    memset(xb_hub_state, 0, sizeof(xboxog_hub_state_t));

    usb_desc_hub_t* desc_hub = (usb_desc_hub_t*)xb_hub_state->desc_hub_buf;
    desc_hub->bLength             = sizeof(usb_desc_hub_t) + 1;
    desc_hub->bDescriptorType     = USB_DTYPE_HUB;
    desc_hub->bNumberOfPorts      = MIN(3, USBD_DEVICES_MAX - 1);
    desc_hub->wHubCharacteristics = USB_HUB_CHAR_POWER_SWITCH_INDIVIDUAL  |
                                    USB_HUB_CHAR_COMPOUND_DEVICE          |
                                    USB_HUB_CHAR_THINKTIME_8_FS_BITS      |
                                    USB_HUB_CHAR_XID;
    desc_hub->bPowerOnToPowerGood = 0x16;
    desc_hub->bHubControlCurrent  = 0xFA;

    usb_device_driver_cfg_t gp_cfg = {
        .usb = {
            .hw_type = config->xboxog_hub.hw_type,
            .addons = config->xboxog_hub.addons,
            .status_buffer = config->xboxog_hub.status_buffer[HUB_PORT_GP],
        }
    };
    xb_hub_state->devices[HUB_PORT_GP].driver = &USBD_DRIVER_XBOXOG_GP;
    xb_hub_state->devices[HUB_PORT_GP].handle = 
        xb_hub_state->devices[HUB_PORT_GP].driver->init(&gp_cfg);

    if (xb_hub_state->devices[HUB_PORT_GP].handle != NULL) {
        desc_hub->bRemoveAndPowerMask[0] |= (1U << HUB_PORT_GP);
        // xb_hub_reset_port(&xb_hub_state->devices[HUB_PORT_GP]);
    }

    // if (config->xboxog_hub.addons & USBD_ADDON_HEADSET) {
    //     usb_device_driver_cfg_t xblc_cfg = {
    //         .usb = {
    //             .hw_type = config->xboxog_hub.hw_type,
    //             .addons = config->xboxog_hub.addons,
    //             .status_buffer = config->xboxog_hub.status_buffer[HUB_PORT_XBLC],
    //         }
    //     };
    //     xb_hub_state->devices[HUB_PORT_XBLC].driver = &USBD_DRIVER_XBOXOG_XBLC;
    //     xb_hub_state->devices[HUB_PORT_XBLC].handle = 
    //         xb_hub_state->devices[HUB_PORT_XBLC].driver->init(&xblc_cfg);

    //     if (xb_hub_state->devices[HUB_PORT_XBLC].handle != NULL) {
    //         desc_hub->bRemoveAndPowerMask[0] |= (1U << HUB_PORT_XBLC);
    //         // xb_hub_reset_port(&xb_hub_state->devices[HUB_PORT_XBLC]);
    //     }
    // }
    // if (config->xboxog_hub.addons & USBD_ADDON_MEMPRY_CARD) {
    //     usb_device_driver_cfg_t xmu_cfg = {
    //         .usb = {
    //             .hw_type = config->xboxog_hub.hw_type,
    //             .addons = config->xboxog_hub.addons,
    //             .status_buffer = config->xboxog_hub.status_buffer[HUB_PORT_XMU],
    //         }
    //     };
    //     xb_hub_state->devices[HUB_PORT_XMU].driver = &USBD_DRIVER_XBOXOG_XMU;
    //     xb_hub_state->devices[HUB_PORT_XMU].handle = 
    //         xb_hub_state->devices[HUB_PORT_XMU].driver->init(&xmu_cfg);

    //     if (xb_hub_state->devices[HUB_PORT_XMU].handle != NULL) {
    //         desc_hub->bRemoveAndPowerMask[0] |= (1U << HUB_PORT_XMU);
    //         // xb_hub_reset_port(&xb_hub_state->devices[HUB_PORT_XMU]);
    //     }
    // }
    return hub_handle;
}

static void xboxog_hub_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad) {
    (void)handle;
    if ((HUB_PORT_GP < USBD_DEVICES_MAX) &&
        (xb_hub_state->devices[HUB_PORT_GP].handle != NULL)) {
        xb_hub_state->devices[HUB_PORT_GP].driver->set_pad(
            xb_hub_state->devices[HUB_PORT_GP].handle,
            pad
        );
    }
}

static void xboxog_hub_set_audio(usbd_handle_t* handle, const gamepad_pcm_out_t* pcm_out) {
    (void)handle;
    if ((HUB_PORT_XBLC < USBD_DEVICES_MAX) &&
        (xb_hub_state->devices[HUB_PORT_XBLC].handle != NULL)) {
        xb_hub_state->devices[HUB_PORT_XBLC].driver->set_audio(
            xb_hub_state->devices[HUB_PORT_XBLC].handle,
            pcm_out
        );
    }
}

const usb_device_driver_t USBD_DRIVER_XBOXOG_HUB = {
    .name = "XboxOG Hub",
    .init = xboxog_hub_init,
    .task = NULL,
    .set_audio = xboxog_hub_set_audio,
    .set_pad = xboxog_hub_set_pad,
};