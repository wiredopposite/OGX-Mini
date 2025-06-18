#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"
#include "usb/host/host.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USBH_STATE_BUFFER_SIZE 0x200U

typedef enum {
    PERIPH_GAMEPAD = 0,
    PERIPH_CHATPAD,
    PERIPH_AUDIO,
    PERIPH_COUNT
} usbh_periph_t;

typedef void (*host_mounted_cb_t)(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer);
typedef void (*host_unmounted_cb_t)(uint8_t index, uint8_t daddr, uint8_t itf_num);
typedef void (*host_task_cb_t)(uint8_t index, uint8_t daddr, uint8_t itf_num);
typedef void (*host_report_cb_t)(uint8_t index, usbh_periph_t subtype, uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len);
typedef void (*host_report_ctrl_cb_t)(uint8_t index, uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len);

typedef void (*host_send_rumble_t)(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble);
typedef void (*host_send_audio_t)(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_pcm_out_t* pcm);

typedef struct {
    const char*             name;

    host_mounted_cb_t       mounted_cb;
    host_unmounted_cb_t     unmounted_cb;
    host_task_cb_t          task_cb;
    host_report_cb_t        report_cb;
    host_report_ctrl_cb_t   report_ctrl_cb;

    host_send_rumble_t      send_rumble;
    host_send_audio_t       send_audio;
} usb_host_driver_t;

extern const usb_host_driver_t USBH_DRIVER_XBOXOG_GP;
extern const usb_host_driver_t USBH_DRIVER_XBOXOG_XBLC;
extern const usb_host_driver_t USBH_DRIVER_XGIP_GP;
extern const usb_host_driver_t USBH_DRIVER_XGIP_AUDIO;
extern const usb_host_driver_t USBH_DRIVER_XINPUT;
extern const usb_host_driver_t USBH_DRIVER_XINPUT_WL;
extern const usb_host_driver_t USBH_DRIVER_XINPUT_AUDIO;
extern const usb_host_driver_t USBH_DRIVER_SWITCH_PRO;
extern const usb_host_driver_t USBH_DRIVER_PS3;
extern const usb_host_driver_t USBH_DRIVER_PS4;
extern const usb_host_driver_t USBH_DRIVER_PS5;
extern const usb_host_driver_t USBH_DRIVER_HID;

void usb_host_driver_connect_cb(uint8_t index, usbh_type_t type, bool connected);
void usb_host_driver_pad_cb(uint8_t index, const gamepad_pad_t* pad);
void usb_host_driver_audio_cb(uint8_t index, const gamepad_pcm_out_t* pcm);

#ifdef __cplusplus
}
#endif