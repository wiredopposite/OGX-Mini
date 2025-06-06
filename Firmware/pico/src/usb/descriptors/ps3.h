#pragma once

#include <stdint.h>
#include "common/usb_util.h"
#include "common/usb_def.h"
#include "common/class/hid_def.h"
#include "assert_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Descriptors */

#define PS3_VID         0x054C
#define PS3_PID         0x0268
#define PS3_EP0_SIZE    64U
#define PS3_EPADDR_IN   0x81
#define PS3_EPADDR_OUT  0x02
#define PS3_EP_SIZE_IN  64U
#define PS3_EP_SIZE_OUT 64U

static const usb_desc_device_t PS3_DESC_DEVICE = {
    .bLength = sizeof(usb_desc_device_t),
    .bDescriptorType = USB_DTYPE_DEVICE,
    .bcdUSB = USB_BCD_VERSION_2_0,
    .bDeviceClass = USB_CLASS_UNSPECIFIED,
    .bDeviceSubClass = USB_SUBCLASS_NONE,
    .bDeviceProtocol = USB_PROTOCOL_NONE,
    .bMaxPacketSize0 = PS3_EP0_SIZE,
    .idVendor = PS3_VID,
    .idProduct = PS3_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

static const uint8_t PS3_DESC_REPORT[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04,        // Usage (Joystick)
    0xA1, 0x01,        // Collection (Physical)
    0xA1, 0x02,        //   Collection (Application)
    0x85, 0x01,        //     Report ID (1)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
                    //     NOTE: reserved byte
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x13,        //     Report Count (19)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x35, 0x00,        //     Physical Minimum (0)
    0x45, 0x01,        //     Physical Maximum (1)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x13,        //     Usage Maximum (0x13)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x0D,        //     Report Count (13)
    0x06, 0x00, 0xFF,  //     Usage Page (Vendor Defined 0xFF00)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
                    //     NOTE: 32 bit integer, where 0:18 are buttons and 19:31 are reserved
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x01,        //     Usage (Pointer)
    0xA1, 0x00,        //     Collection (Undefined)
    0x75, 0x08,        //       Report Size (8)
    0x95, 0x04,        //       Report Count (4)
    0x35, 0x00,        //       Physical Minimum (0)
    0x46, 0xFF, 0x00,  //       Physical Maximum (255)
    0x09, 0x30,        //       Usage (X)
    0x09, 0x31,        //       Usage (Y)
    0x09, 0x32,        //       Usage (Z)
    0x09, 0x35,        //       Usage (Rz)
    0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
                    //       NOTE: four joysticks
    0xC0,              //     End Collection
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x27,        //     Report Count (39)
    0x09, 0x01,        //     Usage (Pointer)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x30,        //     Report Count (48)
    0x09, 0x01,        //     Usage (Pointer)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x30,        //     Report Count (48)
    0x09, 0x01,        //     Usage (Pointer)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0xA1, 0x02,        //   Collection (Application)
    0x85, 0x02,        //     Report ID (2)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x30,        //     Report Count (48)
    0x09, 0x01,        //     Usage (Pointer)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0xA1, 0x02,        //   Collection (Application)
    0x85, 0xEE,        //     Report ID (238)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x30,        //     Report Count (48)
    0x09, 0x01,        //     Usage (Pointer)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0xA1, 0x02,        //   Collection (Application)
    0x85, 0xEF,        //     Report ID (239)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x30,        //     Report Count (48)
    0x09, 0x01,        //     Usage (Pointer)
    0xB1, 0x02,        //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0xC0,              // End Collection
};

typedef struct __attribute__((packed)) {
    usb_desc_config_t config;
    usb_desc_itf_t itf;
    usb_desc_hid_t hid;
    usb_desc_endpoint_t ep_in;
    usb_desc_endpoint_t ep_out;
} ps3_desc_config_t;

static const ps3_desc_config_t PS3_DESC_CONFIG = {
    .config = {
        .bLength = sizeof(usb_desc_config_t),
        .bDescriptorType = USB_DTYPE_CONFIGURATION,
        .wTotalLength = sizeof(ps3_desc_config_t),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = USB_ATTR_RESERVED,
        .bMaxPower = 0xFA, // 100mA
    },
    .itf = {
        .bLength = sizeof(usb_desc_itf_t),
        .bDescriptorType = USB_DTYPE_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CLASS_HID,
        .bInterfaceSubClass = USB_SUBCLASS_NONE,
        .bInterfaceProtocol = USB_PROTOCOL_NONE,
        .iInterface = 0,
    },
    .hid = {
        .bLength = sizeof(usb_desc_hid_t),
        .bDescriptorType = USB_DTYPE_HID,
        .bcdHID = 0x0111,
        .bCountryCode = 0x00,
        .bNumDescriptors = 1,
        .bDescriptorType0 = USB_DTYPE_HID_REPORT,
        .wDescriptorLength0 = sizeof(PS3_DESC_REPORT),
    },
    .ep_in = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = PS3_EPADDR_OUT,
        .bmAttributes = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize = PS3_EP_SIZE_OUT,
        .bInterval = 1,
    },
    .ep_out = {
        .bLength = sizeof(usb_desc_endpoint_t),
        .bDescriptorType = USB_DTYPE_ENDPOINT,
        .bEndpointAddress = PS3_EPADDR_IN,
        .bmAttributes = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize = PS3_EP_SIZE_IN,
        .bInterval = 1,
    },
};

static const usb_desc_string_t PS3_DESC_STR_LANGUAGE     = USB_ARRAY_DESC(0x0409);
static const usb_desc_string_t PS3_DESC_STR_MANUFACTURER = USB_STRING_DESC("Sony");
static const usb_desc_string_t PS3_DESC_STR_PRODUCT      = USB_STRING_DESC("PlayStation(R)3 Controller");
static const usb_desc_string_t* PS3_DESC_STRING[] = {
    &PS3_DESC_STR_LANGUAGE,
    &PS3_DESC_STR_MANUFACTURER,
    &PS3_DESC_STR_PRODUCT,
};

/* Reports */

#define PS3_REPORT_ID_IN        ((uint8_t)0x01)
#define PS3_REPORT_ID_OUT       ((uint8_t)0x01)

#define PS3_JOYSTICK_MIN        ((uint8_t)0)
#define PS3_JOYSTICK_MID        ((uint8_t)127)
#define PS3_JOYSTICK_MAX        ((uint8_t)255)

#define PS3_SIXAXIS_MID         ((int16_t)0xFF01)

#define PS3_BTN0_SELECT         ((uint8_t)0x01)
#define PS3_BTN0_L3             ((uint8_t)0x02)
#define PS3_BTN0_R3             ((uint8_t)0x04)
#define PS3_BTN0_START          ((uint8_t)0x08)
#define PS3_BTN0_DPAD_UP        ((uint8_t)0x10)
#define PS3_BTN0_DPAD_RIGHT     ((uint8_t)0x20)
#define PS3_BTN0_DPAD_DOWN      ((uint8_t)0x40)
#define PS3_BTN0_DPAD_LEFT      ((uint8_t)0x80)

#define PS3_BTN1_L2             ((uint8_t)0x01)
#define PS3_BTN1_R2             ((uint8_t)0x02)
#define PS3_BTN1_L1             ((uint8_t)0x04)
#define PS3_BTN1_R1             ((uint8_t)0x08)
#define PS3_BTN1_TRIANGLE       ((uint8_t)0x10)
#define PS3_BTN1_CIRCLE         ((uint8_t)0x20)
#define PS3_BTN1_CROSS          ((uint8_t)0x40)
#define PS3_BTN1_SQUARE         ((uint8_t)0x80)

#define PS3_BTN2_SYS            ((uint8_t)0x01)

#define PS3_PLUGGED             ((uint8_t)0x02)
#define PS3_UNPLUGGED           ((uint8_t)0x03)

#define PS3_POWER_CHARGING      ((uint8_t)0xEE)
#define PS3_POWER_NOT_CHARGING  ((uint8_t)0xF1)
#define PS3_POWER_SHUTDOWN      ((uint8_t)0x01)
#define PS3_POWER_DISCHARGING   ((uint8_t)0x02)
#define PS3_POWER_LOW           ((uint8_t)0x03)
#define PS3_POWER_HIGH          ((uint8_t)0x04)
#define PS3_POWER_FULL          ((uint8_t)0x05)

#define PS3_RUMBLE_WIRED_ON     ((uint8_t)0x10)
#define PS3_RUMBLE_WIRED_OFF    ((uint8_t)0x12)
#define PS3_RUMBLE_BATTERY_ON   ((uint8_t)0x14)
#define PS3_RUMBLE_BATTERY_OFF  ((uint8_t)0x16)

#define PS3_REPORT_IN_SIZE      ((uint16_t)49)

#define PS3_INFO_MODEL_DS3      ((uint8_t)0x04)
#define PS3_INFO_MODEL_SIXAXIS  ((uint8_t)0x03)
#define PS3_INFO_DEFAULT_FW     ((uint16_t)0x0C03)
#define PS3_BT_INFO_SERIAL      ((uint32_t)0x01D88150)
#define PS3_BT_INFO_PCB_REV     ((uint8_t)0x8A)

#define PS3_REQ_OUTPUT_REPORT_ID                ((uint8_t)0x01)
#define PS3_REQ_FEATURE_REPORT_ID_DS3_INFO      ((uint8_t)0x01)
#define PS3_REQ_FEATURE_REPORT_ID_MOTION_CALIB  ((uint8_t)0xEF)
#define PS3_REQ_FEATURE_REPORT_ID_STORAGE       ((uint8_t)0xF1)
#define PS3_REQ_FEATURE_REPORT_ID_BT_INFO       ((uint8_t)0xF2)
#define PS3_REQ_FEATURE_REPORT_ID_COMMAND       ((uint8_t)0xF4)
#define PS3_REQ_FEATURE_REPORT_ID_BT_PAIRING    ((uint8_t)0xF5)
#define PS3_REQ_FEATURE_REPORT_ID_UNK_F7        ((uint8_t)0xF7)
#define PS3_REQ_FEATURE_REPORT_ID_UNK_F8        ((uint8_t)0xF8)
#define PS3_REQ_FEATURE_REPORT_ID_UNK_F9        ((uint8_t)0xF9)
#define PS3_COMMAND_REPORT_ID                   ((uint8_t)0x42)
#define PS3_COMMAND_INPUT_DISABLE               ((uint8_t)0x01)
#define PS3_COMMAND_INPUT_ENABLE                ((uint8_t)0x02)
#define PS3_COMMAND_SENSORS_ENABLE              ((uint8_t)0x03)
#define PS3_COMMAND_RESTART                     ((uint8_t)0x04)
#define PS3_STORAGE_REPORT_ID                   ((uint8_t)0x00)
#define PS3_STORAGE_CMD_READ                    ((uint8_t)0x0B)
#define PS3_STORAGE_CMD_WRITE                   ((uint8_t)0x0A)
#define PS3_STORAGE_CMD_READ_RESP_REPORT_ID     ((uint8_t)0x57)

typedef struct __attribute__((packed)) {
    uint8_t report_id;
    uint8_t command;
} ps3_cmd_header_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t unk0;
    uint8_t command;
    uint8_t unk1[2];
    uint8_t bank;
    uint8_t address;
    union {
        struct {
            uint8_t len;
            uint8_t payload[PS3_EP0_SIZE - 7];
        } write;
        struct {
            uint8_t unk2;
            uint8_t len;
            uint8_t unk3;
        } read;
    };
} ps3_cmd_storage_t;
_STATIC_ASSERT(sizeof(ps3_cmd_storage_t) == 64, "ps3_cmd_storage_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    uint8_t command;
    uint8_t unk0[2];
    uint8_t len;
    uint8_t payload[PS3_EP0_SIZE - 6];
} ps3_cmd_storage_resp_t;
_STATIC_ASSERT(sizeof(ps3_cmd_storage_resp_t) == 64, "ps3_cmd_storage_resp_t size mismatch");

typedef struct __attribute__((packed)) {
    uint8_t deadzone;
    uint8_t gain;
} ps3_deadzone_gain_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t reserved0;
    uint8_t report_id;
    uint8_t model;
    uint8_t unk1;
    uint16_t fw_version;
    uint8_t unk3[2];
    struct {
        uint8_t lx;
        uint8_t ly;
        uint8_t rx;
        uint8_t ry;
    } stick_mid;
    uint8_t calibration[8];
    uint8_t unk4[2];
    ps3_deadzone_gain_t lx;
    ps3_deadzone_gain_t ly;
    ps3_deadzone_gain_t rx;
    ps3_deadzone_gain_t ry;
    uint8_t unk5[34];
} ps3_info_t;
_STATIC_ASSERT(sizeof(ps3_info_t) == 64, "ps3_info_t size mismatch");

typedef struct __attribute__((packed)) {
    uint8_t report_id;
    uint8_t unk0[3];
    uint8_t bdaddr[6];
    uint8_t unk1[2];
    uint32_t serial;
    uint8_t pcb_rev;
} ps3_bt_info_t;
_STATIC_ASSERT(sizeof(ps3_bt_info_t) == 17, "ps3_bt_info_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    uint8_t unk0[2];
    uint8_t host_mac_addr[6];
    uint8_t unk1[55];
} ps3_bt_pairing_t;
_STATIC_ASSERT(sizeof(ps3_bt_pairing_t) == 64, "ps3_bt_pairing_t size mismatch");

typedef struct __attribute__((packed)) {
    int16_t bias;
    int16_t gain;
} ps3_bias_gain_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    uint8_t unk0[5];
    uint8_t save_byte;
    uint8_t unk1[9];
    ps3_bias_gain_t accel_x;
    ps3_bias_gain_t accel_y;
    ps3_bias_gain_t accel_z;
    ps3_bias_gain_t gyro_z;
    uint8_t unk2[64-33];
} ps3_motion_calib_t;
static const size_t siziziz = offsetof(ps3_motion_calib_t, accel_x);
_STATIC_ASSERT(sizeof(ps3_motion_calib_t) == 64, "ps3_motion_calib_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    uint8_t reserved1;
    uint8_t buttons[3];
    uint8_t unk0;
    uint8_t joystick_lx;
    uint8_t joystick_ly;
    uint8_t joystick_rx;
    uint8_t joystick_ry;
    uint8_t unk1[4];
    uint8_t a_up;
    uint8_t a_right;
    uint8_t a_down;
    uint8_t a_left;
    uint8_t a_l2;
    uint8_t a_r2;
    uint8_t a_l1;
    uint8_t a_r1;
    uint8_t a_triangle;
    uint8_t a_circle;
    uint8_t a_cross;
    uint8_t a_square;
    uint8_t unk2[3];
    uint8_t plugged;
    uint8_t power_status;
    uint8_t rumble_status;
    uint8_t unk3[9];
    // union {
    //     struct {
    //         uint8_t acc_x[2];
    //         uint8_t acc_y[2];
    //         uint8_t acc_z[2];
    //         uint8_t gyro[2];
    //     } raw;
    //     struct {
            int16_t accel_x; // Accelerometer X-axis
            int16_t accel_y; // Accelerometer Y-axis
            int16_t accel_z; // Accelerometer Z-axis
            int16_t gyro_z;  // Gyroscope Z-axis
    //     } sixaxis;
    // };
    uint8_t align[3]; // Padding
} ps3_sixaxis_report_in_t;
_STATIC_ASSERT(sizeof(ps3_sixaxis_report_in_t) == 52, "ps3_sixaxis_report_in_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    struct {
        uint8_t r_duration; /* Right motor duration (0xff means forever) */
        uint8_t r_on;       /* Right (small) motor on/off, only supports values of 0 or 1 (off/on) */
        uint8_t l_duration; /* Left motor duration (0xff means forever) */
        uint8_t l_force;    /* left (large) motor, supports force values from 0 to 255 */
    } rumble;
    uint8_t reserved1[4];
    uint8_t leds_bitmap;      /* bitmap of enabled LEDs: LED_1 = 0x02, LED_2 = 0x04, ... */
    struct {
        uint8_t time_enabled; /* the total time the led is active (0xff means forever) */
        uint8_t duty_length;  /* how long a cycle is in deciseconds (0 means "really fast") */
        uint8_t enabled;
        uint8_t duty_off;     /* % of duty_length the led is off (0xff means 100%) */
        uint8_t duty_on;      /* % of duty_length the led is on (0xff mean 100%) */
    } leds[5];                /* LEDx at (4 - x), last is not used */
    uint8_t reserved2[13];
} ps3_report_out_t;
_STATIC_ASSERT(sizeof(ps3_report_out_t) == 48, "ps3_report_out_t size mismatch");

// static const uint8_t PS3_DEFAULT_OUT_REPORT[] = {
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0xFF, 0x27, 0x10, 0x00, 0x32,
//     0xFF, 0x27, 0x10, 0x00, 0x32,
//     0xFF, 0x27, 0x10, 0x00, 0x32,
//     0xFF, 0x27, 0x10, 0x00, 0x32,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };
// _STATIC_ASSERT(sizeof(PS3_DEFAULT_OUT_REPORT) == sizeof(ps3_report_out_t), "PS3_DEFAULT_OUT_REPORT size mismatch");

static const uint8_t PS3_DEFAULT_STORAGE_BANK_A[] = {
    0x01, 0x04, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0xEE, 0x02, 0x01, 0x03, 0xEF, 0x04, 0x01, 0x03,
    0x00, 0x00, 0x01, 0x64, 0x19, 0x01, 0x00, 0x64, 0x00, 0x01, 0x90, 0x00, 0x19, 0xFE, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x60, 0x80, 0x20, 0x15, 0x01,
    0xE4, 0x58, 0x4F, 0x5F, 0x51, 0x00, 0x02, 0x68, 0x68, 0x5A, 0x52, 0x5E, 0x58, 0x10, 0xB0, 0x03,
    0xFF, 0x00, 0x00, 0xFF, 0x44, 0x44, 0x00, 0x31, 0x03, 0xB0, 0x00, 0x6B, 0x03, 0xDA, 0x00, 0x33,
    0x03, 0xB5, 0x00, 0x65, 0x03, 0xCF, 0x00, 0xFF, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x01, 0x02, 0x18, 0x18, 0x18, 0x18, 0x09, 0x0A, 0x10, 0x11, 0x12, 0x13, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00,
    0x03, 0x00, 0x01, 0x02, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x00,
    0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x62, 0x01, 0x02, 0x01, 0x5E, 0x00, 0x32, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x20, 0x20, 0x02,
    0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6C, 0x6D, 0x6D, 0x6E, 0x6E,
    0x6F, 0x70, 0x71, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7D, 0x7F, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8B,
    0x8D, 0x8E, 0x90, 0x92, 0x93, 0x95, 0x97, 0x99, 0x9A, 0x9C, 0x9E, 0x9F, 0xA1, 0xA2, 0xA4, 0xA5,
    0xA7, 0xA8, 0xAA, 0xAB, 0xAD, 0xAE, 0xB0, 0xB1, 0xB3, 0xB4, 0xB6, 0xB7, 0xB9, 0xBB, 0xBC, 0xBE,
    0xBF, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCF, 0xD0, 0xD1,
};

static const uint8_t PS3_DEFAULT_STORAGE_BANK_B[] = {
    0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDF, 0xE1, 0xE2, 0xE3,
    0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3,
    0xF4, 0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xFA,
    0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD,
    0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
    0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0xE4, 0x58, 0x4F, 0x5F, 0x51, 0x00, 0x02, 0x68, 0x68, 0x5A, 0x52, 0x5E, 0x58, 0x10, 0xB0,
    0x03, 0xFF, 0x00, 0x00, 0xFF, 0x44, 0x44, 0x02, 0x6E, 0x02, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x31, 0x03, 0xB0, 0x00, 0x6B, 0x03, 0xDA, 0x00, 0x33, 0x03, 0xB5, 0x00, 0x65, 0x03, 0xCF,
    0x01, 0xFC, 0x01, 0x8B, 0x02, 0x00, 0x01, 0x93, 0x01, 0xFF, 0x01, 0x8C, 0x01, 0xE9, 0x00, 0x00,
    0x02, 0x6E, 0x02, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x12, 0x18, 0x01, 0x01, 0x07, 0x12, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
};

#ifdef __cplusplus
}
#endif