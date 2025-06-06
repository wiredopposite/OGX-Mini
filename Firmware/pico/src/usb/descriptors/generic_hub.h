#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/class/hub_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUB_GEN_EPSIZE_CTRL ((uint8_t)64)
#define HUB_GEN_EPSIZE_IN   ((uint8_t)1)
#define HUB_GEN_EPADDR_IN   ((uint8_t)0x81)

static const usb_desc_device_t HUB_GEN_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_1_1,
    .bDeviceClass       = USB_CLASS_HUB,
    .bDeviceSubClass    = USB_SUBCLASS_HUB,
    .bDeviceProtocol    = USB_PROTOCOL_HUB_FULL_SPEED,
    .bMaxPacketSize0    = HUB_GEN_EPSIZE_CTRL,
    .idVendor           = 0x1A40,
    .idProduct          = 0x0101,
    .bcdDevice          = 0x0111,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t   config;
    usb_desc_itf_t      hub_itf;
    usb_desc_endpoint_t hub_ep;
} hub_gen_desc_config_t;

static const hub_gen_desc_config_t HUB_GEN_DESC_CONFIG = {
    .config = {
        .bLength                = sizeof(usb_desc_config_t),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(hub_gen_desc_config_t),
        .bNumInterfaces         = 1,
        .bConfigurationValue    = 1,
        .iConfiguration         = 0,
        .bmAttributes           = USB_ATTR_RESERVED      | 
                                  USB_ATTR_REMOTE_WAKEUP | 
                                  USB_ATTR_BUS_POWERED,
        .bMaxPower              = 50, // 100mA
    },
    .hub_itf = {
        .bLength                = sizeof(usb_desc_itf_t),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_HUB,
        .bInterfaceSubClass     = USB_SUBCLASS_NONE,
        .bInterfaceProtocol     = USB_PROTOCOL_NONE,
        .iInterface             = 0
    },
    .hub_ep = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = HUB_GEN_EPADDR_IN,
        .bmAttributes           = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize         = HUB_GEN_EPSIZE_IN, // Downstreams status change mask, 1 byte for <= 7 ports
        .bInterval              = 0xFF
    }
};

static const usb_desc_string_t HUB_GEN_DESC_STR_LANGUAGE     = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t HUB_GEN_DESC_STR_MANUFACTURER = USB_STRING_DESC("WiredOpposite");
static const usb_desc_string_t HUB_GEN_DESC_STR_PRODUCT      = USB_STRING_DESC("PIO USB Hub");
static const usb_desc_string_t* HUB_GEN_DESC_STR[] = {
    &HUB_GEN_DESC_STR_LANGUAGE,
    &HUB_GEN_DESC_STR_MANUFACTURER,
    &HUB_GEN_DESC_STR_PRODUCT
};

#ifdef __cplusplus
}
#endif