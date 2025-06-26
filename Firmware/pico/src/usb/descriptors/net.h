#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/usb_util.h"
#include "common/class/cdc_def.h"
#include "common/class/rndis_def.h"
#include "webserver/webserver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NET_EPSIZE_CTRL     ((uint8_t)64)
#define NET_EPSIZE_COMM     ((uint16_t)8)
#define NET_EPSIZE_DATA     ((uint16_t)64)

#define NET_EPADDR_COMM_IN  ((uint8_t)USB_EP_DIR_IN  | 1U)
#define NET_EPADDR_DATA_IN  ((uint8_t)USB_EP_DIR_IN  | 2U)
#define NET_EPADDR_DATA_OUT ((uint8_t)USB_EP_DIR_OUT | 3U)

#define NET_MAC_STR_INDEX 4U

typedef enum {
    NET_ITF_COMM = 0,
    NET_ITF_DATA,
    NET_ITF_TOTAL
} net_itf_t;

typedef enum {
    NET_CONFIG_RNDIS = 1,
    NET_CONFIG_ECM = 2,
    NET_CONFIG_TOTAL = 2
} net_config_t;

static const usb_desc_device_t NET_DESC_DEVICE = {
    .bLength = sizeof(usb_desc_device_t),
    .bDescriptorType = USB_DTYPE_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_MISC,
    .bDeviceSubClass = USB_SUBCLASS_IAD,
    .bDeviceProtocol = USB_PROTOCOL_IAD,
    .bMaxPacketSize0 = NET_EPSIZE_CTRL,
    .idVendor = 0xCafe,
    .idProduct = 0x5678,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = NET_CONFIG_TOTAL
};

static const usb_desc_string_t NET_DESC_STR_LANG_ID = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t NET_DESC_STR_VENDOR  = USB_STRING_DESC("Wired Opposite");
static const usb_desc_string_t NET_DESC_STR_PRODUCT = USB_STRING_DESC("OGX-Mini WebUSB");
static const usb_desc_string_t* NET_DESC_STRING[] = {
    &NET_DESC_STR_LANG_ID,
    &NET_DESC_STR_VENDOR,
    &NET_DESC_STR_PRODUCT,
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t config;
    usb_desc_iad_t iad;
    usb_desc_itf_t itf_comm;
    usb_cdc_desc_header_t header;
    usb_cdc_desc_call_mgmt_t call_mgmt;
    usb_cdc_desc_acm_t acm;
    usb_cdc_desc_union_t union_desc;
    usb_desc_endpoint_t ep_notif;
    usb_desc_itf_t itf_data;
    usb_desc_endpoint_t ep_in;
    usb_desc_endpoint_t ep_out;
} net_desc_config_rndis_t;

typedef struct __attribute__((packed)) {
    usb_desc_config_t config;
    usb_desc_iad_t iad;
    usb_desc_itf_t itf_comm;
    usb_cdc_desc_header_t header;
    usb_cdc_desc_call_mgmt_t call_mgmt;
    usb_cdc_desc_union_t union_desc;
    usb_cdc_desc_ecm_func_t ecm_func;
    usb_desc_endpoint_t ep_notif;
    usb_desc_itf_t itf_data;
    usb_desc_endpoint_t ep_in;
    usb_desc_endpoint_t ep_out;
} net_desc_config_ecm_t;

// Frame 22182: 14 bytes on wire (112 bits), 14 bytes captured (112 bits) on interface usb, id 0
// USB Link Layer
// USB URB
// CONFIGURATION DESCRIPTOR
//     bLength: 9
//     bDescriptorType: 0x02 (CONFIGURATION)
//     wTotalLength: 75
//     bNumInterfaces: 2
//     bConfigurationValue: 1
//     iConfiguration: 0
//     Configuration bmAttributes: 0x80  NOT SELF-POWERED  NO REMOTE-WAKEUP
//     bMaxPower: 50  (100mA)
// INTERFACE ASSOCIATION DESCRIPTOR
//     bLength: 8
//     bDescriptorType: 0x0b (INTERFACE ASSOCIATION)
//     bFirstInterface: 0
//     bInterfaceCount: 2
//     bFunctionClass: Wireless Controller (0xe0)
//     bFunctionSubClass: 0x01
//     bFunctionProtocol: 0x03
//     iFunction: 0
// INTERFACE DESCRIPTOR (0.0): class Wireless Controller
//     bLength: 9
//     bDescriptorType: 0x04 (INTERFACE)
//     bInterfaceNumber: 0
//     bAlternateSetting: 0
//     bNumEndpoints: 1
//     bInterfaceClass: Wireless Controller (0xe0)
//     bInterfaceSubClass: 0x01
//     bInterfaceProtocol: 0x03
//     iInterface: 4
// UNKNOWN DESCRIPTOR
//     bLength: 5
//     bDescriptorType: 0x24 (unknown)
// UNKNOWN DESCRIPTOR
//     bLength: 5
//     bDescriptorType: 0x24 (unknown)
// UNKNOWN DESCRIPTOR
//     bLength: 4
//     bDescriptorType: 0x24 (unknown)
// UNKNOWN DESCRIPTOR
//     bLength: 5
//     bDescriptorType: 0x24 (unknown)
// ENDPOINT DESCRIPTOR
//     bLength: 7
//     bDescriptorType: 0x05 (ENDPOINT)
//     bEndpointAddress: 0x81  IN  Endpoint:1
//     bmAttributes: 0x03
//     wMaxPacketSize: 8
//     bInterval: 1
// INTERFACE DESCRIPTOR (1.0): class CDC-Data
//     bLength: 9
//     bDescriptorType: 0x04 (INTERFACE)
//     bInterfaceNumber: 1
//     bAlternateSetting: 0
//     bNumEndpoints: 2
//     bInterfaceClass: CDC-Data (0x0a)
//     bInterfaceSubClass: 0x00
//     bInterfaceProtocol: No class specific protocol required (0x00)
//     iInterface: 0
// ENDPOINT DESCRIPTOR
//     bLength: 7
//     bDescriptorType: 0x05 (ENDPOINT)
//     bEndpointAddress: 0x82  IN  Endpoint:2
//     bmAttributes: 0x02
//     wMaxPacketSize: 64
//     bInterval: 0
// ENDPOINT DESCRIPTOR
//     bLength: 7
//     bDescriptorType: 0x05 (ENDPOINT)
//     bEndpointAddress: 0x02  OUT  Endpoint:2
//     bmAttributes: 0x02
//     wMaxPacketSize: 64
//     bInterval: 0

static const net_desc_config_rndis_t NET_DESC_CONFIG_RNDIS = {
    .config = {
        .bLength = sizeof(usb_desc_config_t),
        .bDescriptorType = USB_DTYPE_CONFIGURATION,
        .wTotalLength = sizeof(net_desc_config_rndis_t),
        .bNumInterfaces = NET_ITF_TOTAL,
        .bConfigurationValue = NET_CONFIG_RNDIS,
        .iConfiguration = 0,
        .bmAttributes = USB_ATTR_RESERVED,
        .bMaxPower = 100,
    },
    .iad = {
        .bLength = sizeof(usb_desc_iad_t),
        .bDescriptorType = USB_DTYPE_IAD,
        .bFirstInterface = NET_ITF_COMM,
        .bInterfaceCount = 2,
        .bFunctionClass = USB_CLASS_RNDIS,
        .bFunctionSubClass = USB_RNDIS_ITF_SUBCLASS,
        .bFunctionProtocol = USB_RNDIS_ITF_PROTOCOL,
        .iFunction = 0
    },
    .itf_comm = {
        .bLength = sizeof(usb_desc_itf_t),
        .bDescriptorType = USB_DTYPE_INTERFACE,
        .bInterfaceNumber = NET_ITF_COMM,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_RNDIS,
        .bInterfaceSubClass = USB_RNDIS_ITF_SUBCLASS,
        .bInterfaceProtocol = USB_RNDIS_ITF_PROTOCOL,
        .iInterface = 0
    },
    .header = {
        .bFunctionLength = sizeof(usb_cdc_desc_header_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_HEADER,
        .bcdCDC = 0x0110
    },
    .call_mgmt = {
        .bFunctionLength = sizeof(usb_cdc_desc_call_mgmt_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_CALL_MGMT,
        .bmCapabilities = 0x00,
        .bDataInterface = NET_ITF_DATA,
    },
    .acm = {
        .bFunctionLength = sizeof(usb_cdc_desc_acm_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_ACM,
        .bmCapabilities = 0,
    },
    .union_desc = {
        .bFunctionLength = sizeof(usb_cdc_desc_union_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_UNION,
        .bMasterInterface0 = NET_ITF_COMM,
        .bSlaveInterface0 = NET_ITF_DATA,
    },
    .ep_notif = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = NET_EPADDR_COMM_IN,
        .bmAttributes = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize = NET_EPSIZE_COMM,
        .bInterval = 9,
    },
    .itf_data = {
        .bLength = sizeof(usb_desc_itf_t),
        .bDescriptorType = USB_DTYPE_INTERFACE,
        .bInterfaceNumber = NET_ITF_DATA,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0,
    },
    .ep_in = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = NET_EPADDR_DATA_IN,
        .bmAttributes = USB_EP_TYPE_BULK,
        .wMaxPacketSize = NET_EPSIZE_DATA,
        .bInterval = 0,
    },
    .ep_out = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = NET_EPADDR_DATA_OUT,
        .bmAttributes = USB_EP_TYPE_BULK,
        .wMaxPacketSize = NET_EPSIZE_DATA,
        .bInterval = 0,
    },
};

static const net_desc_config_ecm_t NET_DESC_CONFIG_ECM = {
    .config = {
        .bLength = sizeof(usb_desc_config_t),
        .bDescriptorType = USB_DTYPE_CONFIGURATION,
        .wTotalLength = sizeof(net_desc_config_ecm_t),
        .bNumInterfaces = NET_ITF_TOTAL,
        .bConfigurationValue = NET_CONFIG_ECM,
        .iConfiguration = 0,
        .bmAttributes = USB_ATTR_RESERVED,
        .bMaxPower = 100,
    },
    .iad = {
        .bLength = sizeof(usb_desc_iad_t),
        .bDescriptorType = USB_DTYPE_IAD,
        .bFirstInterface = NET_ITF_COMM,
        .bInterfaceCount = 2,
        .bFunctionClass = USB_CLASS_CDC,
        .bFunctionSubClass = USB_SUBCLASS_CDC_ETHERNET_CONTROL,
        .bFunctionProtocol = 0,
        .iFunction = 0,
    },
    .itf_comm = {
        .bLength = sizeof(usb_desc_itf_t),
        .bDescriptorType = USB_DTYPE_INTERFACE,
        .bInterfaceNumber = NET_ITF_COMM,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_CDC,
        .bInterfaceSubClass = USB_SUBCLASS_CDC_ETHERNET_CONTROL,
        .bInterfaceProtocol = 0,
        .iInterface = 0,
    },
    .header = {
        .bFunctionLength = sizeof(usb_cdc_desc_header_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_HEADER,
        .bcdCDC = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength = sizeof(usb_cdc_desc_call_mgmt_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_CALL_MGMT,
        .bmCapabilities = 0x00,
        .bDataInterface = NET_ITF_DATA,
    },
    .union_desc = {
        .bFunctionLength = sizeof(usb_cdc_desc_union_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_UNION,
        .bMasterInterface0 = NET_ITF_COMM,
        .bSlaveInterface0 = NET_ITF_DATA,
    },
    .ecm_func = {
        .bFunctionLength = sizeof(usb_cdc_desc_ecm_func_t),
        .bDescriptorType = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType = USB_DTYPE_CDC_ETHERNET,
        .iMACAddress = NET_MAC_STR_INDEX,         
        .bmEthernetStatistics = 0,
        .wMaxSegmentSize = WEBSERVER_MTU,     
        .wNumberMCFilters = 0,    
        .bNumberPowerFilters = 0, 
    },
    .ep_notif = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = NET_EPADDR_COMM_IN,
        .bmAttributes = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize = NET_EPSIZE_COMM,
        .bInterval = 9,
    },
    .itf_data = {
        .bLength = sizeof(usb_desc_itf_t),
        .bDescriptorType = USB_DTYPE_INTERFACE,
        .bInterfaceNumber = NET_ITF_DATA,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0,
    },
    .ep_in = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = NET_EPADDR_DATA_IN,
        .bmAttributes = USB_EP_TYPE_BULK,
        .wMaxPacketSize = NET_EPSIZE_DATA,
        .bInterval = 0,
    },
    .ep_out = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = NET_EPADDR_DATA_OUT,
        .bmAttributes = USB_EP_TYPE_BULK,
        .wMaxPacketSize = NET_EPSIZE_DATA,
        .bInterval = 0,
    },
};

#ifdef __cplusplus
}
#endif