#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "usb/descriptors/xid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XBOXOG_GP_EP0_SIZE      8U
#define XBOXOG_GP_EPADDR_IN     (1U | USB_EP_DIR_IN)
#define XBOXOG_GP_EPADDR_OUT    (2U | USB_EP_DIR_OUT)
#define XBOXOG_GP_EP_SIZE_IN    32U
#define XBOXOG_GP_EP_SIZE_OUT   32U

static const usb_desc_device_t XBOXOG_GP_DESC_DEVICE = {
    .bLength = sizeof(usb_desc_device_t),
    .bDescriptorType = USB_DTYPE_DEVICE,
    .bcdUSB = USB_BCD_VERSION_1_1,
    .bDeviceClass = USB_CLASS_UNSPECIFIED,
    .bDeviceSubClass = USB_SUBCLASS_NONE,
    .bDeviceProtocol = USB_PROTOCOL_NONE,
    .bMaxPacketSize0 = XBOXOG_GP_EP0_SIZE,
    .idVendor = 0x045E,
    .idProduct = 0x0289,
    .bcdDevice = 0x0121,
    .iManufacturer = 0,
    .iProduct = 0,
    .iSerialNumber = 0,
    .bNumConfigurations = 1
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t   config;
    usb_desc_itf_t      itf;
    usb_desc_endpoint_t ep_in;
    usb_desc_endpoint_t ep_out;
} xboxog_gp_desc_config_t;

static const xboxog_gp_desc_config_t XBOXOG_GP_DESC_CONFIG = {
    .config = {
        .bLength = sizeof(usb_desc_config_t),
        .bDescriptorType = USB_DTYPE_CONFIGURATION,
        .wTotalLength = sizeof(xboxog_gp_desc_config_t),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = USB_ATTR_RESERVED,
        .bMaxPower = 0x32
    },
    .itf = {
        .bLength = sizeof(usb_desc_itf_t),
        .bDescriptorType = USB_DTYPE_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_ITF_CLASS_XID,
        .bInterfaceSubClass = USB_ITF_SUBCLASS_XID,
        .bInterfaceProtocol = 0,
        .iInterface = 0
    },
    .ep_in = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = XBOXOG_GP_EPADDR_IN,
        .bmAttributes = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize = XBOXOG_GP_EP_SIZE_IN,
        .bInterval = 4
    },
    .ep_out = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = XBOXOG_GP_EPADDR_OUT,
        .bmAttributes = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize = XBOXOG_GP_EP_SIZE_OUT,
        .bInterval = 4
    }
};

#define XBOXOG_GP_BUTTON_UP        ((uint8_t)1U << 0)
#define XBOXOG_GP_BUTTON_DOWN      ((uint8_t)1U << 1)
#define XBOXOG_GP_BUTTON_LEFT      ((uint8_t)1U << 2)
#define XBOXOG_GP_BUTTON_RIGHT     ((uint8_t)1U << 3)
#define XBOXOG_GP_BUTTON_START     ((uint8_t)1U << 4)
#define XBOXOG_GP_BUTTON_BACK      ((uint8_t)1U << 5)
#define XBOXOG_GP_BUTTON_L3        ((uint8_t)1U << 6)
#define XBOXOG_GP_BUTTON_R3        ((uint8_t)1U << 7)

typedef struct __attribute__((packed)) {
    uint8_t report_id;
    uint8_t length;
    uint8_t buttons;
    uint8_t reserved0;
    uint8_t a;
    uint8_t b;
    uint8_t x;
    uint8_t y;
    uint8_t black;
    uint8_t white;
    uint8_t trigger_l;
    uint8_t trigger_r;
    int16_t joystick_lx;
    int16_t joystick_ly;
    int16_t joystick_rx;
    int16_t joystick_ry;
} xboxog_gp_report_in_t;
_Static_assert(sizeof(xboxog_gp_report_in_t) == 20, "xboxog_gp_report_in_t size mismatch");

typedef struct __attribute__((packed)) {
    uint8_t report_id;
    uint8_t length;
    uint16_t rumble_l;
    uint16_t rumble_r;
} xboxog_gp_report_out_t;
_Static_assert(sizeof(xboxog_gp_report_out_t) == 6, "xboxog_gp_report_out_t size mismatch");

static const usb_desc_xid_t XBOXOG_GP_DESC_XID = {
    .bLength = sizeof(usb_desc_xid_t),
    .bDescriptorType = USB_DTYPE_XID,
    .bcdXid = 0x0100,
    .bType = 0x01,
    .bSubType = 0x02,
    .bMaxInputReportSize = sizeof(xboxog_gp_report_in_t),
    .bMaxOutputReportSize = sizeof(xboxog_gp_report_out_t),
    .wAlternateProductIds = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
};

static const xboxog_gp_report_in_t XBOXOG_GP_CAPABILITIES_IN = {
    .report_id = 0x00,
    .length = sizeof(xboxog_gp_report_in_t),
    .buttons = 0xFF,
    .reserved0 = 0x00,
    .a = 0xFF,
    .b = 0xFF,
    .x = 0xFF,
    .y = 0xFF,
    .black = 0xFF,
    .white = 0xFF,
    .trigger_l = 0xFF,
    .trigger_r = 0xFF,
    .joystick_lx = 0xFF,
    .joystick_ly = 0xFF,
    .joystick_rx = 0xFF,
    .joystick_ry = 0xFF
};

static const xboxog_gp_report_out_t XBOXOG_GP_CAPABILITIES_OUT = {            
    .report_id = 0x00,
    .length = sizeof(xboxog_gp_report_out_t),
    .rumble_l = 0xFFFF,
    .rumble_r = 0xFFFF,
};

#ifdef __cplusplus
}
#endif