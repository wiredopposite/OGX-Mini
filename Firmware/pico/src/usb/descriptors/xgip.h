#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_ITF_SUBCLASS_XGIP   ((uint8_t)0x47)
#define USB_ITF_PROTOCOL_XGIP   ((uint8_t)0xD0)
#define XGIP_ITF_NUM_GAMEPAD    ((uint8_t)0x00)
#define XGIP_ITF_NUM_AUDIO      ((uint8_t)0x01)
#define XGIP_ITF_NUM_CHATPAD    ((uint8_t)0x02)

#define GIP_CMD_ACK             ((uint8_t)0x01)
#define GIP_CMD_ANNOUNCE        ((uint8_t)0x02)
#define GIP_CMD_IDENTIFY        ((uint8_t)0x04)
#define GIP_CMD_POWER           ((uint8_t)0x05)
#define GIP_CMD_AUTHENTICATE    ((uint8_t)0x06)
#define GIP_CMD_VIRTUAL_KEY     ((uint8_t)0x07)
#define GIP_CMD_RUMBLE          ((uint8_t)0x09)
#define GIP_CMD_LED             ((uint8_t)0x0a)
#define GIP_CMD_FIRMWARE        ((uint8_t)0x0c)
#define GIP_CMD_INPUT           ((uint8_t)0x20)
#define GIP_SEQ0                ((uint8_t)0x00)
#define GIP_OPT_ACK             ((uint8_t)0x10)
#define GIP_OPT_INTERNAL        ((uint8_t)0x20)

#define GIP_PL_LEN(n)           ((uint8_t)n)

#define GIP_PWR_ON              ((uint8_t)0x00)
#define GIP_PWR_SLEEP           ((uint8_t)0x01)
#define GIP_PWR_OFF             ((uint8_t)0x04)
#define GIP_PWR_RESET           ((uint8_t)0x07)
#define GIP_LED_ON              ((uint8_t)0x01)

#define GIP_MOTOR_R             ((uint8_t)1 << 0)
#define GIP_MOTOR_L             ((uint8_t)1 << 1)
#define GIP_MOTOR_RT            ((uint8_t)1 << 2)
#define GIP_MOTOR_LT            ((uint8_t)1 << 3)
#define GIP_MOTOR_ALL           (GIP_MOTOR_R | GIP_MOTOR_L | GIP_MOTOR_RT | GIP_MOTOR_LT)

static const uint8_t GIP_POWER_ON[]     = { GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(1), GIP_PWR_ON };
static const uint8_t GIP_S_INIT[]       = { GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(15), 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t GIP_S_LED_INIT[]   = { GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(3), 0x00, 0x01, 0x14 };
static const uint8_t GIP_EXTRA_INPUT_PACKET_INIT[] = { 0x4d, 0x10, GIP_SEQ0, 0x02, 0x07, 0x00 };
static const uint8_t GIP_PDP_LED_ON[]   = { GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(3), 0x00, GIP_LED_ON, 0x14 };
static const uint8_t GIP_PDP_AUTH[]     = { GIP_CMD_AUTHENTICATE, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(2), 0x01, 0x00 };
static const uint8_t GIP_RUMBLE[]       = { GIP_CMD_RUMBLE, 0x00, 0x00, GIP_PL_LEN(9), 0x00, GIP_MOTOR_ALL, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF };

#define XGIP_CLASS_COMMAND                  0x00U
#define XGIP_CLASS_LOW_LATENCY              0x01U
#define XGIP_CLASS_STANDARD_LATENCY         0x02U
#define XGIP_CLASS_AUDIO                    0x03U

#define XGIP_COMMAND_PROTOCOL_CONTROL       0x01U
#define XGIP_COMMAND_HELLO_DEVICE           0x02U
#define XGIP_COMMAND_STATUS_DEVICE          0x03U
#define XGIP_COMMAND_METADATA_RESPONSE      0x04U
#define XGIP_COMMAND_SET_DEVICE_STATE       0x05U
#define XGIP_COMMAND_SECURITY_CTRL_DATA     0x06U
#define XGIP_COMMAND_GUIDE_BTN_STATUS       0x07U
#define XGIP_COMMAND_AUDIO_CTRL             0x08U
#define XGIP_COMMAND_GAMEPAD_VIBRATION      0x09U
#define XGIP_COMMAND_LED_GUIDE_BTN          0x0AU
#define XGIP_COMMAND_EXT_COMMANDS           0x1EU
#define XGIP_COMMAND_LARGE_MSG_REQUEST      0x1FU

#define XGIP_LOW_LATENCY_GAMEPAD_INPUT      0x20U
#define XGIP_LOW_LATENCY_GAMEPAD_EXT_INPUT  0x26U

#define XGIP_AUDIO_RENDER_CAPTURE           0x60U

#define XGIP_MTU_COMMAND             64U
#define XGIP_MTU_LOW_LATENCY         64U
#define XGIP_MTU_STANDARD_LATENCY    64U
#define XGIP_MTU_AUDIO               2048U

#define XGIP_STATUS_BATTERY_LEVEL_VERY_LOW      0x00U
#define XGIP_STATUS_BATTERY_LEVEL_LOW           0x01U
#define XGIP_STATUS_BATTERY_LEVEL_MEDIUM        0x02U
#define XGIP_STATUS_BATTERY_LEVEL_FULL          0x03U
#define XGIP_STATUS_BATTERY_TYPE_ABSENT         0x00U
#define XGIP_STATUS_BATTERY_TYPE_STANDARD       0x01U
#define XGIP_STATUS_BATTERY_TYPE_RECHARGEABLE   0x02U
#define XGIP_STATUS_BATTERY_TYPE_RESERVED       0x03U
#define XGIP_STATUS_CHARGE_NOT_CHARGING         0x00U
#define XGIP_STATUS_CHARGE_CHARGING             0x01U
#define XGIP_STATUS_CHARGE_CHARGE_ERROR         0x02U
#define XGIP_STATUS_CHARGE_RESERVED             0x03U
#define XGIP_STATUS_POWER_LEVEL_OFF_RESETTING   0x00U
#define XGIP_STATUS_POWER_LEVEL_NOT_USED        0x01U
#define XGIP_STATUS_POWER_LEVEL_FULL            0x02U
#define XGIP_STATUS_POWER_LEVEL_RESERVED        0x03U

#define XGIP_STATE_START        ((uint8_t)0x00)
#define XGIP_STATE_STOP         ((uint8_t)0x01)
#define XGIP_STATE_FULL_POWER   ((uint8_t)0x03) // Wireless only
#define XGIP_STATE_OFF          ((uint8_t)0x04)
#define XGIP_STATE_QUIESCE      ((uint8_t)0x05)
#define XGIP_STATE_RESET        ((uint8_t)0x07)

#define XGIP_LED_EBUTTON_CMD_OFF            ((uint8_t)0x00)
#define XGIP_LED_EBUTTON_CMD_ON             ((uint8_t)0x01)
#define XGIP_LED_EBUTTON_CMD_BLINK_FAST     ((uint8_t)0x02)
#define XGIP_LED_EBUTTON_CMD_BLINK_SLOW     ((uint8_t)0x03)
#define XGIP_LED_EBUTTON_CMD_BLINK_CHARGING ((uint8_t)0x04)
#define XGIP_LED_EBUTTON_CMD_RAMP_TO_LEVEL  ((uint8_t)0x0D)

#define XGIP_BUTTON0_KEEP_ALIVE    ((uint8_t)1 << 1)
#define XGIP_BUTTON0_MENU          ((uint8_t)1 << 2)
#define XGIP_BUTTON0_VIEW          ((uint8_t)1 << 3)
#define XGIP_BUTTON0_A             ((uint8_t)1 << 4)
#define XGIP_BUTTON0_B             ((uint8_t)1 << 5)
#define XGIP_BUTTON0_X             ((uint8_t)1 << 6)
#define XGIP_BUTTON0_Y             ((uint8_t)1 << 7)

#define XGIP_BUTTON1_DPAD_UP       ((uint8_t)1 << 0)
#define XGIP_BUTTON1_DPAD_DOWN     ((uint8_t)1 << 1)
#define XGIP_BUTTON1_DPAD_LEFT     ((uint8_t)1 << 2)
#define XGIP_BUTTON1_DPAD_RIGHT    ((uint8_t)1 << 3)
#define XGIP_BUTTON1_LB            ((uint8_t)1 << 4)
#define XGIP_BUTTON1_RB            ((uint8_t)1 << 5)
#define XGIP_BUTTON1_L3            ((uint8_t)1 << 6)
#define XGIP_BUTTON1_R3            ((uint8_t)1 << 7)

typedef struct __attribute__((packed)) {
    union {
        struct {
            uint8_t number  : 5;
            uint8_t class   : 3; // Command/Low Latency/Standard Latency/Audio
        };
        uint8_t raw;
    } type;
    union {
        struct {
            uint8_t expansion_index : 3;
            uint8_t reserved        : 1;
            uint8_t ack_required    : 1;
            uint8_t system_msg      : 1;
            uint8_t first_fragment  : 1;
            uint8_t is_fragment     : 1;
        };
        uint8_t raw;
    } flags;
    uint8_t sequence_id;
} xgip_header_t;
_Static_assert(sizeof(xgip_header_t) == 3, "xgip_header_t is not the correct size");

typedef union __attribute__((packed)) {
    struct {
        uint8_t length : 7;
        uint8_t extended : 1;
    };
    uint8_t raw;
} xgip_payload_len_t;
_Static_assert(sizeof(xgip_payload_len_t) == 1, "xgip_payload_len_t is not the correct size");

typedef union __attribute__((packed)) {
    struct {
        uint8_t battery_level   : 2;
        uint8_t batter_type     : 2;
        uint8_t charge          : 2;
        uint8_t power_level     : 2;
    };
    uint8_t raw;
} xgip_status_t;
_Static_assert(sizeof(xgip_status_t) == 1, "xgip_status_in_t is not the correct size");

typedef union __attribute__((packed)) {
    struct {
        uint8_t active          : 1;
        uint8_t events_present  : 1;
        uint8_t reserved        : 6;
    };
    uint8_t raw;
} xgip_status_ext_t;
_Static_assert(sizeof(xgip_status_ext_t) == 1, "xgip_status_ext_t is not the correct size");

/* Device to host */

typedef struct __attribute__((packed, aligned(4))) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    uint8_t             device_id[8];
    uint16_t            vid;
    uint16_t            pid;
    uint16_t            fw_ver_major;
    uint16_t            fw_ver_minor;
    uint16_t            fw_ver_build;
    uint16_t            fw_rev;
    uint8_t             hw_ver_major;
    uint8_t             hw_ver_minor;
    uint8_t             rf_ver_major;
    uint8_t             rf_ver_minor;
    uint8_t             security_ver_major;
    uint8_t             security_ver_minor;
    uint8_t             gip_ver_major;
    uint8_t             gip_ver_minor;
} xgip_hello_device_in_t;
_Static_assert(sizeof(xgip_hello_device_in_t) == 32, "xgip_hello_device_in_t is not the correct size");

typedef struct __attribute__((packed, aligned(4))) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    xgip_status_t       status;
    xgip_status_ext_t   ext_status;
    uint8_t             reserved[2];
    uint8_t             events[]; // Optional field: events[0] is the event count if events are present
} xgip_status_in_t;
_Static_assert(sizeof(xgip_status_in_t) == 8, "xgip_status_in_t is not the correct size");

typedef struct __attribute__((packed)) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    xgip_status_t       status;
} xgip_status_legacy_in_t;
_Static_assert(sizeof(xgip_status_legacy_in_t) == 5, "xgip_status_legacy_in_t is not the correct size");

typedef struct __attribute__((packed, aligned(4))) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    xgip_payload_len_t  total_len_low;
    xgip_payload_len_t  total_len_high;
    uint8_t             data[58];
} xgip_metadata_init_in_t;
_Static_assert(sizeof(xgip_metadata_init_in_t) == 64U, "xgip_metadata_init_in_t is not the correct size");

typedef struct __attribute__((packed, aligned(4))) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len_low;
    xgip_payload_len_t  payload_len_high;
    uint8_t             fragment_offset;
    uint8_t             data[58];
} xgip_metadata_mid_in_t;
_Static_assert(sizeof(xgip_metadata_mid_in_t) == 64U, "xgip_metadata_mid_in_t is not the correct size");

typedef struct __attribute__((packed, aligned(4))) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    xgip_payload_len_t  fragment_offset_low;
    xgip_payload_len_t  fragment_offset_high;
    uint8_t             data[58];
} xgip_metadata_last_in_t;
_Static_assert(sizeof(xgip_metadata_last_in_t) == 64U, "xgip_metadata_last_in_t is not the correct size");

typedef struct __attribute__((packed)) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    xgip_payload_len_t  msg_len_low;
    xgip_payload_len_t  msg_len_high;
} xgip_metadata_complete_in_t;
_Static_assert(sizeof(xgip_metadata_complete_in_t) == 6U, "xgip_metadata_complete_in_t is not the correct size");

typedef struct __attribute__((packed)) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    uint8_t             buttons[2];
    uint16_t            trigger_l; // 0 - 1023
    uint16_t            trigger_r; // 0 - 1023
    int16_t             joystick_lx;
    int16_t             joystick_ly;
    int16_t             joystick_rx;
    int16_t             joystick_ry;
    uint8_t             ext_data[]; // Share button or vendor-specific extension data
} xgip_report_in_t;
_Static_assert(sizeof(xgip_report_in_t) == 18U, "xbone_report_in_t is not the correct size");

typedef struct __attribute__((packed)) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    uint8_t             state;
    uint8_t             unk;
} xgip_guide_button_in_t;
_Static_assert(sizeof(xgip_guide_button_in_t) == 6U, "xgip_guide_button_in_t is not the correct size");

/* Host to device */

typedef struct __attribute__((packed)) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    uint8_t             state;
} xgip_cmd_set_state_t;
_Static_assert(sizeof(xgip_cmd_set_state_t) == 5U, "xgip_cmd_set_state_t is not the correct size");

typedef struct __attribute__((packed)) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    uint8_t             command;
    uint8_t             pattern;
    uint8_t             intensity;
} xgip_cmd_led_ebutton_t;
_Static_assert(sizeof(xgip_cmd_led_ebutton_t) == 7U, "xgip_cmd_led_ebutton_t is not the correct size");

typedef struct __attribute__((packed)) {
    xgip_header_t       header;
    xgip_payload_len_t  payload_len;
    uint8_t             command;
    union {
        struct {
            uint8_t right           : 1;
            uint8_t left            : 1;
            uint8_t impulse_right   : 1;
            uint8_t impulse_left    : 1;
            uint8_t reserved        : 4;
        };
        uint8_t raw;
    } motor;
    uint8_t impulse_left;    // 0 - 100
    uint8_t impulse_right;   // 0 - 100
    uint8_t vibration_left;  // 0 - 100
    uint8_t vibration_right; // 0 - 100
    uint8_t duration_ms;     // 0 - 255, 0 means cancel
    uint8_t delay_ms;        // 0 - 255
    uint8_t repeat_count;    // 0 - 255
} xgip_cmd_vibration_t;
_Static_assert(sizeof(xgip_cmd_vibration_t) == 13U, "xgip_cmd_vibration_t is not the correct size");

#ifdef __cplusplus
} 
#endif