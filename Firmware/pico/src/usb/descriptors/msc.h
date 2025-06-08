#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/usb_util.h"
#include "common/class/msc_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MSC_EPSIZE_CTRL ((uint8_t)8)
#define MSC_EPADDR_IN   ((uint8_t)0x81)
#define MSC_EPADDR_OUT  ((uint8_t)0x01)
#define MSC_EPSIZE_IN   ((uint16_t)64)
#define MSC_EPSIZE_OUT  ((uint16_t)64)

static const usb_desc_device_t MSC_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_2_0,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = MSC_EPSIZE_CTRL,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4006,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

// Frame 426: 3 bytes on wire (24 bits), 3 bytes captured (24 bits) on interface usb, id 0
// USB Link Layer
// USB URB
// CONFIGURATION DESCRIPTOR
//     bLength: 9
//     bDescriptorType: 0x02 (CONFIGURATION)
//     wTotalLength: 32
//     bNumInterfaces: 1
//     bConfigurationValue: 1
//     iConfiguration: 0
//     Configuration bmAttributes: 0x80  NOT SELF-POWERED  NO REMOTE-WAKEUP
//     bMaxPower: 30  (60mA)
// INTERFACE DESCRIPTOR (0.0): class Mass Storage
//     bLength: 9
//     bDescriptorType: 0x04 (INTERFACE)
//     bInterfaceNumber: 0
//     bAlternateSetting: 0
//     bNumEndpoints: 2
//     bInterfaceClass: Mass Storage (0x08)
//     bInterfaceSubClass: Unknown (0x42)
//     bInterfaceProtocol: Bulk-Only (BBB) Transport (0x50)
//     iInterface: 0
// ENDPOINT DESCRIPTOR
//     bLength: 7
//     bDescriptorType: 0x05 (ENDPOINT)
//     bEndpointAddress: 0x81  IN  Endpoint:1
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

// Frame 52: 60 bytes on wire (480 bits), 60 bytes captured (480 bits) on interface \\.\USBPcap1, id 0
// USB URB
// CONFIGURATION DESCRIPTOR
//     bLength: 9
//     bDescriptorType: 0x02 (CONFIGURATION)
//     wTotalLength: 32
//     bNumInterfaces: 1
//     bConfigurationValue: 1
//     iConfiguration: 0
//     Configuration bmAttributes: 0x80  NOT SELF-POWERED  NO REMOTE-WAKEUP
//     bMaxPower: 50  (100mA)
// INTERFACE DESCRIPTOR (0.0): class Mass Storage
//     bLength: 9
//     bDescriptorType: 0x04 (INTERFACE)
//     bInterfaceNumber: 0
//     bAlternateSetting: 0
//     bNumEndpoints: 2
//     bInterfaceClass: Mass Storage (0x08)
//     bInterfaceSubClass: SCSI transparent command set (0x06)
//     bInterfaceProtocol: Bulk-Only (BBB) Transport (0x50)
//     iInterface: 0
// ENDPOINT DESCRIPTOR
//     bLength: 7
//     bDescriptorType: 0x05 (ENDPOINT)
//     bEndpointAddress: 0x01  OUT  Endpoint:1
//     bmAttributes: 0x02
//     wMaxPacketSize: 512
//     bInterval: 0
// ENDPOINT DESCRIPTOR
//     bLength: 7
//     bDescriptorType: 0x05 (ENDPOINT)
//     bEndpointAddress: 0x81  IN  Endpoint:1
//     bmAttributes: 0x02
//     wMaxPacketSize: 512
//     bInterval: 0


typedef struct __attribute__((packed)) {
    usb_desc_config_t config;
    usb_desc_itf_t itf;
    usb_desc_endpoint_t ep_out;
    usb_desc_endpoint_t ep_in;
} msc_desc_config_t;

static const msc_desc_config_t MSC_DESC_CONFIG = {
    .config = {
        .bLength                = sizeof(usb_desc_config_t),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(msc_desc_config_t),
        .bNumInterfaces         = 1,
        .bConfigurationValue    = 1,
        .iConfiguration         = 0,
        .bmAttributes           = USB_ATTR_RESERVED,
        .bMaxPower              = 50, // 100mA
    },
    .itf = {
        .bLength                = sizeof(usb_desc_itf_t),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_CLASS_MSC,
        .bInterfaceSubClass     = USB_MSC_SUBCLASS_SCSI,
        .bInterfaceProtocol     = USB_PROTOCOL_BULK_ONLY,
        .iInterface             = 0
    },
    .ep_out = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = MSC_EPADDR_OUT,
        .bmAttributes           = USB_EP_TYPE_BULK,
        .wMaxPacketSize         = MSC_EPSIZE_OUT,
        .bInterval              = 0
    },
    .ep_in = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = MSC_EPADDR_IN,
        .bmAttributes           = USB_EP_TYPE_BULK,
        .wMaxPacketSize         = MSC_EPSIZE_IN,
        .bInterval              = 0
    },
};

static const usb_desc_string_t MSC_DESC_STR_LANGID       = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t MSC_DESC_STR_MANUFACTURER = USB_STRING_DESC("WiredOpposite");
static const usb_desc_string_t MSC_DESC_STR_PRODUCT      = USB_STRING_DESC("OGX-Mini MSC");
static const usb_desc_string_t* MSC_DESC_STRING[] = {
    &MSC_DESC_STR_LANGID,
    &MSC_DESC_STR_MANUFACTURER,
    &MSC_DESC_STR_PRODUCT
};

#ifdef __cplusplus
}
#endif