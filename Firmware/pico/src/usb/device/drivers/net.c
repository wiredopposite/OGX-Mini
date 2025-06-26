#include <stdbool.h>
#include <string.h>
#include <pico/unique_id.h>
#include "usbd/usbd.h"
#include "common/class/cdc_def.h"
#include "common/class/rndis_def.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/apps/httpd.h"

#include "log/log.h"
#include "gamepad/gamepad.h"
#include "settings/settings.h"
#include "usb/device/device.h"
#include "usb/descriptors/net.h"
#include "usb/device/device_private.h"
#include "usb/device/drivers/rndis/rndis.h"
#include "webserver/webserver.h"

typedef struct {
    usb_ctrl_req_t header;
    uint32_t downlink;
    uint32_t uplink;
} ecm_notify_t;

typedef union {
    uint8_t rndis[0x80];
    ecm_notify_t ecm;
} net_notify_t;

typedef struct {
    struct {
        usbd_handle_t*  handle;
        net_config_t    config;
        uint8_t         itf_alt_data;
        uint16_t        out_xfer_idx;
        uint16_t        rndis_next_len;

        bool            in_xfer_busy;
        uint16_t        in_xfer_idx;
        uint16_t        in_xfer_len;
    } usb;
    bool                can_xmit;
    net_notify_t        notify;
} net_state_t;
_Static_assert(sizeof(net_state_t) <= USBD_STATUS_BUF_SIZE, "net_state_t size exceeds USBH_EPSIZE_MAX");

static const ecm_notify_t ECM_NOTIFY_NC = {
    .header = {
        .bmRequestType = 0xA1,
        .bRequest = 0 /* NETWORK_CONNECTION aka NetworkConnection */,
        .wValue = 1 /* Connected */,
        .wLength = 0,
    },
};

static const ecm_notify_t ECM_NOTIFY_CSC = {
    .header = {
        .bmRequestType = 0xA1,
        .bRequest = 0x2A /* CONNECTION_SPEED_CHANGE aka ConnectionSpeedChange */,
        .wLength = 8,
    },
    .downlink = 9728000,
    .uplink = 9728000,
};

static net_state_t* net_state = NULL;
static uint8_t net_buf_out[WEBSERVER_MTU + sizeof(usb_rndis_msg_data_t)] __attribute__((aligned(4)));
static uint8_t net_buf_in[WEBSERVER_MTU + sizeof(usb_rndis_msg_data_t)] __attribute__((aligned(4)));

// static int32_t write_ep_bulk_blocking(usbd_handle_t* handle, uint8_t epaddr, 
//                                       const void* data, uint32_t len) {
//     int32_t remain = len;
//     while (remain > 0) {
//         while (!usbd_ep_ready(handle, epaddr)) {
//             usbd_task();
//         }
//         int32_t written = usbd_ep_write(handle, epaddr, data, remain);
//         if (written < 0) {
//             ogxm_loge("Failed to write to endpoint %02x: %d", 
//                 epaddr, written);
//             return -1;
//         }
//         remain -= written;
//         data = (const uint8_t*)data + written;
//     }
//     return len;
// }

static bool handle_eth_frame_write(usbd_handle_t* handle) {
    if (net_state->usb.in_xfer_idx == net_state->usb.in_xfer_len) {
        ogxm_logv("NET: Transfer complete, %d bytes\n", 
                  net_state->usb.in_xfer_len);
        net_state->usb.in_xfer_busy = false;
        net_state->usb.in_xfer_idx = 0;
        net_state->usb.in_xfer_len = 0;
        return true;
    }
    net_state->usb.in_xfer_busy = true;
    int32_t written = usbd_ep_write(handle, NET_EPADDR_DATA_IN, 
                                    net_buf_in + net_state->usb.in_xfer_idx, 
                                    net_state->usb.in_xfer_len - net_state->usb.in_xfer_idx);
    if (written < 0) {
        ogxm_loge("Failed to write to endpoint %02x: %d", 
            NET_EPADDR_DATA_IN, written);
        net_state->usb.in_xfer_busy = false;
        net_state->usb.in_xfer_idx = 0;
        net_state->usb.in_xfer_len = 0;
        return false;
    }
    net_state->usb.in_xfer_idx += written;
    return true;
}

static err_t netif_linkoutput_cb(struct netif *netif, struct pbuf *p) {
    if ((net_state == NULL) || (net_state->usb.handle == NULL)) {
        ogxm_loge("USB device handle is not initialized");
        return ERR_IF;
    }
    if (!usbd_ep_ready(net_state->usb.handle, NET_EPADDR_DATA_IN) ||
        net_state->usb.in_xfer_busy) {
        ogxm_loge("Endpoint %02x is not ready for data transmission\n", 
                  NET_EPADDR_DATA_IN);
        return ERR_USE;
    }
    ogxm_logv("NET: Sending %d bytes on interface %d\n", p->tot_len, netif->num);
    
    switch (net_state->usb.config) {
    case NET_CONFIG_RNDIS:
        {
        usb_rndis_msg_data_t* hdr = (usb_rndis_msg_data_t*)net_buf_in;
        hdr->MessageType = USB_RNDIS_PACKET_MSG;
        hdr->MessageLength = sizeof(usb_rndis_msg_data_t) + p->tot_len;
        hdr->DataOffset = sizeof(usb_rndis_msg_data_t) - offsetof(usb_rndis_msg_data_t, DataOffset);
        hdr->DataLength = p->tot_len;
        hdr->OOBDataOffset = 0;
        hdr->OOBDataLength = 0;
        hdr->NumOOBDataElements = 0;
        hdr->PerPacketInfoOffset = 0;
        hdr->PerPacketInfoLength = 0;
        hdr->DeviceVcHandle = 0;
        hdr->Reserved = 0; 
        if (pbuf_copy_partial(p, net_buf_in + sizeof(usb_rndis_msg_data_t), 
                              p->tot_len, 0) != p->tot_len) {
            ogxm_loge("NET: Failed to copy pbuf data to RNDIS buffer");
            return ERR_IF;
        }
        
        net_state->usb.in_xfer_len = hdr->MessageLength;
        if (!handle_eth_frame_write(net_state->usb.handle)) {
            ogxm_loge("NET: Failed to write RNDIS data to endpoint %02x", 
                NET_EPADDR_DATA_IN);
            return ERR_IF;
        }
        ogxm_logv_hex(net_buf_in + sizeof(usb_rndis_msg_data_t), 16, "NET: 16:");
        }
        break;
    case NET_CONFIG_ECM:
        {
        const uint32_t len = p->tot_len;
        if (pbuf_copy_partial(p, net_buf_in, p->tot_len, 0) != p->tot_len) {
            ogxm_loge("NET: Failed to copy pbuf data to RNDIS buffer");
            return ERR_IF;
        }
        net_state->usb.in_xfer_len = len;
        handle_eth_frame_write(net_state->usb.handle);
        }
        break;
    default:
        ogxm_loge("NET: Unsupported NET configuration type: %d", 
            net_state->usb.config);
        return ERR_IF;
    }
    ogxm_logd("NET: Frame send started\n");
    return ERR_OK;
}

static void net_rumble_cb(uint8_t index, const gamepad_rumble_t* rumble) {
    (void)index;
    if ((net_state == NULL) || (net_state->usb.handle == NULL)) {
        ogxm_loge("USB device handle is not initialized");
        return;
    }
    usb_device_rumble_cb(net_state->usb.handle, rumble);
}

static void ecm_report(bool nc) {
    net_state->notify.ecm = (nc) ? ECM_NOTIFY_NC : ECM_NOTIFY_CSC;
    net_state->notify.ecm.header.wIndex = NET_ITF_COMM;
    usbd_ep_write(net_state->usb.handle, 
                  NET_EPADDR_COMM_IN, 
                  &net_state->notify.ecm, 
                  (nc) ? sizeof(net_state->notify.ecm.header) 
                       : sizeof(net_state->notify.ecm));
}

static void net_rndis_notify(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    static const uint8_t NDIS_REPORT[8] = { 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
    usbd_ep_write(handle, NET_EPADDR_COMM_IN, NDIS_REPORT, sizeof(NDIS_REPORT)); 
}

// USBD NET Class driver

static void net_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void net_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool net_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &NET_DESC_DEVICE, 
                                    sizeof(NET_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        ogxm_logd("NET: Configuration request for %d\n", 
            USB_DESC_INDEX(req->wValue) + 1);

        if (USB_DESC_INDEX(req->wValue) == (NET_CONFIG_RNDIS - 1)) {
            return usbd_send_ctrl_resp(handle, &NET_DESC_CONFIG_RNDIS, 
                                       sizeof(NET_DESC_CONFIG_RNDIS), NULL);
        } else if (USB_DESC_INDEX(req->wValue) == (NET_CONFIG_ECM - 1)) {
            return usbd_send_ctrl_resp(handle, &NET_DESC_CONFIG_ECM, 
                                       sizeof(NET_DESC_CONFIG_ECM), NULL);
        }
        break;
    case USB_DTYPE_STRING:
        {
        uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (idx < ARRAY_SIZE(NET_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, NET_DESC_STRING[idx], 
                                       NET_DESC_STRING[idx]->bLength, NULL);
        } else if (idx == NET_DESC_DEVICE.iSerialNumber) {
            const usb_desc_string_t* serial = usbd_get_desc_string_serial(handle);
            return usbd_send_ctrl_resp(handle, serial, 
                                       serial->bLength, NULL);
        } else if (idx == NET_MAC_STR_INDEX) {
            uint8_t mac_addr[6] = {0};
            webserver_get_mac_addr(mac_addr);
            uint8_t mac_str[6 * 4 + 2] = {0};
            usbd_hex_to_desc_string(mac_addr, sizeof(mac_addr), 
                                    mac_str, sizeof(mac_str));
            return usbd_send_ctrl_resp(handle, mac_str, 
                                       sizeof(mac_str), NULL);
        }
        }
        break;
    default:
        break;
    }
    return false;
}

static bool net_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    // ogxm_logd("NET: Control request: %02x %02x %04x %04x %04x\n", 
    //     req->bmRequestType, req->bRequest, req->wValue, req->wIndex, req->wLength);
    switch (req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) {
    case USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_INTERFACE:
        switch (req->bRequest) {
        case USB_REQ_STD_GET_INTERFACE:
            {
            const uint8_t itf_num = (uint8_t)req->wIndex;
            if (itf_num != NET_ITF_COMM &&
                itf_num != NET_ITF_DATA) {
                ogxm_loge("Invalid interface number: %d", itf_num);
                return false;
            }
            return usbd_send_ctrl_resp(handle, &net_state->usb.itf_alt_data, 
                                       sizeof(net_state->usb.itf_alt_data), NULL);
            }
            break;
        case USB_REQ_STD_SET_INTERFACE:
            {
            const uint8_t itf_num = (uint8_t)req->wIndex;
            const uint8_t req_alt = (uint8_t)req->wValue;

            // // Only valid for Data Interface with Alternate is either 0 or 1
            // TU_VERIFY(_netd_itf.itf_num+1 == itf_num && req_alt < 2);

            if (net_state->usb.config != NET_CONFIG_ECM) {
                ogxm_loge("Invalid configuration for SET_INTERFACE: %d", 
                    net_state->usb.config);
                return false;
            }

            net_state->usb.itf_alt_data = req_alt;

            if (net_state->usb.itf_alt_data != 0) {
                net_state->can_xmit = true; // we are ready to transmit a packet
            } else {
                // TODO close the endpoint pair
                // For now pretend that we did, this should have no harm since host won't try to
                // communicate with the endpoints again
                // _netd_itf.ep_in = _netd_itf.ep_out = 0
            }
            }
            return true;
        default:
            break;
        }
        break;
    case USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE:
        switch (req->bRequest) {
        case USB_REQ_CDC_GET_LINE_CODING:
        case USB_REQ_CDC_SET_LINE_CODING:
        case USB_REQ_CDC_SET_CONTROL_LINE_STATE:
            return false;
        default:
            break;
        }
        switch (net_state->usb.config) {
        case NET_CONFIG_RNDIS:
            {
            const usb_rndis_msg_generic_t* msg = 
                (const usb_rndis_msg_generic_t*)req->data;
            switch (req->bRequest) {
            case USB_REQ_RNDIS_SET_ENCAPSULATED_COMMAND:
                {
                int32_t ret = rndis_handle_msg(req->data, 
                                               net_state->notify.rndis, 
                                               sizeof(net_state->notify.rndis));
                if (ret < 0) {
                    ogxm_loge("NET: Failed to handle RNDIS message: %d, type: %02X\n", 
                        ret, msg->MessageType);
                    net_state->usb.rndis_next_len = 0;
                    return false;
                } else if (ret > 0) {
                    ogxm_logd("NET: RNDIS message handled, response length: %d, type: %02X\n", 
                        ret, msg->MessageType);
                    net_state->usb.rndis_next_len = ret;
                    net_rndis_notify(handle, req);
                    return true;
                }
                ogxm_logd("NET: RNDIS message handled, no response needed\n");
                net_state->usb.rndis_next_len = 0;
                return true;
                }
            case USB_REQ_RNDIS_GET_ENCAPSULATED_RESPONSE:
                ogxm_logd("NET: RNDIS message complete, length: %d\n", 
                          net_state->usb.rndis_next_len);
                return usbd_send_ctrl_resp(handle, 
                                           net_state->notify.rndis, 
                                           net_state->usb.rndis_next_len, NULL);
            default:
                break;
            }
            }
        case NET_CONFIG_ECM:
            /* the only required CDC-ECM Management Element Request is SetEthernetPacketFilter */
            if (USB_REQ_CDC_SET_ETHERNET_PACKET_FILTER == req->bRequest) {
                usbd_send_ctrl_resp(handle, NULL, 0, NULL);
                ecm_report(true);
                return true;
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    ogxm_loge("NET: Unhandled control request: %02x %02x %04x %04x %04x",
              req->bmRequestType, req->bRequest, req->wValue, req->wIndex, req->wLength);
    return false;
}

static bool net_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    net_state->usb.config = config;
    ogxm_logd("NET: Set configuration to %d, %s\n", config, 
              (config == NET_CONFIG_RNDIS) ? "RNDIS" : 
              (config == NET_CONFIG_ECM) ? "ECM" : "Unknown");
    switch (net_state->usb.config) {
    case NET_CONFIG_RNDIS:
        return usbd_configure_all_eps(handle, &NET_DESC_CONFIG_RNDIS);
    case NET_CONFIG_ECM:
        return usbd_configure_all_eps(handle, &NET_DESC_CONFIG_ECM);
    default:
        ogxm_loge("NET: Invalid configuration selected: %d", config);
        return false;
    }
}

static void net_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)handle;
    (void)config;
}

static void handle_rndis_ep_out(uint16_t len) {
    usb_rndis_msg_data_t *msg = (usb_rndis_msg_data_t*)net_buf_out;
    if ((net_state->usb.out_xfer_idx + len) < sizeof(usb_rndis_msg_data_t)) {
        net_state->usb.out_xfer_idx += len;
        return;
    }
    if (msg->MessageType != USB_RNDIS_PACKET_MSG) {
        ogxm_loge("Unexpected RNDIS message type: %d", msg->MessageType);
        net_state->usb.out_xfer_idx = 0;
        return;
    }
    uint32_t data_offset = RNDIS_DATA_OFFSET(msg);
    if ((net_state->usb.out_xfer_idx + len) < (data_offset + msg->DataLength)) {
        net_state->usb.out_xfer_idx += len;
        return;
    }
    net_state->usb.out_xfer_idx = 0;
    webserver_set_frame(&net_buf_out[data_offset], msg->DataLength);
    // ogxm_logv("NET: RNDIS OUT transfer complete, %d bytes\n", 
    //           msg->DataLength);
}

static void handle_ecm_ep_out(uint16_t len) {
    if (len < NET_EPSIZE_DATA) {
        net_state->usb.out_xfer_idx += len;
        webserver_set_frame(net_buf_out, net_state->usb.out_xfer_idx);
        net_state->usb.out_xfer_idx = 0;
    } else {
        net_state->usb.out_xfer_idx += len;
    }
}

static void net_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    switch (epaddr) {
    case NET_EPADDR_DATA_OUT:
        {
        uint8_t* data_p = &net_buf_out[net_state->usb.out_xfer_idx];
        int32_t len = usbd_ep_read(handle, epaddr, 
                                   data_p, NET_EPSIZE_DATA);
        if (len < 0) {
            ogxm_loge("NET: Failed to read from endpoint %02x: %d", epaddr, len);
            net_state->usb.out_xfer_idx = 0;
            return;
        }
        switch (net_state->usb.config) {
        case NET_CONFIG_RNDIS:
            handle_rndis_ep_out(len);
            break;
        case NET_CONFIG_ECM:
            handle_ecm_ep_out(len);
            break;
        }
        break;
        }
    case NET_EPADDR_DATA_IN:
        if (net_state->usb.in_xfer_busy) {
            handle_eth_frame_write(handle);
            // if (net_state->usb.in_xfer_busy) {
            //     // ogxm_logv("NET: Data IN xfer progress: %d/%d bytes\n", 
            //     //           net_state->usb.in_xfer_idx, net_state->usb.in_xfer_len);
            // }
        }
    default:
        break;
    }
}

// USBH driver callbacks

static usbd_handle_t* net_init(const usb_device_driver_cfg_t* cfg) {
    if ((net_state != NULL) &&
        (net_state->usb.handle != NULL) &&
        (net_state->usb.handle->state != USBD_STATE_DISABLED)) {
        return NULL;
    }
    usbd_driver_t driver = {
        .init_cb = net_init_cb,
        .deinit_cb = net_deinit_cb,
        .get_desc_cb = net_get_desc_cb,
        .set_config_cb = net_set_config_cb,
        .configured_cb = net_configured_cb,
        .ctrl_xfer_cb = net_ctrl_xfer_cb,
        .ep_xfer_cb = net_ep_xfer_cb,
    };
    net_state = (net_state_t*)cfg->usb.status_buffer;
    net_state->usb.handle = usbd_init(cfg->usb.hw_type, &driver, NET_EPSIZE_CTRL);

    if (net_state->usb.handle != NULL) {
        webserver_init(netif_linkoutput_cb, net_rumble_cb);

        usbd_connect(cfg->usb.hw_type);
    
        webserver_wait_ready(usbd_task);
        webserver_start();

        uint8_t mac_addr[6] = {0};
        webserver_get_mac_addr(mac_addr);
        rndis_init(WEBSERVER_MTU, mac_addr);
    } else {
        ogxm_loge("Failed to initialize USB device for net driver");
    }
    return net_state->usb.handle;
}

static void net_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad) {
    // net_state_t* net = net_state[handle->port];
    // memcpy(&net_state->report_in, pad, sizeof(gamepad_pad_t));
    // webserver_set_pad(pad);
}

static void net_task(usbd_handle_t* handle) {
    (void)handle;
    webserver_task();
}

const usb_device_driver_t USBD_DRIVER_NET = {
    .name = "Net",
    .init = net_init,
    .task = net_task,
    .set_pad = net_set_pad,
    .set_audio = NULL,
};