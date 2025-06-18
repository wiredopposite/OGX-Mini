#pragma once

#include "board_config.h"
#include <stdint.h>
#include <stdbool.h>
#include "usb/host/host_types.h"
#include "gamepad/gamepad.h"
#include "gamepad/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*usb_host_connect_cb_t)(uint8_t index, usbh_type_t type, bool connected);

typedef enum {
    USBH_HW_USB = 0,
    USBH_HW_PIO
} usbh_hw_type_t;

typedef struct {
    bool                    multithread;
    usbh_hw_type_t          hw_type;
    usb_host_connect_cb_t   connect_cb;
    gamepad_pad_cb_t        gamepad_cb;
    gamepad_pcm_cb_t        audio_cb;
} usb_host_config_t;

#if USBH_ENABLED

void usb_host_configure(const usb_host_config_t* config);

void usb_host_enable(void);

void usb_host_set_device_enabled(uint8_t index, bool enabled);

void usb_host_set_rumble(uint8_t index, const gamepad_rumble_t* rumble);

void usb_host_set_audio(uint8_t index, const gamepad_pcm_out_t* pcm);

void usb_host_task(void);

#endif // USBH_ENABLED

#ifdef __cplusplus
}
#endif