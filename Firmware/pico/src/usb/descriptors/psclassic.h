#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/usb_util.h"
#include "common/class/hid_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PSCLASSIC_EPSIZE_CTRL ((uint8_t)64)
#define PSCLASSIC_EPADDR_IN   ((uint8_t)0x81)
#define PSCLASSIC_EPSIZE_IN   ((uint8_t)64)

static const usb_desc_device_t PSCLASSIC_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_2_0,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = PSCLASSIC_EPSIZE_CTRL,
    .idVendor           = 0x054C,
    .idProduct          = 0x0CDA,
    .bcdDevice          = 1,
    .iManufacturer      = 2,
    .iProduct           = 0,
    .iSerialNumber      = 1,
};

static const uint8_t PSCLASSIC_DESC_REPORT[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0A,        //   Report Count (10)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0A,        //   Usage Maximum (0x0A)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x02,        //   Logical Maximum (2)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x02,        //   Physical Maximum (2)
    0x75, 0x02,        //   Report Size (2)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t config;
    usb_desc_itf_t itf;
    usb_desc_hid_t hid;
    usb_desc_endpoint_t ep_in;
} psclassic_desc_config_t;

static const psclassic_desc_config_t PSCLASSIC_DESC_CONFIG = {
    .config = {
        .bLength                = sizeof(usb_desc_config_t),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(psclassic_desc_config_t),
        .bNumInterfaces         = 1,
        .bConfigurationValue    = 1,
        .iConfiguration         = 0,
        .bmAttributes           = USB_ATTR_RESERVED | USB_ATTR_REMOTE_WAKEUP,
        .bMaxPower              = 0x32,
    },
    .itf = {
        .bLength                = sizeof(usb_desc_itf_t),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_HID,
        .bInterfaceSubClass     = 0,
        .bInterfaceProtocol     = 0,
        .iInterface             = 0,
    },
    .hid = {
        .bLength                = sizeof(usb_desc_hid_t),
        .bDescriptorType        = USB_DTYPE_HID,
        .bcdHID                 = 0x0111,
        .bCountryCode           = 0x00,
        .bNumDescriptors        = 1,
        .bDescriptorType0       = USB_DTYPE_HID_REPORT,
        .wDescriptorLength0     = sizeof(PSCLASSIC_DESC_REPORT),
    },
    .ep_in = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = PSCLASSIC_EPADDR_IN,
        .bmAttributes           = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize         = PSCLASSIC_EPSIZE_IN,
        .bInterval              = 10,
    },
};

static const usb_desc_string_t PSCLASSIC_DESC_STR_LANGID    = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t PSCLASSIC_DESC_STR_VENDOR    = USB_STRING_DESC("Sony Interactive Entertainment");
static const usb_desc_string_t PSCLASSIC_DESC_STR_PRODUCT   = USB_STRING_DESC("Controller");
static const usb_desc_string_t* PSCLASSIC_DESC_STRING[] = {
    &PSCLASSIC_DESC_STR_LANGID,
    &PSCLASSIC_DESC_STR_VENDOR,
    &PSCLASSIC_DESC_STR_PRODUCT,
};

#define PSCLASSIC_DPAD_Msk          ((uint16_t)0x3C00)

#define PSCLASSIC_DPAD_UP_LEFT      ((uint16_t)0x0000)
#define PSCLASSIC_DPAD_UP           ((uint16_t)0x0400)
#define PSCLASSIC_DPAD_UP_RIGHT     ((uint16_t)0x0800)
#define PSCLASSIC_DPAD_LEFT         ((uint16_t)0x1000)
#define PSCLASSIC_DPAD_CENTER       ((uint16_t)0x1400)
#define PSCLASSIC_DPAD_RIGHT        ((uint16_t)0x1800)
#define PSCLASSIC_DPAD_DOWN_LEFT    ((uint16_t)0x2000)
#define PSCLASSIC_DPAD_DOWN         ((uint16_t)0x2400)
#define PSCLASSIC_DPAD_DOWN_RIGHT   ((uint16_t)0x2800)

#define PSCLASSIC_BTN_TRIANGLE      ((uint16_t)1 << 0)
#define PSCLASSIC_BTN_CIRCLE        ((uint16_t)1 << 1)
#define PSCLASSIC_BTN_CROSS         ((uint16_t)1 << 2)
#define PSCLASSIC_BTN_SQUARE        ((uint16_t)1 << 3)
#define PSCLASSIC_BTN_L2            ((uint16_t)1 << 4)
#define PSCLASSIC_BTN_R2            ((uint16_t)1 << 5)
#define PSCLASSIC_BTN_L1            ((uint16_t)1 << 6)
#define PSCLASSIC_BTN_R1            ((uint16_t)1 << 7)
#define PSCLASSIC_BTN_SELECT        ((uint16_t)1 << 8)
#define PSCLASSIC_BTN_START         ((uint16_t)1 << 9)

typedef struct __attribute__((packed)) {
    uint16_t buttons;
} psclassic_report_in_t;

#ifdef __cplusplus
}
#endif