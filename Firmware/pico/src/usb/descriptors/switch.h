#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/usb_util.h"
#include "common/class/hid_def.h"
#include "assert_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SWITCH_EPSIZE_CTRL ((uint8_t)64)
#define SWITCH_EPSIZE_IN   ((uint16_t)32)
#define SWITCH_EPSIZE_OUT  ((uint16_t)32)
#define SWITCH_EPADDR_IN   ((uint8_t)1 | USB_EP_DIR_IN)
#define SWITCH_EPADDR_OUT  ((uint8_t)2 | USB_EP_DIR_OUT)

static const usb_desc_device_t SWITCH_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_2_0,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = SWITCH_EPSIZE_CTRL,
    .idVendor           = 0x0F0D,
    .idProduct          = 0x0092,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

static const uint8_t SWITCH_DESC_REPORT[] = {
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
} switch_desc_config_t;

static const switch_desc_config_t SWITCH_DESC_CONFIG = {
    .config = {
        .bLength            = sizeof(usb_desc_config_t),
        .bDescriptorType    = USB_DTYPE_CONFIGURATION,
        .wTotalLength       = sizeof(switch_desc_config_t),
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
        .wDescriptorLength0 = sizeof(SWITCH_DESC_REPORT)
    },
    .ep_out = {
        .bLength            = sizeof(usb_desc_endpoint_t),
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = SWITCH_EPADDR_OUT,
        .bmAttributes       = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize     = SWITCH_EPSIZE_OUT,
        .bInterval          = 1
    },
    .ep_in = {
        .bLength            = sizeof(usb_desc_endpoint_t),
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = SWITCH_EPADDR_IN,
        .bmAttributes       = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize     = SWITCH_EPSIZE_IN,
        .bInterval          = 1
    }
};

static const usb_desc_string_t SWITCH_DESC_STR_LANGID   = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t SWITCH_DESC_STR_VENDOR   = USB_STRING_DESC("HORI CO.,LTD.");
static const usb_desc_string_t SWITCH_DESC_STR_PRODUCT  = USB_STRING_DESC("POKKEN CONTROLLER");
static const usb_desc_string_t* SWITCH_DESC_STRING[] = {
    &SWITCH_DESC_STR_LANGID,
    &SWITCH_DESC_STR_VENDOR,
    &SWITCH_DESC_STR_PRODUCT
};

#define SWITCH_DPAD_UP              ((uint8_t)0x00)
#define SWITCH_DPAD_UP_RIGHT        ((uint8_t)0x01)
#define SWITCH_DPAD_RIGHT           ((uint8_t)0x02)
#define SWITCH_DPAD_DOWN_RIGHT      ((uint8_t)0x03)
#define SWITCH_DPAD_DOWN            ((uint8_t)0x04)
#define SWITCH_DPAD_DOWN_LEFT       ((uint8_t)0x05)
#define SWITCH_DPAD_LEFT            ((uint8_t)0x06)
#define SWITCH_DPAD_UP_LEFT         ((uint8_t)0x07)
#define SWITCH_DPAD_CENTER          ((uint8_t)0x08)

#define SWITCH_BUTTON_Y             ((uint8_t)1 << 0)
#define SWITCH_BUTTON_B             ((uint8_t)1 << 1)
#define SWITCH_BUTTON_A             ((uint8_t)1 << 2)
#define SWITCH_BUTTON_X             ((uint8_t)1 << 3)
#define SWITCH_BUTTON_L             ((uint8_t)1 << 4)
#define SWITCH_BUTTON_R             ((uint8_t)1 << 5)
#define SWITCH_BUTTON_ZL            ((uint8_t)1 << 6)
#define SWITCH_BUTTON_ZR            ((uint8_t)1 << 7)
#define SWITCH_BUTTON_MINUS         ((uint8_t)1 << 8)
#define SWITCH_BUTTON_PLUS          ((uint8_t)1 << 9)
#define SWITCH_BUTTON_L3            ((uint8_t)1 << 10)
#define SWITCH_BUTTON_R3            ((uint8_t)1 << 11)
#define SWITCH_BUTTON_HOME          ((uint8_t)1 << 12)
#define SWITCH_BUTTON_CAPTURE       ((uint8_t)1 << 13)

typedef struct __attribute__((packed)) {
    uint16_t buttons;
    uint8_t dpad;
    uint8_t joystick_lx;
    uint8_t joystick_ly;
    uint8_t joystick_rx;
    uint8_t joystick_ry;
    uint8_t vendor;
} switch_report_in_t;
_STATIC_ASSERT(sizeof(switch_report_in_t) == 8, "DInput report size exceeds buffer size");

#ifdef __cplusplus
}
#endif