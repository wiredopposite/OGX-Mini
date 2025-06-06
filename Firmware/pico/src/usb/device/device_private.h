#pragma once

#include <stdint.h>
#include "usbd/usbd.h"
#include "gamepad/gamepad.h"
#include "usb/device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USBD_STATUS_BUF_SIZE 0x500U

typedef union {
    struct {
        usbd_hw_type_t      hw_type;
        uint32_t            addons;
        uint8_t*            status_buffer;
    } usb;
    struct {
        usbd_hw_type_t      hw_type;
        uint32_t            addons;
        uint8_t*            status_buffer[USBD_DEVICES_MAX];
    } xboxog_hub;
} usb_device_driver_cfg_t;

typedef usbd_handle_t* (*driver_init_t)(const usb_device_driver_cfg_t* config);
typedef void (*driver_deinit_t)(usbd_handle_t* handle);
typedef void (*driver_set_pad_t)(usbd_handle_t* handle, const gamepad_pad_t* pad, uint32_t flags);
typedef void (*driver_set_audio_t)(usbd_handle_t* handle, const gamepad_pcm_out_t* pcm_out);
typedef void (*driver_task_t)(usbd_handle_t* handle);

typedef struct {
    char*               name;
    driver_init_t       init;
    driver_task_t       task;
    driver_set_audio_t  set_audio;
    driver_set_pad_t    set_pad;
} usb_device_driver_t;

extern const usb_device_driver_t USBD_DRIVER_XBOXOG_GP;
extern const usb_device_driver_t USBD_DRIVER_XBOXOG_SB;
extern const usb_device_driver_t USBD_DRIVER_XBOXOG_XR;
extern const usb_device_driver_t USBD_DRIVER_XINPUT;
extern const usb_device_driver_t USBD_DRIVER_DINPUT;
extern const usb_device_driver_t USBD_DRIVER_PS3;
extern const usb_device_driver_t USBD_DRIVER_PSCLASSIC;
extern const usb_device_driver_t USBD_DRIVER_SWITCH;
extern const usb_device_driver_t USBD_DRIVER_WEBAPP;
extern const usb_device_driver_t USBD_DRIVER_UART_BRIDGE;
extern const usb_device_driver_t USBD_DRIVER_XBOXOG_HUB;
extern const usb_device_driver_t USBD_DRIVER_XBOXOG_XMU;
extern const usb_device_driver_t USBD_DRIVER_XBOXOG_XBLC;

void usb_device_rumble_cb(usbd_handle_t* handle, const gamepad_rumble_t* rumble);
void usb_device_audio_cb(usbd_handle_t* handle, const gamepad_pcm_out_t* pcm_out);

#ifdef __cplusplus
}
#endif