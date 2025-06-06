#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/class/hub_def.h"

#define USB_HUB_CHAR_XID    ((uint8_t)1 << 3)

#define HUB_XID_EPSIZE_CTRL ((uint8_t)8)
#define HUB_XID_EPSIZE_IN   ((uint8_t)1)
#define HUB_XID_EPADDR_IN   ((uint8_t)0x81)

static const usb_desc_device_t HUB_XID_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_1_1,
    .bDeviceClass       = USB_CLASS_HUB,
    .bDeviceSubClass    = USB_SUBCLASS_NONE,
    .bDeviceProtocol    = USB_PROTOCOL_HUB_FULL_SPEED,
    .bMaxPacketSize0    = HUB_XID_EPSIZE_CTRL,
    .idVendor           = 0x045E,
    .idProduct          = 0x0288,
    .bcdDevice          = 0x0121,
    .iManufacturer      = 0,
    .iProduct           = 0,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t   config;
    usb_desc_itf_t      itf;
    usb_desc_endpoint_t ep_in;
} hub_xid_desc_config_t;

static const hub_xid_desc_config_t HUB_XID_DESC_CONFIG = {
    .config = {
        .bLength                = sizeof(usb_desc_config_t),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(hub_xid_desc_config_t),
        .bNumInterfaces         = 1,
        .bConfigurationValue    = 1,
        .iConfiguration         = 0,
        .bmAttributes           = USB_ATTR_RESERVED | USB_ATTR_REMOTE_WAKEUP,
        .bMaxPower              = 50, // 100mA
    },
    .itf = {
        .bLength                = sizeof(usb_desc_itf_t),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_HUB,
        .bInterfaceSubClass     = 0,
        .bInterfaceProtocol     = 0,
        .iInterface             = 0
    },
    .ep_in = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = HUB_XID_EPADDR_IN,
        .bmAttributes           = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize         = HUB_XID_EPSIZE_IN, // Downstreams status change mask, 1 byte for <= 7 ports
        .bInterval              = 0xFF
    }
};

static const usb_desc_string_t HUB_XID_DESC_STR_LANGUAGE = USB_ARRAY_DESC(0x0409);