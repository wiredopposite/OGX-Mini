#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*app_rumble_cb_t)(uint8_t index, const gamepad_rumble_t *rumble);
typedef void (*app_audio_cb_t)(uint8_t index, const gamepad_pcm_out_t *pcm_out);

typedef enum {
    USBD_TYPE_XBOXOG_GP = 0,
    USBD_TYPE_XBOXOG_SB,
    USBD_TYPE_XBOXOG_XR,
    USBD_TYPE_XINPUT,
    // USBD_TYPE_DINPUT,
    USBD_TYPE_PS3,
    USBD_TYPE_PSCLASSIC,
    USBD_TYPE_SWITCH,
    USBD_TYPE_WEBAPP,
    USBD_TYPE_UART_BRIDGE,
    USBD_TYPE_COUNT
} usbd_type_t;

typedef enum {
    USBD_ADDON_HEADSET      = (1U << 0),
    USBD_ADDON_MEMORY_CARD  = (1U << 1),
    USBD_ADDON_CHATPAD      = (1U << 2),
} usbd_addon_t;

typedef struct {
    bool    use_mutex;      /* True if audio/pad set methods can be called from a different thread */
    uint8_t count;          /* The number of USB devices to emulate, if this is > 1, PIO USBD must be enabled */
    
    struct {
        usbd_type_t     type;   /* The type of USB device to emulate */
        uint8_t         addons; /* Bitmask of addons to use with the device, see usbd_addon_t */
    } usb[GAMEPADS_MAX];

    app_rumble_cb_t rumble_cb;  /* Callback for rumble receive events */
    app_audio_cb_t  audio_cb;   /* Callback for audio receive events */
} usb_device_config_t;

/**
 * @brief Initialize USB device driver
 * 
 * @param config Pointer to USB device configuration
 * 
 * @return true if initialization was successful, false otherwise
 */
bool usb_device_init(const usb_device_config_t* config);

/**
 * @brief Enable and connect the USB device
 * 
 * @note This function should be called after usb_device_init() 
 *       to start the USB device
 */
void usb_device_connect(void);

/**
 * @brief Disable, disconnect, and deinitialize the USB device.
 * 
 * This function will completely deinitialize the USB device 
 * and free any resources used by it.
 */
void usb_device_deinit(void);

/**
 * @brief Get the current gamepad state with mutex protection
 * 
 * @param index The index of the gamepad to get the state for
 * @param pad Pointer to a gamepad_pad_t structure to fill with the current state
 * 
 * @return true if the pad state was successfully retrieved, false otherwise
 */
bool usb_device_get_pad_safe(uint8_t index, gamepad_pad_t* pad);

/**
 * @brief Get the current gamepad state without mutex protection
 * 
 * @param index The index of the gamepad to get the state for
 * @param pad Pointer to a gamepad_pad_t structure to fill with the current state
 * 
 * @return true if the pad state was successfully retrieved, false otherwise
 */
bool usb_device_get_pad_unsafe(uint8_t index, gamepad_pad_t* pad);

/**
 * @brief Set the gamepad state
 * 
 * @param index The index of the gamepad to set the state for
 * @param pad Pointer to a gamepad_pad_t structure containing the new state
 * @param flags Flags to indicate which parts of the pad are being set
 */
void usb_device_set_pad(uint8_t index, const gamepad_pad_t* pad, uint32_t flags);

/**
 * @brief Set the audio output for a gamepad
 * 
 * @param index The index of the gamepad to set the audio for
 * @param pcm_out Pointer to a gamepad_pcm_out_t structure containing the audio data
 */
void usb_device_set_audio(uint8_t index, const gamepad_pcm_out_t* pcm_out);

/**
 * @brief USB device task
 * 
 * This function should be called periodically to handle USB events.
 */
void usb_device_task(void);

#ifdef __cplusplus
}
#endif