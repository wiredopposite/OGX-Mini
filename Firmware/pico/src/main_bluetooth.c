#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_BLUETOOTH)

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/time.h>
#include <hardware/clocks.h>
#include "bluetooth/bluetooth.h"
#include "usb/device/device.h"
#include "gamepad/gamepad.h"
#include "settings/settings.h"
#include "button_combo.h"

volatile bool check_pad = false;

void core1_entry(void) {
    if (!bluetooth_init(NULL, usb_device_set_pad, NULL)) {
        panic("Bluetooth initialization failed on core 1\n");
        while (true) {
            sleep_ms(1000);
        }
    }
    while (true) {
        bluetooth_task();
    }
}

static bool check_pad_cb(repeating_timer_t *rt) {
    check_pad = true;
    return true;
}

static void launch_core1(void) {
    multicore_reset_core1();
    multicore_launch_core1(core1_entry);
}

int main(void) {
    set_sys_clock_khz(240000, true);
    stdio_init_all();
    // settings_init();

    // // usbd_type_t device_type = settings_get_device_type();
    // usbd_type_t device_type = USBD_TYPE_XINPUT;
    // usb_device_config_t usbd_config = {
    //     .use_mutex = true,
    //     .count = 1,
    //     .usb = {
    //         {
    //             .type = device_type,
    //             .addons = 0,
    //         }
    //     },
    //     .rumble_cb = bluetooth_set_rumble, // Thread safe
    //     .audio_cb = NULL,
    // };

    // usb_device_init(&usbd_config);
    // usb_device_connect();

    launch_core1();

    // repeating_timer_t gp_check_timer;
    // add_repeating_timer_ms(COMBO_CHECK_INTERVAL_MS, check_pad_cb, 
    //                        NULL, &gp_check_timer);

    while (true) {
        // usb_device_task();
        sleep_ms(1);

        // if (check_pad) {
        //     check_pad = false;
        //     gamepad_pad_t pad = {0};
        //     if (!usb_device_get_pad_unsafe(0, &pad)) {
        //         continue;
        //     }
        //     usbd_type_t type = check_button_combo(0, &pad);
        //     if ((type != USBD_TYPE_COUNT) && (type != device_type)) {
        //         usb_device_deinit();
        //         multicore_reset_core1();
        //         settings_store_device_type(type);
        //         usbd_config.usb[0].type = type;
        //         usb_device_init(&usbd_config);
        //         usb_device_connect();
        //         launch_core1();
        //     }
        // }
    }
}

// volatile bool check_pad = false;

// static bool check_pad_cb(repeating_timer_t *rt) {
//     check_pad = true;
//     return true;
// }

// void core1_entry(void) {
//     // usbd_type_t device_type = settings_get_device_type();
//     usbd_type_t device_type = USBD_TYPE_XINPUT; // Default to XInput
//     usb_device_config_t usbd_config = {
//         .use_mutex = true,
//         .count = 1,
//         .usb = {
//             {
//                 .type = device_type,
//                 .addons = 0,
//             }
//         },
//         .rumble_cb = bluetooth_set_rumble, // Thread safe
//         .audio_cb = NULL,
//     };

//     usb_device_init(&usbd_config);
//     usb_device_connect();

//     repeating_timer_t gp_check_timer;
//     add_repeating_timer_ms(COMBO_CHECK_INTERVAL_MS, check_pad_cb, 
//                            NULL, &gp_check_timer);

//     while (true) {
//         usb_device_task();
//         sleep_ms(1);

//         // if (check_pad) {
//         //     check_pad = false;
//         //     gamepad_pad_t pad = {0};
//         //     if (!usb_device_get_pad_unsafe(0, &pad)) {
//         //         continue;
//         //     }
//         //     usbd_type_t type = check_button_combo(0, &pad);
//         //     if ((type != USBD_TYPE_COUNT) && (type != device_type)) {
//         //         usb_device_deinit();
//         //         usbd_config.usb[0].type = type;
//         //         usb_device_init(&usbd_config);
//         //         usb_device_connect();
//         //     }
//         // }
//     }
// }

// int main(void) {
//     stdio_init_all();
//     // settings_init();

//     // multicore_reset_core1();
//     // multicore_launch_core1(core1_entry);

//     if (!bluetooth_init(NULL, usb_device_set_pad, NULL)) {
//         panic("Bluetooth initialization failed on core 1\n");
//         return -1;
//     }
//     while (true) {
//         bluetooth_task();
//     }
// }

#endif // (OGXM_BOARD == OGXM_BOARD_BLUETOOTH)