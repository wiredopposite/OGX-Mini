#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_BLUETOOTH_USBH)

#include <stdatomic.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/clocks.h>
#include "log/log.h"
#include "bluetooth/bluetooth.h"
#include "usb/device/device.h"
#include "usb/host/host.h"
#include "gamepad/gamepad.h"
#include "settings/settings.h"
#include "shared.h"

atomic_bool bt_connected;

static void bt_connect_cb(uint8_t index, bool connected) {
    atomic_store(&bt_connected, connected);
    ogxm_logi("Bluetooth %s on core 0\n", connected ? "connected" : "disconnected");
}

void core1_entry(void) {
    if (!bluetooth_init(bt_connect_cb, usb_device_set_pad, usb_device_set_audio)) {
        panic("Bluetooth initialization failed on core 1!\n");
    }
    while (true) {
        bluetooth_task();
    }
}

static void uart_bridge_task(void) {
    usb_device_config_t usbd_config = {
        .multithread = false,
        .count = 1,
        .usb = {
            {
                .type = USBD_TYPE_UART_BRIDGE,
                .addons = 0,
            }
        },
        .rumble_cb = NULL,
        .audio_cb = NULL,
    };

    usb_device_configure(&usbd_config);
    usb_device_connect();
    ogxm_logi("UART Bridge started\n");

    while (true) {
        usb_device_task();
        sleep_ms(1);
    }
}

static void dev_set_rumble_cb(uint8_t index, const gamepad_rumble_t* rumble) {
    if (atomic_load(&bt_connected)) {
        bluetooth_set_rumble(index, rumble);
    } else {
        usb_host_set_rumble(index, rumble);
    }
}

static void dev_set_audio_cb(uint8_t index, const gamepad_pcm_in_t* pcm) {
    if (atomic_load(&bt_connected)) {
        bluetooth_set_audio(index, pcm);
    } else {
        usb_host_set_audio(index, pcm);
    }
}

static void host_connect_cb(uint8_t index, usbh_type_t type, bool connected) {
    ogxm_logi("USB Host %s on core 0\n", 
        connected ? "connected" : "disconnected");
}

static void host_set_pad_cb(uint8_t index, const gamepad_pad_t* pad) {
    if (!atomic_load(&bt_connected)) {
        usb_host_set_pad(index, pad);
    }
}

static void host_set_audio_cb(uint8_t index, const gamepad_pcm_out_t* pcm) {
    if (!atomic_load(&bt_connected)) {
        usb_host_set_audio(index, pcm);
    }
}

int main(void) {
    set_sys_clock_khz(240000, true);
    ogxm_log_init(true);
    settings_init();

#if VERIFY_BUILD_VERSION
    ogxm_logd("Verifying datetime...\n");
    if (!settings_valid_datetime()) {
        ogxm_logd("Invalid datetime, starting UART bridge\n");
        uart_bridge_task();
    }
    ogxm_logd("Datetime is valid, continuing...\n");
#endif

    atomic_store(&bt_connected, false);

    // usbd_type_t device_type = settings_get_device_type();
    usbd_type_t device_type = USBD_TYPE_XINPUT;
    usb_device_config_t usbd_config = {
        .multithread = true,
        .count = 1,
        .usb = {
            {
                .type = device_type,
                .addons = 0,
            }
        },
        .rumble_cb = dev_set_rumble_cb,
        .audio_cb = dev_set_audio_cb,
    };

    usb_host_config_t usbh_config = {
        .multithread = false,
        .hw_type = USBH_HW_PIO,
        .connect_cb = host_connect_cb,
        .gamepad_cb = host_set_pad_cb,
        .audio_cb = host_set_audio_cb,
    };

    usb_device_configure(&usbd_config);
    usb_device_connect();

    ogxm_logi("USB Device initialized on core 0\n");

    usb_host_configure(&usbh_config);
    usb_host_enable();

    ogxm_logi("USB Host initialized on core 0\n");

    multicore_reset_core1();
    multicore_launch_core1(core1_entry);

    ogxm_logi("Bluetooth initialized on core 1\n");

    check_pad_timer_set_enabled(true);

    while (true) {
        usb_device_task();
        usb_host_task();

        if (check_pad_time()) {
            check_pad_for_driver_change(0, device_type);
        }
    }
}

#endif // (OGXM_BOARD == OGXM_BOARD_BLUETOOTH)