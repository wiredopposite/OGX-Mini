#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/usb_util.h"
#include "common/class/hid_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBAPP_EPSIZE_CTRL ((uint8_t)64)
#define WEBAPP_EPSIZE_IN   ((uint16_t)32)
#define WEBAPP_EPSIZE_OUT  ((uint16_t)32)
#define WEBAPP_EPADDR_IN   ((uint8_t)1 | USB_EP_DIR_IN)
#define WEBAPP_EPADDR_OUT  ((uint8_t)2 | USB_EP_DIR_OUT)

#define WEBAPP_REQ_GET_PROFILE          ((uint8_t)0x50)
#define WEBAPP_REQ_SET_PROFILE          ((uint8_t)0x60)
#define WEBAPP_REQ_GET_DEVICE_TYPE      ((uint8_t)0x70)
#define WEBAPP_REQ_SET_DEVICE_TYPE      ((uint8_t)0x80)
#define WEBAPP_REQ_GET_GAMEPAD_INFO     ((uint8_t)0x90)

#define WEBAPP_GET_PROFILE_REPORT_ID_BY_INDEX   ((uint8_t)0x01)
#define WEBAPP_GET_PROFILE_REPORT_ID_BY_ID      ((uint8_t)0x02)

static const usb_desc_device_t WEBAPP_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_2_0,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = WEBAPP_EPSIZE_CTRL,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4006,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

static const uint8_t WEBAPP_DESC_REPORT[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x01,        //   Physical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x10,        //   Usage Maximum (0x10)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,        //   Usage (Hat switch)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x46, 0xFF, 0x00,  //   Physical Maximum (255)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x95, 0x08,        //   Report Count (8)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t   config;
    usb_desc_itf_t      itf;
    usb_desc_hid_t      hid;
    usb_desc_endpoint_t ep_out;
    usb_desc_endpoint_t ep_in;
} WEBAPP_desc_config_t;

static const WEBAPP_desc_config_t WEBAPP_DESC_CONFIG = {
    .config = {
        .bLength            = sizeof(usb_desc_config_t),
        .bDescriptorType    = USB_DTYPE_CONFIGURATION,
        .wTotalLength       = sizeof(WEBAPP_desc_config_t),
        .bNumInterfaces     = 1,
        .bConfigurationValue= 1,
        .iConfiguration     = 0,
        .bmAttributes       = USB_ATTR_RESERVED,
        .bMaxPower          = 0xFA,
    },
    .itf = {
        .bLength            = sizeof(usb_desc_itf_t),
        .bDescriptorType    = USB_DTYPE_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 2,
        .bInterfaceClass    = USB_CLASS_HID,
        .bInterfaceSubClass = 0x00,
        .bInterfaceProtocol = 0x00,
        .iInterface         = 0
    },
    .hid = {
        .bLength            = sizeof(usb_desc_hid_t),
        .bDescriptorType    = USB_DTYPE_HID,
        .bcdHID             = 0x0111,
        .bCountryCode       = 0x00,
        .bNumDescriptors    = 1,
        .bDescriptorType0   = USB_DTYPE_HID_REPORT,
        .wDescriptorLength0 = sizeof(WEBAPP_DESC_REPORT)
    },
    .ep_out = {
        .bLength            = sizeof(usb_desc_endpoint_t),
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = WEBAPP_EPADDR_OUT,
        .bmAttributes       = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize     = WEBAPP_EPSIZE_OUT,
        .bInterval          = 1
    },
    .ep_in = {
        .bLength            = sizeof(usb_desc_endpoint_t),
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = WEBAPP_EPADDR_IN,
        .bmAttributes       = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize     = WEBAPP_EPSIZE_IN,
        .bInterval          = 1
    }
};

static const usb_desc_string_t WEBAPP_DESC_STR_LANGID   = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t WEBAPP_DESC_STR_VENDOR   = USB_STRING_DESC("WiredOpposite");
static const usb_desc_string_t WEBAPP_DESC_STR_PRODUCT  = USB_STRING_DESC("OGX-Mini WebApp");
static const usb_desc_string_t* WEBAPP_DESC_STRING[] = {
    &WEBAPP_DESC_STR_LANGID,
    &WEBAPP_DESC_STR_VENDOR,
    &WEBAPP_DESC_STR_PRODUCT
};

#ifdef __cplusplus
}
#endif