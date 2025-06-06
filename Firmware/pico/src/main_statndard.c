#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/multicore.h>
#include <string.h>
#include <stdio.h>
#include "gamepad/gamepad.h"
#include "usb/host/host.h"
#include "led/led.h"

#include "usb/descriptors/switch.h"
#include "settings/settings.h"

static const switch_report_in_t sw_report_default = {
    .buttons = 0,
    .dpad = 0x80,
    .joystick_lx = 0x80,
    .joystick_ly = 0x80,
    .joystick_rx = 0x80,
    .joystick_ry = 0x80,
    .vendor = 0x00
};

void get_button_name(uint16_t button, char* name, size_t name_size) {
    memset(name, 0, name_size);
    switch (button) {
        case GAMEPAD_BTN_A: snprintf(name, name_size, "A"); break;
        case GAMEPAD_BTN_B: snprintf(name, name_size, "B"); break;
        case GAMEPAD_BTN_X: snprintf(name, name_size, "X"); break;
        case GAMEPAD_BTN_Y: snprintf(name, name_size, "Y"); break;
        case GAMEPAD_BTN_LB: snprintf(name, name_size, "LB"); break;
        case GAMEPAD_BTN_RB: snprintf(name, name_size, "RB"); break;
        case GAMEPAD_BTN_L3: snprintf(name, name_size, "L3"); break;
        case GAMEPAD_BTN_R3: snprintf(name, name_size, "R3"); break;
        case GAMEPAD_BTN_BACK: snprintf(name, name_size, "Back"); break;
        case GAMEPAD_BTN_START: snprintf(name, name_size, "Start"); break;
        case GAMEPAD_BTN_SYS: snprintf(name, name_size, "System"); break;
        case GAMEPAD_BTN_MISC: snprintf(name, name_size, "Misc"); break;
        default: snprintf(name, name_size, "Unknown");
    }
}

void get_dpad_name(uint8_t dpad, char* name, size_t name_size) {
    memset(name, 0, name_size);
    switch (dpad) {
        case GAMEPAD_D_UP: snprintf(name, name_size, "Up"); break;
        case GAMEPAD_D_DOWN: snprintf(name, name_size, "Down"); break;
        case GAMEPAD_D_LEFT: snprintf(name, name_size, "Left"); break;
        case GAMEPAD_D_RIGHT: snprintf(name, name_size, "Right"); break;
        default: snprintf(name, name_size, "Center");
    }
}

static void usb_host_connect_cb(uint8_t index, usbh_type_t type, bool connected) {
    printf("USB Host Connect: Index %d, Type %d, Connected %d\n", index, type, connected);
}

static void usb_host_gamepad_cb(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    char button_name[16];
    char dpad_name[16];
    static gamepad_pad_t last_pad = {0};
    if (memcmp(pad, &last_pad, sizeof(gamepad_pad_t)) == 0) {
        printf("No change in gamepad state for index %d\n", index);
        return; // No change in pad state
    }
    // printf("now  GP %d:", index);
    // for (uint8_t i = 0; i < sizeof(gamepad_pad_t); i++) {
    //     printf(" %02X", ((uint8_t*)pad)[i]);
    // }
    // printf("\nprev GP %d:", index);
    // for (uint8_t i = 0; i < sizeof(gamepad_pad_t); i++) {
    //     printf(" %02X", ((uint8_t*)&last_pad)[i]);
    // }
    // printf("\n");

    if (pad->dpad) {
        printf("DPad: 0x%02X | ", pad->dpad);
        for (uint8_t i = 0; i < GAMEPAD_DPAD_BIT_COUNT; i++) {
            if (pad->dpad & (1 << i)) {
                get_dpad_name(1 << i, dpad_name, sizeof(dpad_name));
                printf("%s | ", dpad_name);
            }
        }
        printf("\n");
    }

    if (pad->buttons) {
        printf("Buttons: 0x%04X | ", pad->buttons);
        for (uint16_t i = 0; i < GAMEPAD_BTN_BIT_COUNT; i++) {
            if (pad->buttons & (1 << i)) {
                get_button_name(1 << i, button_name, sizeof(button_name));
                printf("%s | ", button_name);
            }
        }
        printf("\n");
    }
    if (pad->trigger_l || pad->trigger_r) {
        printf("Triggers: L: %d | R: %d\n", pad->trigger_l, pad->trigger_r);
    }
    memcpy(&last_pad, pad, sizeof(gamepad_pad_t));

    // // // Handle joystick positions
    // printf("Joystick LX: %d | LY: %d | RX: %d | RY: %d\n",
    //        pad->joystick_lx, pad->joystick_ly,
    //        pad->joystick_rx, pad->joystick_ry);
}

static void usb_host_audio_cb(uint8_t index, const gamepad_pcm_out_t* pcm) {
    printf("USB Host Audio Callback: Index %d, Samples %d\n", index, pcm->samples);
    // Process PCM data here if needed
}

int main(void) {
    stdio_init_all();
    printf("USB Host Example1\n");
    led_init();
    led_set_on(true);

    usb_host_init(USBH_HW_PIO, usb_host_connect_cb, usb_host_gamepad_cb, usb_host_audio_cb);
    usb_host_set_enabled(0, true);
    printf("USB Host Initialized\n");

    uint32_t start_us = time_us_32();
    uint32_t clear_us = time_us_32();
    bool clear_rumble = false;

    while (true) {
        if (time_us_32() - start_us > 5 * 1000 * 1000) {
            start_us = time_us_32();
            clear_us = time_us_32();
            usb_host_send_rumble(0, &(gamepad_rumble_t){
                .l = 0xFF, // Left motor
                .r = 0xFF, // Right motor
                .l_duration = 0,
                .r_duration = 0
            });
            clear_rumble = true;
        } else if (((time_us_32() - clear_us) > (1 * 1000 * 1000)) && clear_rumble) {
            clear_rumble = false;
            usb_host_send_rumble(0, &(gamepad_rumble_t){
                .l = 0, // Left motor
                .r = 0, // Right motor
                .l_duration = 0,
                .r_duration = 0
            });
        }
        usb_host_task();
        sleep_ms(1);
        // printf("USB Host Task Running...\n");
    }
}

// static void usb_device_rumble_cb(uint8_t index, const gamepad_rumble_t* rumble) {
//     printf("USB Device Rumble Callback: Index %d, L: %d, R: %d\n", index, rumble->l, rumble->r);
// }

// static void usb_device_audio_cb(uint8_t index, const gamepad_pcm_out_t* pcm_out) {
//     printf("USB Device Audio Callback: Index %d, Samples %d\n", index, pcm_out->samples);
//     // Process PCM data here if needed
// }

// static gamepad_handle_t* gp_handles[GAMEPADS_MAX] = { NULL };

// int main(void) {
//     stdio_init_all();

//     usb_device_config_t config = {
//         // .hw_type = USBD_HW_USB, // Use PIO for USB device emulation
//         .count = 1,
//         .usb = {
//             { .type = USBD_TYPE_XINPUT, .addons = 0 }
//         },
//         .audio_cb = usb_device_audio_cb,
//         .rumble_cb = usb_device_rumble_cb
//     };

//     gamepad_pad_t pad = {0};
//     usb_device_init(&config);
//     usb_device_connect();
//     printf("USB Device Initialized\n");

//     for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
//         gp_handles[i] = gamepad_init(i);
//         if (gp_handles[i] == NULL) {
//             printf("Failed to initialize gamepad handle %d\n", i);
//         }
//     }

//     uint32_t start_us = time_us_32();

//     while (true) {
//         if (time_us_32() - start_us > 1000*1000) {
//             pad.buttons ^= GAMEPAD_BTN_A | GAMEPAD_BTN_Y;
//             gamepad_set_pad(gp_handles[0], &pad, 0);
//             start_us = time_us_32();
//         }
//         usb_device_task(gp_handles);
//         sleep_ms(1);
//     }
// }