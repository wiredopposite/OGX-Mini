#pragma once

#include "board_config.h"
#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"

typedef enum {
    USBH_TYPE_NONE = 0,
    USBH_TYPE_XID,
    USBH_TYPE_XBLC,  
    USBH_TYPE_XGIP,
    USBH_TYPE_XGIP_AUDIO,
    USBH_TYPE_XGIP_CHATPAD,
    USBH_TYPE_XINPUT,
    USBH_TYPE_XINPUT_AUDIO,
    USBH_TYPE_XINPUT_CHATPAD,
    USBH_TYPE_XINPUT_WL,
    USBH_TYPE_XINPUT_WL_AUDIO,
    USBH_TYPE_HID_GENERIC,
    USBH_TYPE_HID_SWITCH_PRO,
    USBH_TYPE_HID_SWITCH,
    USBH_TYPE_HID_PSCLASSIC,
    USBH_TYPE_HID_DINPUT,
    USBH_TYPE_HID_PS3,
    USBH_TYPE_HID_PS4,
    USBH_TYPE_HID_PS5,
    USBH_TYPE_HID_N64,
    USBH_TYPE_COUNT
} usbh_type_t;

typedef void (*usb_host_connect_cb_t)(uint8_t index, usbh_type_t type, bool connected);
typedef void (*usb_host_gamepad_cb_t)(uint8_t index, const gamepad_pad_t* pad, uint32_t flags);
typedef void (*usb_host_audio_cb_t)(uint8_t index, const gamepad_pcm_out_t* pcm);

typedef enum {
    USBH_HW_USB = 0,
    USBH_HW_PIO
} usbh_hw_type_t;

#if USBH_ENABLED

/**
 * @brief Initialize the USB host system.
 * 
 * @param hw_type Hardware type (USB or PIO).
 * @param connect_cb Callback for device connection events.
 * @param gamepad_cb Callback for gamepad input events.
 * @param audio_cb Callback for audio input events.
 */
void usb_host_init(usbh_hw_type_t hw_type, usb_host_connect_cb_t connect_cb, 
                   usb_host_gamepad_cb_t gamepad_cb, usb_host_audio_cb_t audio_cb);

void usb_host_task(void);

void usb_host_set_enabled(uint8_t index, bool enabled);

void usb_host_send_rumble(uint8_t index, const gamepad_rumble_t* rumble);

void usb_host_send_audio(uint8_t index, const gamepad_pcm_out_t* pcm);

#endif // USBH_ENABLED