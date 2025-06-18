#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "common/usb_util.h"
#include "common/class/hid_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Descriptors */

#define USB_DTYPE_XINPUT                ((uint8_t)0x21)
#define USB_DTYPE_XINPUT_AUTH           ((uint8_t)0x41)
#define USB_SUBCLASS_XINPUT             ((uint8_t)0x5D)
#define USB_SUBCLASS_XINPUT_AUTH        ((uint8_t)0xFD)
#define USB_PROTOCOL_XINPUT_GP          ((uint8_t)0x01)
#define USB_PROTOCOL_XINPUT_GP_WL       ((uint8_t)0x81)
#define USB_PROTOCOL_XINPUT_AUDIO       ((uint8_t)0x03)
#define USB_PROTOCOL_XINPUT_AUDIO_WL    ((uint8_t)0x82)
#define USB_PROTOCOL_XINPUT_PLUGIN      ((uint8_t)0x02)
#define USB_PROTOCOL_XINPUT_AUTH        ((uint8_t)0x13)

#define XINPUT_EPSIZE_CTRL              ((uint8_t)8)

#define XINPUT_EPADDR_GP_IN             ((uint8_t)0x81)
#define XINPUT_EPADDR_GP_OUT            ((uint8_t)0x02)
#define XINPUT_EPADDR_AUDIO_IN          ((uint8_t)0x83)
#define XINPUT_EPADDR_AUDIO_OUT         ((uint8_t)0x04)
#define XINPUT_EPADDR_AUDIO_CTRL_IN 	((uint8_t)0x85)
#define XINPUT_EPADDR_AUDIO_CTRL_OUT 	((uint8_t)0x05)
#define XINPUT_EPADDR_PLUGIN_IN 	    ((uint8_t)0x86)

#define XINPUT_EPSIZE_GP_IN             ((uint16_t)32)
#define XINPUT_EPSIZE_GP_OUT            ((uint16_t)32)
#define XINPUT_EPSIZE_AUDIO_IN          ((uint16_t)32)
#define XINPUT_EPSIZE_AUDIO_OUT         ((uint16_t)32)
#define XINPUT_EPSIZE_AUDIO_CTRL_IN     ((uint16_t)32)
#define XINPUT_EPSIZE_AUDIO_CTRL_OUT    ((uint16_t)32)

static const usb_desc_device_t XINPUT_DESC_DEVICE = {
    .bLength = sizeof(usb_desc_device_t),
    .bDescriptorType = USB_DTYPE_DEVICE,
    .bcdUSB = USB_BCD_VERSION_2_0,
    .bDeviceClass = USB_CLASS_VENDOR,
    .bDeviceSubClass = USB_SUBCLASS_VENDOR,
    .bDeviceProtocol = USB_PROTOCOL_VENDOR,
    .bMaxPacketSize0 = XINPUT_EPSIZE_CTRL,
    .idVendor = 0x045E,
    .idProduct = 0x028E,
    .bcdDevice = 0x0110,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

typedef struct __attribute__((packed)) {
    uint8_t bLength; // Length of this descriptor.
    uint8_t bDescriptorType; // CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
    uint8_t unk0[4];
    uint8_t bEndpointAddressIn;
    uint8_t unk1[6];
    uint8_t bEndpointAddressOut;
    uint8_t unk2[3];
} usb_desc_xinput_gp_t;
static const size_t XINPUT_DESC_XINPUT_SIZE = sizeof(usb_desc_xinput_gp_t);

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t unk0[4];
    uint8_t bEndpointAddressIn;
    uint8_t unk1[2];
    uint8_t bEndpointAddressOut;
    uint8_t unk2[2];
    uint8_t bEndpointAddressIn1;
    uint8_t unk3[7];
    uint8_t bEndpointAddressOut1;
    uint8_t unk4[6];
} usb_desc_xinput_audio_t;

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t unk[4];
} usb_desc_xinput_auth_t;

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t unk0[5];
    uint8_t bEndpointAddressIn;
    uint8_t unk1;
} usb_desc_xinput_plugin_t;

typedef struct __attribute__((packed)) {
    usb_desc_config_t config;
    struct {
        usb_desc_itf_t itf;
        usb_desc_xinput_gp_t desc;
        usb_desc_endpoint_t ep_in;
        usb_desc_endpoint_t ep_out;
    } xinput;
    struct {
        usb_desc_itf_t itf;
        usb_desc_xinput_audio_t desc;
        usb_desc_endpoint_t ep_in;
        usb_desc_endpoint_t ep_out;
        usb_desc_endpoint_t ctrl_ep_in;
        usb_desc_endpoint_t ctrl_ep_out;
    } audio;
    struct {
        usb_desc_itf_t itf;
        usb_desc_xinput_plugin_t desc;
        usb_desc_endpoint_t ep_in;
    } plugin;
    struct {
        usb_desc_itf_t itf;
        usb_desc_xinput_auth_t desc;
    } auth;
} xinput_desc_config_t;

typedef enum {
    XINPUT_ITF_GP = 0,
    XINPUT_ITF_AUDIO,
    XINPUT_ITF_PLUGIN,
    XINPUT_ITF_AUTH,
    XINPUT_ITF_COUNT,
} xinput_itf_t;

static const xinput_desc_config_t XINPUT_DESC_CONFIG = {
    .config = {
        .bLength = sizeof(usb_desc_config_t),
        .bDescriptorType = USB_DTYPE_CONFIGURATION,
        .wTotalLength = sizeof(xinput_desc_config_t),
        .bNumInterfaces = XINPUT_ITF_COUNT,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = USB_ATTR_RESERVED | USB_ATTR_REMOTE_WAKEUP,
        .bMaxPower = 0xFA,
    },
    .xinput = {
        .itf = {
            .bLength = sizeof(usb_desc_itf_t),
            .bDescriptorType = USB_DTYPE_INTERFACE,
            .bInterfaceNumber = XINPUT_ITF_GP,
            .bAlternateSetting = 0,
            .bNumEndpoints = 2,
            .bInterfaceClass = USB_CLASS_VENDOR,
            .bInterfaceSubClass = USB_SUBCLASS_XINPUT,
            .bInterfaceProtocol = USB_PROTOCOL_XINPUT_GP,
            .iInterface = 0,
        },
        .desc = {
            .bLength = sizeof(usb_desc_xinput_gp_t),
            .bDescriptorType = USB_DTYPE_XINPUT,
            .unk0 = {0x00, 0x01, 0x01, 0x25},
            .bEndpointAddressIn = XINPUT_EPADDR_GP_IN,
            .unk1 = {0x14, 0x00, 0x00, 0x00, 0x00, 0x13},
            .bEndpointAddressOut = XINPUT_EPADDR_GP_OUT,
            .unk2 = {0x08, 0x00, 0x00},
        },
        .ep_in = {
            .bLength = sizeof(usb_desc_endpoint_t),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = XINPUT_EPADDR_GP_IN,
            .bmAttributes = USB_EP_TYPE_INTERRUPT,
            .wMaxPacketSize = XINPUT_EPSIZE_GP_IN,
            .bInterval = 4,
        },
        .ep_out = {
            .bLength = sizeof(usb_desc_endpoint_t),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = XINPUT_EPADDR_GP_OUT,
            .bmAttributes = USB_EP_TYPE_INTERRUPT,
            .wMaxPacketSize = XINPUT_EPSIZE_GP_OUT,
            .bInterval = 8,
        },
    },
    .audio = {
        .itf = {
            .bLength = sizeof(usb_desc_itf_t),
            .bDescriptorType = USB_DTYPE_INTERFACE,
            .bInterfaceNumber = XINPUT_ITF_AUDIO,
            .bAlternateSetting = 0,
            .bNumEndpoints = 4,
            .bInterfaceClass = USB_CLASS_VENDOR,
            .bInterfaceSubClass = USB_SUBCLASS_XINPUT,
            .bInterfaceProtocol = USB_PROTOCOL_XINPUT_AUDIO,
            .iInterface = 0,
        },
        .desc = {
            .bLength = sizeof(usb_desc_xinput_audio_t),
            .bDescriptorType = USB_DTYPE_XINPUT,
            .unk0 = {0x00, 0x01, 0x01, 0x01},
            .bEndpointAddressIn = XINPUT_EPADDR_AUDIO_IN,
            .unk1 = {0x40, 0x01},
            .bEndpointAddressOut = XINPUT_EPADDR_AUDIO_OUT,
            .unk2 = {0x20, 0x16},
            .bEndpointAddressIn1 = XINPUT_EPADDR_AUDIO_CTRL_IN,
            .unk3 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16},
            .bEndpointAddressOut1 = XINPUT_EPADDR_AUDIO_CTRL_OUT,
            .unk4 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        },
        .ep_in = {
            .bLength = sizeof(usb_desc_endpoint_t),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = XINPUT_EPADDR_AUDIO_IN,
            .bmAttributes = USB_EP_TYPE_INTERRUPT,
            .wMaxPacketSize = XINPUT_EPSIZE_AUDIO_IN,
            .bInterval = 2,
        },
        .ep_out = {
            .bLength = sizeof(usb_desc_endpoint_t),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = XINPUT_EPADDR_AUDIO_OUT,
            .bmAttributes = USB_EP_TYPE_INTERRUPT,
            .wMaxPacketSize = XINPUT_EPSIZE_AUDIO_OUT,
            .bInterval = 4,
        },
        .ctrl_ep_in = {
            .bLength = sizeof(usb_desc_endpoint_t),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = XINPUT_EPADDR_AUDIO_CTRL_IN,
            .bmAttributes = USB_EP_TYPE_INTERRUPT,
            .wMaxPacketSize = XINPUT_EPSIZE_AUDIO_CTRL_IN,
            .bInterval = 64,
        },
        .ctrl_ep_out = {
            .bLength = sizeof(usb_desc_endpoint_t),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = XINPUT_EPADDR_AUDIO_CTRL_OUT,
            .bmAttributes = USB_EP_TYPE_INTERRUPT,
            .wMaxPacketSize = XINPUT_EPSIZE_AUDIO_CTRL_OUT,
            .bInterval = 16,
        },
    },
    .plugin = {
        .itf = {
            .bLength = sizeof(usb_desc_itf_t),
            .bDescriptorType = USB_DTYPE_INTERFACE,
            .bInterfaceNumber = XINPUT_ITF_PLUGIN,
            .bAlternateSetting = 0,
            .bNumEndpoints = 1,
            .bInterfaceClass = USB_CLASS_VENDOR,
            .bInterfaceSubClass = USB_SUBCLASS_XINPUT,
            .bInterfaceProtocol = USB_PROTOCOL_XINPUT_PLUGIN,
            .iInterface = 0,
        },
        .desc = {
            .bLength = sizeof(usb_desc_xinput_plugin_t),
            .bDescriptorType = USB_DTYPE_XINPUT,
            .unk0 = {0x00, 0x01, 0x01, 0x22}, 
            .bEndpointAddressIn = XINPUT_EPADDR_PLUGIN_IN, 
            .unk1 = 0x07,
        },
        .ep_in = {
            .bLength = sizeof(usb_desc_endpoint_t),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = XINPUT_EPADDR_PLUGIN_IN,
            .bmAttributes = USB_EP_TYPE_INTERRUPT,
            .wMaxPacketSize = 32,
            .bInterval = 16,
        },
    },
    .auth = {
        .itf = {
            .bLength = sizeof(usb_desc_itf_t),
            .bDescriptorType = USB_DTYPE_INTERFACE,
            .bInterfaceNumber = XINPUT_ITF_AUTH,
            .bAlternateSetting = 0,
            .bNumEndpoints = 0,
            .bInterfaceClass = USB_CLASS_VENDOR,
            .bInterfaceSubClass = 0xfd,
            .bInterfaceProtocol = USB_PROTOCOL_XINPUT_AUTH,
            .iInterface = 4,
        },
        .desc = {
            .bLength = sizeof(usb_desc_xinput_auth_t),
            .bDescriptorType = USB_DTYPE_XINPUT_AUTH,
            .unk = {0x00, 0x01, 0x01, 0x03},
        },
    }
};

static const usb_desc_string_t XINPUT_DESC_STRING_LANGUAGE  = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t XINPUT_DESC_STRING_VENDOR    = USB_STRING_DESC("©Microsoft Corporation");
static const usb_desc_string_t XINPUT_DESC_STRING_PRODUCT   = USB_STRING_DESC("Controller");
static const usb_desc_string_t XINPUT_DESC_STRING_SERIAL    = USB_STRING_DESC("3F88628");
static const usb_desc_string_t XINPUT_DESC_STRING_AUTH      = USB_STRING_DESC("Xbox Security Method 3, Version 1.00, © 2005 Microsoft Corporation. All rights reserved.");
static const usb_desc_string_t* XINPUT_DESC_STRING[] = {
    &XINPUT_DESC_STRING_LANGUAGE,
    &XINPUT_DESC_STRING_VENDOR,
    &XINPUT_DESC_STRING_PRODUCT,
    &XINPUT_DESC_STRING_SERIAL,
    &XINPUT_DESC_STRING_AUTH,
};

/* Reports */

// #define XINPUT_WL_REPORT_SIZE               ((uint8_t)29)
// #define XINPUT_WL_GAMEPAD_REPORT_LENGTH     ((uint8_t)0x13)

#define XINPUT_REPORT_ID_OUT_RUMBLE         ((uint8_t)0x00)
#define XINPUT_REPORT_ID_OUT_LED            ((uint8_t)0x01)

// #define XINPUT_WL_STATUS_ID_CONNECT         ((uint8_t)0x08)
// #define XINPUT_WL_STATUS_ID_REPORT          ((uint8_t)0x00)

// #define XINPUT_WL_STATUS_CONNECT_FALSE      ((uint8_t)0x00)
// #define XINPUT_WL_STATUS_CONNECT_TRUE       ((uint8_t)0x80)
// #define XINPUT_WL_STATUS_REPORT_GAMEPAD     ((uint8_t)0x01)
// #define XINPUT_WL_STATUS_REPORT_CHATPAD     ((uint8_t)0x02)

// #define XINPUT_WL_REPORT_ID_GAMEPAD_IN      ((uint8_t)0x00)

#define XINPUT_CHATPAD_STATUS_PRESSED       ((uint8_t)0x00)

#define XINPUT_WL_EVENT_CONNECTION          ((uint8_t)1 << 3)
#define XINPUT_WL_STATUS_HEADSET_PRESENT    ((uint8_t)1 << 6)
#define XINPUT_WL_STATUS_CONTROLLER_PRESENT ((uint8_t)1 << 7)
#define XINPUT_WL_STATUS_PAD_STATE          ((uint8_t)1 << 0)
#define XINPUT_WL_STATUS_CHATPAD_STATE      ((uint8_t)1 << 1)

#define XINPUT_CHAT_CMD_KEEPALIVE_1         ((uint8_t)0x1F)
#define XINPUT_CHAT_CMD_KEEPALIVE_2         ((uint8_t)0x1E)
#define XINPUT_CHAT_CMD_LED_ON_KEY_PRESS    ((uint8_t)0x1B)
#define XINPUT_CHAT_CMD_CAPSLOCK_1          ((uint8_t)0x08)
#define XINPUT_CHAT_CMD_SQUARE_1            ((uint8_t)0x09)
#define XINPUT_CHAT_CMD_PEOPLE              ((uint8_t)0x0B)
#define XINPUT_CHAT_CMD_BACKLIGHT           ((uint8_t)0x0C)
#define XINPUT_CHAT_CMD_CAPSLOCK_2          ((uint8_t)0x11)
#define XINPUT_CHAT_CMD_SQUARE_2            ((uint8_t)0x12)
#define XINPUT_CHAT_CMD_SQUARE_CAPSLOCK     ((uint8_t)0x13)
#define XINPUT_CHAT_CMD_CIRCLE              ((uint8_t)0x14)
#define XINPUT_CHAT_CMD_CIRCLE_CAPSLOCK     ((uint8_t)0x15)
#define XINPUT_CHAT_CMD_CIRCLE_SQUARE       ((uint8_t)0x16)
#define XINPUT_CHAT_CMD_CIRCLE_SQUARE_CAPS  ((uint8_t)0x17)

#define XINPUT_BUTTON_UP            ((uint16_t)1 << 0)
#define XINPUT_BUTTON_DOWN          ((uint16_t)1 << 1)
#define XINPUT_BUTTON_LEFT          ((uint16_t)1 << 2)
#define XINPUT_BUTTON_RIGHT         ((uint16_t)1 << 3)
#define XINPUT_BUTTON_START         ((uint16_t)1 << 4)
#define XINPUT_BUTTON_BACK          ((uint16_t)1 << 5)
#define XINPUT_BUTTON_L3            ((uint16_t)1 << 6)
#define XINPUT_BUTTON_R3            ((uint16_t)1 << 7)
#define XINPUT_BUTTON_LB            ((uint16_t)1 << 8)
#define XINPUT_BUTTON_RB            ((uint16_t)1 << 9)
#define XINPUT_BUTTON_HOME          ((uint16_t)1 << 10)
#define XINPUT_BUTTON_SYNC          ((uint16_t)1 << 11)
#define XINPUT_BUTTON_A             ((uint16_t)1 << 12)
#define XINPUT_BUTTON_B             ((uint16_t)1 << 13)
#define XINPUT_BUTTON_X             ((uint16_t)1 << 14)
#define XINPUT_BUTTON_Y             ((uint16_t)1 << 15)

#define XINPUT_KEYCODE_1            ((uint8_t)23)
#define XINPUT_KEYCODE_2            ((uint8_t)22)
#define XINPUT_KEYCODE_3            ((uint8_t)21)
#define XINPUT_KEYCODE_4            ((uint8_t)20)
#define XINPUT_KEYCODE_5            ((uint8_t)19)
#define XINPUT_KEYCODE_6            ((uint8_t)18)
#define XINPUT_KEYCODE_7            ((uint8_t)17)
#define XINPUT_KEYCODE_8            ((uint8_t)103)
#define XINPUT_KEYCODE_9            ((uint8_t)102)
#define XINPUT_KEYCODE_0            ((uint8_t)101)

#define XINPUT_KEYCODE_Q            ((uint8_t)39)
#define XINPUT_KEYCODE_W            ((uint8_t)38)
#define XINPUT_KEYCODE_E            ((uint8_t)37)
#define XINPUT_KEYCODE_R            ((uint8_t)36)
#define XINPUT_KEYCODE_T            ((uint8_t)35)
#define XINPUT_KEYCODE_Y            ((uint8_t)34)
#define XINPUT_KEYCODE_U            ((uint8_t)33)
#define XINPUT_KEYCODE_I            ((uint8_t)118)
#define XINPUT_KEYCODE_O            ((uint8_t)117)
#define XINPUT_KEYCODE_P            ((uint8_t)100)

#define XINPUT_KEYCODE_A            ((uint8_t)55)
#define XINPUT_KEYCODE_S            ((uint8_t)54)
#define XINPUT_KEYCODE_D            ((uint8_t)53)
#define XINPUT_KEYCODE_F            ((uint8_t)52)
#define XINPUT_KEYCODE_G            ((uint8_t)51)
#define XINPUT_KEYCODE_H            ((uint8_t)50)
#define XINPUT_KEYCODE_J            ((uint8_t)49)
#define XINPUT_KEYCODE_K            ((uint8_t)19)
#define XINPUT_KEYCODE_L            ((uint8_t)14)
#define XINPUT_KEYCODE_COMMA        ((uint8_t)98)

#define XINPUT_KEYCODE_Z            ((uint8_t)70)
#define XINPUT_KEYCODE_X            ((uint8_t)69)
#define XINPUT_KEYCODE_C            ((uint8_t)68)
#define XINPUT_KEYCODE_V            ((uint8_t)67)
#define XINPUT_KEYCODE_B            ((uint8_t)66)
#define XINPUT_KEYCODE_N            ((uint8_t)65)
#define XINPUT_KEYCODE_M            ((uint8_t)82)
#define XINPUT_KEYCODE_PERIOD       ((uint8_t)83)
#define XINPUT_KEYCODE_ENTER        ((uint8_t)99)

#define XINPUT_KEYCODE_LEFT         ((uint8_t)85)
#define XINPUT_KEYCODE_SPACE        ((uint8_t)84)
#define XINPUT_KEYCODE_RIGHT        ((uint8_t)81)
#define XINPUT_KEYCODE_BACK         ((uint8_t)113)

#define XINPUT_KEYCODE_SHIFT        ((uint8_t)1)
#define XINPUT_KEYCODE_GREEN        ((uint8_t)2)
#define XINPUT_KEYCODE_ORANGE       ((uint8_t)4)
#define XINPUT_KEYCODE_MESSENGER    ((uint8_t)8)

typedef struct __attribute__((packed)) {
    uint8_t  report_id;
    uint8_t  length;
} xinput_report_header_t;

typedef struct __attribute__((packed)) {
    uint8_t  flags;
    uint8_t  status;
} xinput_wl_event_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t status;
    uint8_t buttons[3];
} xinput_chatpad_report_t;
_Static_assert(sizeof(xinput_chatpad_report_t) == 4, "xinput_chatpad_report_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t  report_id;
    uint8_t  length;
    uint16_t buttons;
    uint8_t  trigger_l;
    uint8_t  trigger_r;
    int16_t  joystick_lx;
    int16_t  joystick_ly;
    int16_t  joystick_rx;
    int16_t  joystick_ry;
    uint8_t  reserved[6];
} xinput_report_in_t;
_Static_assert(sizeof(xinput_report_in_t) == 20, "xinput_report_in_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t status_id;
    uint8_t status;
    uint8_t unk1;
    uint8_t sub_status;
    xinput_report_in_t report;
    xinput_chatpad_report_t chatpad;
} xinput_wl_report_in_t;
_Static_assert(sizeof(xinput_wl_report_in_t) == 28, "xinput_wl_report_in_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    uint8_t length;
    uint8_t led;
    uint8_t rumble_l;
    uint8_t rumble_r;
    uint8_t reserved[3];
} xinput_report_out_t;
_Static_assert(sizeof(xinput_report_out_t) == 8, "xinput_report_out_t size mismatch");

// typedef struct __attribute__((packed)) {
//     uint8_t unk[2];
//     xinput_report_out_t report;
// } xinput_wl_rumble_t;
// _Static_assert(sizeof(xinput_wl_rumble_t) == 10, "xinput_wl_rumble_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
	uint8_t report_id;
	uint8_t length;
	uint8_t data[6]; /* This can be longer but this is most we'll use */
} xinput_audio_ctrl_t;

#ifdef __cplusplus
}
#endif