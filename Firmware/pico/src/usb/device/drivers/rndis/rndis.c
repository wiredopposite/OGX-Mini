#include <string.h>
#include "common/usb_def.h"
#include "common/class/rndis_def.h"

#include "log/log.h"
#include "usb/device/drivers/rndis/rndis.h"
#include "netif/ethernet.h"

#define RNDIS_LINK_SPEED    12000000           /* Link baudrate (12Mbit/s for USB-FS) */
#define RNDIS_VENDOR        "Wired Opposite"   /* NIC vendor name */
#define ENC_BUF_SIZE        (OID_LIST_LENGTH * 4 + 32)
#define MAC_OPT             NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | \
                            NDIS_MAC_OPTION_RECEIVE_SERIALIZED  | \
                            NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  | \
                            NDIS_MAC_OPTION_NO_LOOPBACK

typedef enum {
    RNDIS_STATE_IDLE = 0,
    RNDIS_STATE_INITIALIZED,
    RNDIS_STATE_DATA_INITIALIZED
} rndis_state_t;

typedef struct {
    rndis_state_t state;
    uint16_t      mtu;
    uint8_t       mac_addr[6];
	uint32_t      txok;
	uint32_t      rxok;
	uint32_t      txbad;
	uint32_t      rxbad;
    uint32_t      oid_packet_filter;
} rndis_t;

static const uint32_t OID_SUPPORT_LIST[] __attribute__((aligned(4))) = {
    NDIS_OID_GEN_SUPPORTED_LIST,
    NDIS_OID_GEN_HARDWARE_STATUS,
    NDIS_OID_GEN_MEDIA_SUPPORTED,
    NDIS_OID_GEN_MEDIA_IN_USE,
    NDIS_OID_GEN_MAXIMUM_FRAME_SIZE,
    NDIS_OID_GEN_LINK_SPEED,
    NDIS_OID_GEN_TRANSMIT_BLOCK_SIZE,
    NDIS_OID_GEN_RECEIVE_BLOCK_SIZE,
    NDIS_OID_GEN_VENDOR_ID,
    NDIS_OID_GEN_VENDOR_DESCRIPTION,
    NDIS_OID_GEN_VENDOR_DRIVER_VERSION,
    NDIS_OID_GEN_CURRENT_PACKET_FILTER,
    NDIS_OID_GEN_MAXIMUM_TOTAL_SIZE,
    NDIS_OID_GEN_PROTOCOL_OPTIONS,
    NDIS_OID_GEN_MAC_OPTIONS,
    NDIS_OID_GEN_MEDIA_CONNECT_STATUS,
    NDIS_OID_GEN_MAXIMUM_SEND_PACKETS,
    NDIS_OID_802_3_PERMANENT_ADDRESS,
    NDIS_OID_802_3_CURRENT_ADDRESS,
    NDIS_OID_802_3_MULTICAST_LIST,
    NDIS_OID_802_3_MAXIMUM_LIST_SIZE,
    NDIS_OID_802_3_MAC_OPTIONS
};

static const char *rndis_vendor = RNDIS_VENDOR;

static rndis_t rndis = {0};

// static void rndis_report(void) {
//   uint8_t ndis_report[8] = { 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
//   netd_report(ndis_report, sizeof(ndis_report));
// }

#define VENDOR_DRIVER_VERSION 0x00001000

static int32_t rndis_handle_query(const usb_rndis_msg_query_t* m, 
                                  usb_rndis_msg_query_cmplt_t* r) {
    int status = USB_RNDIS_STATUS_SUCCESS;
    uint32_t val = 0;
    const uint8_t* data = (const uint8_t*)&val;
    uint16_t size = sizeof(val);

    switch (m->Oid) {
    case NDIS_OID_GEN_SUPPORTED_LIST:   
        data = (const uint8_t*)OID_SUPPORT_LIST;
        size = sizeof(OID_SUPPORT_LIST);      
        break;
    case NDIS_OID_GEN_VENDOR_DRIVER_VERSION:  
        val = VENDOR_DRIVER_VERSION;
        break;
    case NDIS_OID_802_3_CURRENT_ADDRESS:      
    case NDIS_OID_802_3_PERMANENT_ADDRESS:   
        data = rndis.mac_addr;
        size = sizeof(rndis.mac_addr);
        break;
    case NDIS_OID_GEN_MEDIA_SUPPORTED:
    case NDIS_OID_GEN_MEDIA_IN_USE:
    case NDIS_OID_GEN_PHYSICAL_MEDIUM:
        val = NDIS_MEDIUM_802_3;
        break;
    case NDIS_OID_GEN_HARDWARE_STATUS:
        val = 0; // NDIS_HARDWARE_STATUS_READY
        break;
    case NDIS_OID_GEN_LINK_SPEED:
        val = RNDIS_LINK_SPEED / 100; // Convert to Kbps
        break;
    case NDIS_OID_GEN_VENDOR_ID:
        val = 0x00FFFFFF; // Vendor ID placeholder
        break;
    case NDIS_OID_GEN_VENDOR_DESCRIPTION:
        data = (const uint8_t*)rndis_vendor;
        size = strlen(rndis_vendor) + 1;
        break;
    case NDIS_OID_GEN_CURRENT_PACKET_FILTER:
        val = rndis.oid_packet_filter;
        break;    
    case NDIS_OID_GEN_MAXIMUM_FRAME_SIZE:
        val = rndis.mtu - SIZEOF_ETH_HDR;
        break;
    case NDIS_OID_GEN_MAXIMUM_TOTAL_SIZE: 
    case NDIS_OID_GEN_TRANSMIT_BLOCK_SIZE:
    case NDIS_OID_GEN_RECEIVE_BLOCK_SIZE: 
        val = rndis.mtu;
        break;
    case NDIS_OID_GEN_MEDIA_CONNECT_STATUS:
        val = NDIS_MEDIA_STATE_CONNECTED;
        break;
    case NDIS_OID_GEN_RNDIS_CONFIG_PARAMETER: 
        val = 0;
        break;    
    case NDIS_OID_802_3_MAXIMUM_LIST_SIZE:
        val = 1;
        break;
    case NDIS_OID_802_3_MULTICAST_LIST:
    case NDIS_OID_802_3_MAC_OPTIONS:
        status = USB_RNDIS_STATUS_NOT_SUPPORTED;
        val = 0;
        break;
    case NDIS_OID_GEN_MAC_OPTIONS: 
        // val = MAC_OPT; // MAC options
        // break;
    case NDIS_OID_802_3_RCV_ERROR_ALIGNMENT: 
    case NDIS_OID_802_3_XMIT_ONE_COLLISION:  
    case NDIS_OID_802_3_XMIT_MORE_COLLISIONS:
        val = 0;
        break;
    case NDIS_OID_GEN_XMIT_OK:
        val = rndis.txok;
        break;
    case NDIS_OID_GEN_RCV_OK:
        val = rndis.rxok;
        break;
    case NDIS_OID_GEN_RCV_ERROR:
        val = rndis.rxbad;
        break;
    case NDIS_OID_GEN_XMIT_ERROR:
        val = rndis.txbad;
        break;
    case NDIS_OID_GEN_RCV_NO_BUFFER:
        val = 0; // No buffer errors
        break;
    default:
        status = USB_RNDIS_STATUS_FAILURE;
        val = 0;
        break;
    }
    r->MessageType = USB_RNDIS_QUERY_CMPLT;
    r->MessageLength = sizeof(usb_rndis_msg_query_cmplt_t) + size;
    r->RequestId = m->RequestId;
    r->InformationBufferLength = size;
    r->InformationBufferOffset = 16;
    r->Status = status;
    memcpy(r + 1, data, size);
    return r->MessageLength;
}

// #define INFBUF  ((uint8_t *)&(m->RequestId) + m->InformationBufferOffset)

// static void rndis_handle_config_parm(const char *data, int keyoffset, int valoffset, int keylen, int vallen)
// {
//     (void)data;
//     (void)keyoffset;
//     (void)valoffset;
//     (void)keylen;
//     (void)vallen;
// }

// static void rndis_packetFilter(uint32_t newfilter)
// {
//     (void)newfilter;
// }

static int32_t rndis_handle_set_msg(const usb_rndis_msg_set_t* m, usb_rndis_msg_set_cmplt_t* r) {
    r->MessageType = USB_RNDIS_SET_CMPLT;
    r->MessageLength = sizeof(usb_rndis_msg_set_cmplt_t);
    r->Status = USB_RNDIS_STATUS_SUCCESS;
    r->RequestId = m->RequestId;

    switch (m->Oid) {
    case NDIS_OID_GEN_RNDIS_CONFIG_PARAMETER:
        {
        // usb_rndis_msg_config_param_t *p;
        // char *ptr = (char *)m;
        // ptr += sizeof(usb_rndis_msg_generic_t);
        // ptr += m->InformationBufferOffset;
        // p = (usb_rndis_msg_config_param_t *) ((void*) ptr);
        // rndis_handle_config_parm(ptr, p->ParameterNameOffset, p->ParameterValueOffset, p->ParameterNameLength, p->ParameterValueLength);
        }
        break;
    case NDIS_OID_GEN_CURRENT_PACKET_FILTER:
        {
        const uint8_t* data = (const uint8_t*)m->RequestId + m->InformationBufferOffset;
        memcpy(&rndis.oid_packet_filter, data, sizeof(rndis.oid_packet_filter));
        if (rndis.oid_packet_filter) {
            // rndis_packetFilter(rndis.oid_packet_filter);
            rndis.state = RNDIS_STATE_DATA_INITIALIZED;
        } else {
            rndis.state = RNDIS_STATE_INITIALIZED;
        }
        }
        break;
    case NDIS_OID_GEN_CURRENT_LOOKAHEAD:
        break;
    case NDIS_OID_GEN_PROTOCOL_OPTIONS:
        break;
    case NDIS_OID_802_3_MULTICAST_LIST:
        break;
    /* Power Management: fails */
    case NDIS_OID_PNP_ADD_WAKE_UP_PATTERN:
    case NDIS_OID_PNP_REMOVE_WAKE_UP_PATTERN:
    case NDIS_OID_PNP_ENABLE_WAKE_UP:
    default:
        r->Status = USB_RNDIS_STATUS_FAILURE;
        break;
    }
    return r->MessageLength;
}

int32_t rndis_handle_msg(const uint8_t* msg, uint8_t* resp_buf, uint16_t resp_buf_len) {
    // if (resp_buf_len < (rndis.mtu + sizeof(usb_rndis_msg_data_t))) {
    //     ogxm_loge("RNDIS: Response buffer too small: %d < %d", 
    //             resp_buf_len, rndis.mtu + sizeof(usb_rndis_msg_data_t));
    //     return -1;
    // }
    const usb_rndis_msg_generic_t* m = 
        (const usb_rndis_msg_generic_t*)msg;

    switch (m->MessageType) {
    case USB_RNDIS_INITIALIZE_MSG:
        ogxm_logv("RNDIS: Initialize message received\n");
        {
        usb_rndis_msg_init_cmplt_t *r = 
            (usb_rndis_msg_init_cmplt_t *)resp_buf;
        /* r->MessageID is same as before */
        r->MessageType = USB_RNDIS_INITIALIZE_CMPLT;
        r->MessageLength = sizeof(usb_rndis_msg_init_cmplt_t);
        r->RequestId = m->RequestId;
        r->MajorVersion = USB_RNDIS_MAJOR_VERSION;
        r->MinorVersion = USB_RNDIS_MINOR_VERSION;
        r->Status = USB_RNDIS_STATUS_SUCCESS;
        r->DeviceFlags = USB_RNDIS_DF_CONNECTIONLESS;
        r->Medium = USB_RNDIS_MEDIUM_802_3;
        r->MaxPacketsPerTransfer = 1;
        r->MaxTransferSize = rndis.mtu + sizeof(usb_rndis_msg_data_t);
        r->PacketAlignmentFactor = 0;
        r->AfListOffset = 0;
        r->AfListSize = 0;
        rndis.state = RNDIS_STATE_INITIALIZED;
        return r->MessageLength;
        }
    case USB_RNDIS_QUERY_MSG:
        ogxm_logv("RNDIS: Query message received\n");
        return rndis_handle_query((const usb_rndis_msg_query_t*)msg, 
                                  (usb_rndis_msg_query_cmplt_t*)resp_buf);
    case USB_RNDIS_SET_MSG:
        ogxm_logv("RNDIS: Set message received\n");
        return rndis_handle_set_msg((const usb_rndis_msg_set_t*)msg, 
                                    (usb_rndis_msg_set_cmplt_t*)resp_buf);
    case USB_RNDIS_RESET_MSG:
        ogxm_logv("RNDIS: Reset message received\n");
        {
        usb_rndis_msg_reset_cmplt_t *r = 
            (usb_rndis_msg_reset_cmplt_t *)resp_buf;
        rndis.state = RNDIS_STATE_IDLE;
        r->MessageType = USB_RNDIS_RESET_CMPLT;
        r->MessageLength = sizeof(usb_rndis_msg_reset_cmplt_t);
        r->Status = USB_RNDIS_STATUS_SUCCESS;
        r->AddressingReset = 1; /* Make it look like we did something */
        /* r->AddressingReset = 0; - Windows halts if set to 1 for some reason */
        return r->MessageLength;
        }
        break;
    case USB_RNDIS_KEEPALIVE_MSG:
        ogxm_logv("RNDIS: Keepalive message received\n");
        {
        usb_rndis_msg_keepalive_cmplt_t* r =
            (usb_rndis_msg_keepalive_cmplt_t*)resp_buf;
        r->MessageType = USB_RNDIS_KEEPALIVE_CMPLT;
        r->MessageLength = sizeof(usb_rndis_msg_keepalive_cmplt_t);
        r->RequestId = m->RequestId;
        r->Status = USB_RNDIS_STATUS_SUCCESS;
        return r->MessageLength;
        }
    case USB_RNDIS_INITIALIZE_CMPLT:
    case USB_RNDIS_QUERY_CMPLT:
    case USB_RNDIS_SET_CMPLT:
    case USB_RNDIS_RESET_CMPLT:
    case USB_RNDIS_KEEPALIVE_CMPLT:
    default:
        break;
    }
    ogxm_loge("RNDIS: Unknown message type: %d\n", m->MessageType);
    return -1;
}

void rndis_init(uint16_t mtu, uint8_t mac_address[6]) {
    rndis.state = RNDIS_STATE_IDLE;
    rndis.mtu = mtu;
    memcpy(rndis.mac_addr, mac_address, sizeof(rndis.mac_addr));
}