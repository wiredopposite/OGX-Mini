#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/usb_util.h"
#include "common/class/hid_def.h"
#include "assert_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DINPUT_EPSIZE_CTRL ((uint8_t)64)
#define DINPUT_EPSIZE_IN   ((uint16_t)32)
#define DINPUT_EPSIZE_OUT  ((uint16_t)32)
#define DINPUT_EPADDR_IN   ((uint8_t)1 | USB_EP_DIR_IN)
#define DINPUT_EPADDR_OUT  ((uint8_t)2 | USB_EP_DIR_OUT)

static const usb_desc_device_t DINPUT_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_1_1,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = DINPUT_EPSIZE_CTRL,
    .idVendor           = 0x2563,
    .idProduct          = 0x0575,
    .bcdDevice          = 0x2000,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

static const uint8_t DINPUT_DESC_REPORT[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x01,        //   Physical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0D,        //   Report Count (13)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0D,        //   Usage Maximum (0x0D)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //   Report Count (3)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
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
    0x09, 0x21,        //   Usage (0x21)
    0x09, 0x22,        //   Usage (0x22)
    0x09, 0x23,        //   Usage (0x23)
    0x09, 0x24,        //   Usage (0x24)
    0x09, 0x25,        //   Usage (0x25)
    0x09, 0x26,        //   Usage (0x26)
    0x09, 0x27,        //   Usage (0x27)
    0x09, 0x28,        //   Usage (0x28)
    0x09, 0x29,        //   Usage (0x29)
    0x09, 0x2A,        //   Usage (0x2A)
    0x09, 0x2B,        //   Usage (0x2B)
    0x95, 0x0C,        //   Report Count (12)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x95, 0x08,        //   Report Count (8)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x46, 0xFF, 0x03,  //   Physical Maximum (1023)
    0x09, 0x2C,        //   Usage (0x2C)
    0x09, 0x2D,        //   Usage (0x2D)
    0x09, 0x2E,        //   Usage (0x2E)
    0x09, 0x2F,        //   Usage (0x2F)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t   config;
    usb_desc_itf_t      itf;
    usb_desc_hid_t      hid;
    usb_desc_endpoint_t ep_out;
    usb_desc_endpoint_t ep_in;
} dinput_desc_config_t;

static const dinput_desc_config_t DINPUT_DESC_CONFIG = {
    .config = {
        .bLength            = sizeof(usb_desc_config_t),
        .bDescriptorType    = USB_DTYPE_CONFIGURATION,
        .wTotalLength       = sizeof(dinput_desc_config_t),
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
        .bcdHID             = 0x0110,
        .bCountryCode       = 0x00,
        .bNumDescriptors    = 1,
        .bDescriptorType0   = USB_DTYPE_HID_REPORT,
        .wDescriptorLength0 = sizeof(DINPUT_DESC_REPORT)
    },
    .ep_out = {
        .bLength            = sizeof(usb_desc_endpoint_t),
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = DINPUT_EPADDR_OUT,
        .bmAttributes       = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize     = DINPUT_EPSIZE_OUT,
        .bInterval          = 10
    },
    .ep_in = {
        .bLength            = sizeof(usb_desc_endpoint_t),
        .bDescriptorType    = USB_DTYPE_ENDPOINT,
        .bEndpointAddress   = DINPUT_EPADDR_IN,
        .bmAttributes       = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize     = DINPUT_EPSIZE_IN,
        .bInterval          = 10
    }
};

static const usb_desc_string_t DINPUT_DESC_STR_LANGID = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t DINPUT_DESC_STR_MANUFACTURER = USB_STRING_DESC("SHANWAN");
static const usb_desc_string_t DINPUT_DESC_STR_PRODUCT = USB_STRING_DESC("2In1 USB Joystick");
static const usb_desc_string_t* DINPUT_DESC_STRING[] = {
    &DINPUT_DESC_STR_LANGID,
    &DINPUT_DESC_STR_MANUFACTURER,
    &DINPUT_DESC_STR_PRODUCT
};

#define DINPUT_DPAD_Msk             ((uint8_t)0x0F)

#define DINPUT_DPAD_UP              ((uint8_t)0x00)
#define DINPUT_DPAD_UP_RIGHT        ((uint8_t)0x01)
#define DINPUT_DPAD_RIGHT           ((uint8_t)0x02)
#define DINPUT_DPAD_DOWN_RIGHT      ((uint8_t)0x03)
#define DINPUT_DPAD_DOWN            ((uint8_t)0x04)
#define DINPUT_DPAD_DOWN_LEFT       ((uint8_t)0x05)
#define DINPUT_DPAD_LEFT            ((uint8_t)0x06)
#define DINPUT_DPAD_UP_LEFT         ((uint8_t)0x07)
#define DINPUT_DPAD_CENTER          ((uint8_t)0x08)

#define DINPUT_BUTTONS0_SQUARE      ((uint8_t)1 << 0)
#define DINPUT_BUTTONS0_CROSS       ((uint8_t)1 << 1)
#define DINPUT_BUTTONS0_CIRCLE      ((uint8_t)1 << 2)
#define DINPUT_BUTTONS0_TRIANGLE    ((uint8_t)1 << 3)
#define DINPUT_BUTTONS0_L1          ((uint8_t)1 << 4)
#define DINPUT_BUTTONS0_R1          ((uint8_t)1 << 5)
#define DINPUT_BUTTONS0_L2          ((uint8_t)1 << 6)
#define DINPUT_BUTTONS0_R2          ((uint8_t)1 << 7)

#define DINPUT_BUTTONS1_SELECT      ((uint8_t)1 << 0)
#define DINPUT_BUTTONS1_START       ((uint8_t)1 << 1)
#define DINPUT_BUTTONS1_L3          ((uint8_t)1 << 2)
#define DINPUT_BUTTONS1_R3          ((uint8_t)1 << 3)
#define DINPUT_BUTTONS1_SYS         ((uint8_t)1 << 4)
#define DINPUT_BUTTONS1_MISC        ((uint8_t)1 << 5)

typedef struct __attribute__((packed)) {
    uint8_t buttons[2];
    uint8_t dpad;

    uint8_t joystick_lx;
    uint8_t joystick_ly;
    uint8_t joystick_rx;
    uint8_t joystick_ry;

    uint8_t right_axis;
    uint8_t left_axis;
    uint8_t up_axis;
    uint8_t down_axis;

    uint8_t triangle_axis;
    uint8_t circle_axis;
    uint8_t cross_axis;
    uint8_t square_axis;

    uint8_t l1_axis;
    uint8_t r1_axis;
    uint8_t l2_axis;
    uint8_t r2_axis;
} dinput_report_in_t;
_STATIC_ASSERT(sizeof(dinput_report_in_t) == 19, "DInput report size exceeds buffer size");

#ifdef __cplusplus
}
#endif