#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "settings/settings.h"
#include "gamepad/gamepad.h"
#include "usb/host/host_private.h"
#include "usb/host/host.h"
#include "usb/host/drivers/hid/hid_usage.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HIDH_BUFFER_OUT_SIZE 64U

typedef void (*hid_init_cb_t)(uint8_t daddr, uint8_t itf_num, uint8_t* buffer_out);
typedef void (*hid_set_led_cb_t)(uint8_t led, uint8_t daddr, uint8_t itf_num, uint8_t* buffer_out);
typedef void (*hid_set_rumble_cb_t)(uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble, uint8_t* buffer_out);
typedef bool (*hid_desktop_quirk_cb_t)(uint8_t usage, uint32_t value, gamepad_pad_t* pad, const user_profile_t* profile);
typedef bool (*hid_button_quirk_cb_t)(uint8_t usage, uint32_t value, gamepad_pad_t* pad, const user_profile_t* profile);
typedef bool (*hid_vendor_quirk_cb_t)(uint8_t usage, uint32_t value, gamepad_pad_t* pad, const user_profile_t* profile);

typedef struct {
    const char*             name;
    
    hid_init_cb_t           init_cb;
    hid_set_led_cb_t        set_led_cb;
    hid_set_rumble_cb_t     set_rumble_cb;
    hid_desktop_quirk_cb_t  desktop_quirk_cb;
    hid_button_quirk_cb_t   button_quirk_cb;
    hid_vendor_quirk_cb_t   vendor_quirk_cb;

    const hid_usage_map_t*  usage_map;
} hidh_driver_t;

extern const hidh_driver_t HIDH_DRIVER_GENERIC;
extern const hidh_driver_t HIDH_DRIVER_SWITCH;
extern const hidh_driver_t HIDH_DRIVER_DINPUT;
extern const hidh_driver_t HIDH_DRIVER_PSCLASSIC;
extern const hidh_driver_t HIDH_DRIVER_N64;

#ifdef __cplusplus
}
#endif