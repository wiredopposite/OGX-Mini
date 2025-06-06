#pragma once

#include <stdint.h>
#include "common/usb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XBLC_EPSIZE_CTRL ((uint8_t)8)
#define XBLC_EPSIZE_IN   ((uint16_t)48)
#define XBLC_EPSIZE_OUT  ((uint16_t)48)
#define XBLC_EPADDR_IN   ((uint8_t)0x85)
#define XBLC_EPADDR_OUT  ((uint8_t)0x04)
#define XBLC_ITF_CLASS   ((uint8_t)0x78)

static const usb_desc_device_t XBLC_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_1_1,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = XBLC_EPSIZE_CTRL,
    .idVendor           = 0x045E,
    .idProduct          = 0x0283,
    .bcdDevice          = 0x0158,
    .iManufacturer      = 0,
    .iProduct           = 0,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t   config;
    usb_desc_itf_t      itf_out;
    usb_desc_endpoint_t ep_out;
    uint8_t             reserved0[2];
    usb_desc_itf_t      itf_in;
    usb_desc_endpoint_t ep_in;
    uint8_t             reserved1[2];
} xblc_desc_config_t;

static const xblc_desc_config_t XBLC_DESC_CONFIG = {
    .config = {
        .bLength             = sizeof(usb_desc_config_t),
        .bDescriptorType     = USB_DTYPE_CONFIGURATION,
        .wTotalLength        = sizeof(xblc_desc_config_t),
        .bNumInterfaces      = 2,
        .bConfigurationValue = 1,
        .iConfiguration      = 0,
        .bmAttributes        = USB_ATTR_RESERVED,
        .bMaxPower           = 0x32
    },
    .itf_out = {
        .bLength            = sizeof(usb_desc_itf_t),
        .bDescriptorType    = USB_DTYPE_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 1,
        .bInterfaceClass    = XBLC_ITF_CLASS,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface         = 0
    },
    .ep_out = {
        .bLength            = sizeof(usb_desc_endpoint_t) + 2,
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = XBLC_EPADDR_OUT,
        .bmAttributes       = USB_EP_TYPE_ISOCHRONUS | USB_EP_ATTR_ASYNC,
        .wMaxPacketSize     = XBLC_EPSIZE_OUT,
        .bInterval          = 1
    },
    .reserved0 = { 0, 0 },
    .itf_in = {
        .bLength            = sizeof(usb_desc_itf_t),
        .bDescriptorType    = USB_DTYPE_INTERFACE,
        .bInterfaceNumber   = 1,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 1,
        .bInterfaceClass    = XBLC_ITF_CLASS,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface         = 0
    },
    .ep_in = {
        .bLength            = sizeof(usb_desc_endpoint_t) + 2,
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = XBLC_EPADDR_IN,
        .bmAttributes       = USB_EP_TYPE_ISOCHRONUS | USB_EP_ATTR_ASYNC,
        .wMaxPacketSize     = XBLC_EPSIZE_IN,
        .bInterval          = 1
    },
    .reserved1 = { 0, 0 }
};

#ifdef __cplusplus
}
#endif