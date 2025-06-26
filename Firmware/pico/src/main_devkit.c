#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_DEVKIT)

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/clocks.h>
#include "led/led.h"
#include "usb/device/device.h"
#include "usb/host/host.h"
#include "settings/settings.h"
#include "log/log.h"
#include "shared.h"

static void host_connect_cb(uint8_t index, usbh_type_t type, bool connected) {
    ogxm_logi("USB host %s at index %d\n", 
        connected ? "connected" : "disconnected", index);
    led_set_on(connected);
}

void core1_entry(void) {
    led_init();
    led_set_on(false);

    usb_host_config_t usbh_config = {
        .multithread = true,
        .hw_type = USBH_HW_PIO,
        .connect_cb = host_connect_cb,
        .gamepad_cb = usb_device_set_pad,
        .audio_cb = usb_device_set_audio,
    };
    
    usb_host_configure(&usbh_config);
    usb_host_enable();

    while (true) {
        usb_host_task();
        sleep_ms(1);
    }
}

int main(void) {
    set_sys_clock_khz(240000, true);
    ogxm_log_init(true);
    // settings_init();

    // core1_entry();
    
    // usbd_type_t device_type = settings_get_device_type();
    // usbd_type_t device_type = USBD_TYPE_XINPUT;
    usbd_type_t device_type = USBD_TYPE_WEBAPP;
    usb_device_config_t usbd_config = {
        .multithread = true,
        .count = 1,
        .usb = {
            {
                .type = device_type,
                .addons = 0,
            }
        },
        .rumble_cb = usb_host_set_rumble,
        .audio_cb = usb_host_set_audio,
    };

    // multicore_reset_core1();
    // multicore_launch_core1(core1_entry);

    usb_device_configure(&usbd_config);
    usb_device_connect();

    // check_pad_timer_set_enabled(true);

    while (true) {
        usb_device_task();
        // sleep_ms(1);

        // if (check_pad_time()) {
        //     check_pad_for_driver_change(0, device_type);
        // }
    }
}
// #include "sdcard/sd_spi.h"

// int main(void) {
//     set_sys_clock_khz(240000, true);
//     ogxm_log_init(true);
    
//     sd_error_t err = sd_spi_init();
//     if (err != SD_ERR_OKAY) {
//         ogxm_loge("SD card initialization failed: %d\n", err);
//         return -1;
//     }
//     ogxm_logi("SD card initialized successfully\n");
//     uint32_t block_count = 0;
//     err = sd_spi_get_block_count(&block_count);
//     if (err != SD_ERR_OKAY) {
//         ogxm_loge("Failed to get block count: %d\n", err);
//         return -1;
//     }
//     ogxm_logi("SD card block count: %u\n", block_count);

//     while (true) {
//         // usb_device_task();
//         sleep_ms(1);

//         // if (check_pad_time()) {
//         //     check_pad_for_driver_change(0, device_type);
//         // }
//     }
// }

#endif // (OGXM_BOARD == OGXM_BOARD_DEVKIT)