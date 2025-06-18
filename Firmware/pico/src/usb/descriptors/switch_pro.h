#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SWITCH_PRO_REPORT_ID_HID            ((uint8_t)0x80)
#define SWITCH_PRO_REPORT_ID_RUMBLE_ONLY    ((uint8_t)0x10)
#define SWITCH_PRO_REPORT_ID_CMD_AND_RUMBLE ((uint8_t)0x01)

#define SWITCH_PRO_COMMAND_LED              ((uint8_t)0x30)
#define SWITCH_PRO_COMMAND_HOME_LED         ((uint8_t)0x38)
#define SWITCH_PRO_COMMAND_GYRO             ((uint8_t)0x40)
#define SWITCH_PRO_COMMAND_GYRO_ENABLE      ((uint8_t)0x01)
#define SWITCH_PRO_COMMAND_MODE             ((uint8_t)0x03)
#define SWITCH_PRO_COMMAND_MODE_FULL_REPORT ((uint8_t)0x30)
#define SWITCH_PRO_COMMAND_HANDSHAKE        ((uint8_t)0x02)
#define SWITCH_PRO_COMMAND_DISABLE_TIMEOUT  ((uint8_t)0x04)

#define SWITCH_PRO_BUTTON0_Y        ((uint8_t)1 << 0)
#define SWITCH_PRO_BUTTON0_X        ((uint8_t)1 << 1)
#define SWITCH_PRO_BUTTON0_B        ((uint8_t)1 << 2)
#define SWITCH_PRO_BUTTON0_A        ((uint8_t)1 << 3)
#define SWITCH_PRO_BUTTON0_R        ((uint8_t)1 << 6)
#define SWITCH_PRO_BUTTON0_ZR       ((uint8_t)1 << 7)

#define SWITCH_PRO_BUTTON1_MINUS    ((uint8_t)1 << 0)
#define SWITCH_PRO_BUTTON1_PLUS     ((uint8_t)1 << 1)
#define SWITCH_PRO_BUTTON1_L3       ((uint8_t)1 << 3)
#define SWITCH_PRO_BUTTON1_R3       ((uint8_t)1 << 2)
#define SWITCH_PRO_BUTTON1_HOME     ((uint8_t)1 << 4)
#define SWITCH_PRO_BUTTON1_CAPTURE  ((uint8_t)1 << 5)

#define SWITCH_PRO_BUTTON2_DOWN     ((uint8_t)1 << 0)
#define SWITCH_PRO_BUTTON2_UP       ((uint8_t)1 << 1)
#define SWITCH_PRO_BUTTON2_RIGHT    ((uint8_t)1 << 2)
#define SWITCH_PRO_BUTTON2_LEFT     ((uint8_t)1 << 3)
#define SWITCH_PRO_BUTTON2_L        ((uint8_t)1 << 6)
#define SWITCH_PRO_BUTTON2_ZL       ((uint8_t)1 << 7)

typedef struct __attribute__((packed)) {
    uint8_t report_id;
    uint8_t command;
} switch_pro_command_t;
_Static_assert(sizeof(switch_pro_command_t) == 2, "switch_pro_command_t size mismatch");

typedef struct __attribute__((packed)) {
        uint8_t report_id;
        uint8_t timer;

        union {
            struct {
                uint8_t conn_info   : 4;
                uint8_t battery     : 4; 
            };
            uint8_t info;
        };

        uint8_t buttons[3];

        // uint8_t leftX  : 12;
        // uint8_t leftY  : 12;
        // uint8_t rightX : 12;
        // uint8_t rightY : 12;
        uint8_t joysticks[6];

        uint8_t vibrator;

        uint16_t accelerX;
        uint16_t accelerY;
        uint16_t accelerZ;

        uint16_t velocityX;
        uint16_t velocityY;
        uint16_t velocityZ;
} switch_pro_report_in_t;
_Static_assert(sizeof(switch_pro_report_in_t) == 25, "switch_pro_report_in_t size mismatch");

typedef struct __attribute__((packed)) {
    uint8_t report_id;
    uint8_t sequence_counter;
    uint8_t rumble_l[4];
    uint8_t rumble_r[4];
    uint8_t commands[]; // Command id + parameters
} switch_pro_report_out_t;
_Static_assert(sizeof(switch_pro_report_out_t) == 10, "switch_pro_report_out_t size mismatch");

#ifdef __cplusplus
}
#endif