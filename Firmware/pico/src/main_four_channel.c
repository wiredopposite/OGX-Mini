#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL)

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/clocks.h>
#include "usb/device/device.h"
#include "usb/host/host.h"
#include "settings/settings.h"
#include "log/log.h"
#include "led/led.h"
#include "four_channel/slave.h"
#include "four_channel/master.h"

typedef enum {
    FOUR_CH_ROLE_MASTER = 0,
    FOUR_CH_ROLE_SLAVE,
} four_ch_role_t;

static four_ch_role_t four_ch_role = FOUR_CH_ROLE_SLAVE;
static uint8_t four_ch_index = 0;
static atomic_bool four_ch_slave_en = true;

static uint8_t get_four_ch_index(void) {
    gpio_init(FOUR_CHANNEL_PIN_SLAVE_1);
    gpio_init(FOUR_CHANNEL_PIN_SLAVE_2);
    gpio_set_dir(FOUR_CHANNEL_PIN_SLAVE_1, GPIO_IN);
    gpio_set_dir(FOUR_CHANNEL_PIN_SLAVE_2, GPIO_IN);
    gpio_pull_up(FOUR_CHANNEL_PIN_SLAVE_1);
    gpio_pull_up(FOUR_CHANNEL_PIN_SLAVE_2);

    uint8_t index = 0;
    if (gpio_get(FOUR_CHANNEL_PIN_SLAVE_1) && 
        gpio_get(FOUR_CHANNEL_PIN_SLAVE_2)) {
        index = 0;
    } else if (!gpio_get(FOUR_CHANNEL_PIN_SLAVE_1) && 
               gpio_get(FOUR_CHANNEL_PIN_SLAVE_2)) {
        index = 1;
    } else if (gpio_get(FOUR_CHANNEL_PIN_SLAVE_1) && 
               !gpio_get(FOUR_CHANNEL_PIN_SLAVE_2)) {
        index = 2;
    } else {
        index = 3;
    }
    gpio_deinit(FOUR_CHANNEL_PIN_SLAVE_1);
    gpio_deinit(FOUR_CHANNEL_PIN_SLAVE_2);
    return index;
}

static void master_host_connect_cb(uint8_t index, usbh_type_t type, bool connected) {
    ogxm_logd("Host connect callback: index=%d, type=%d, connected=%d\n", 
        index, type, connected);

    if (index > 0) {
        four_ch_master_set_connected(index, connected);
    } else {
        led_set_on(connected);
    }
}

static void master_host_set_pad_cb(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    if (index > 0) {
        // Reroute to slave if master
        four_ch_master_set_pad(index, pad, flags);
    } else {
        usb_device_set_pad(index, pad, flags);
    }
}

static void master_host_set_audio(uint8_t index, const gamepad_pcm_t* pcm) {
    if (index == 0) {
        usb_device_set_audio(index, pcm);
    }
}

static void slave_host_connect_cb(uint8_t index, usbh_type_t type, bool connected) {
    ogxm_logd("Host connect callback: index=%d, type=%d, connected=%d\n", index, type, connected);
    // Disable i2c if something gets plugged in
    atomic_store(&four_ch_slave_en, !connected);
}

static void slave_host_set_pad_cb(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    usb_device_set_pad(index, pad, flags);
}

static void slave_host_set_audio(uint8_t index, const gamepad_pcm_t* pcm) {
    usb_device_set_audio(index, pcm);
}

static void slave_set_pad_cb(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    if (atomic_load(&four_ch_slave_en)) {
        usb_device_set_pad(index, pad, flags);
    }
}

static void slave_enabled_cb(bool enabled) {
    ogxm_logd("Slave enabled callback: %d\n", enabled);
    if (atomic_load(&four_ch_slave_en)) {
        led_set_on(enabled);
    }
}   

void core1_entry(void) {
    led_init();
    led_set_on(false);

    usb_host_config_t usbh_config = {
        .multithread = true,
        .hw_type = USBH_HW_PIO,
        .connect_cb = (four_ch_role == FOUR_CH_ROLE_MASTER) 
                      ? master_host_connect_cb : slave_host_connect_cb,
        .gamepad_cb = (four_ch_role == FOUR_CH_ROLE_MASTER) 
                      ? master_host_set_pad_cb : slave_host_set_pad_cb,
        .audio_cb = (four_ch_role == FOUR_CH_ROLE_MASTER) 
                    ? master_host_set_audio : slave_host_set_audio,
    };
    
    usb_host_configure(&usbh_config);
    usb_host_enable();

    if (four_ch_role == FOUR_CH_ROLE_MASTER) {
        ogxm_logd("Running four channel as master\n");
        four_ch_master_init(usb_host_set_rumble);
    } else {
        ogxm_logd("Running four channel as slave at index %d\n", four_ch_index);
        four_ch_slave_init(four_ch_index, slave_enabled_cb, slave_set_pad_cb);
    }

    void (*four_ch_task)(void) = (four_ch_role == FOUR_CH_ROLE_MASTER) 
                                 ? four_ch_master_task : four_ch_slave_task;

    while (true) {
        usb_host_task();
        four_ch_task();
        sleep_ms(1);
    }
}

static void master_device_set_rumble_cb(uint8_t index, const gamepad_rumble_t* rumble) {
    usb_host_set_rumble(index, rumble);
}

static void master_device_set_audio_cb(uint8_t index, const gamepad_pcm_out_t* pcm_out) {
    usb_host_set_audio(index, pcm_out);
}

static void slave_device_set_rumble_cb(uint8_t index, const gamepad_rumble_t* rumble) {
    if (!atomic_load(&four_ch_slave_en)) {
        usb_host_set_rumble(index, rumble);
    } else {
        four_ch_slave_set_rumble(rumble);
    }
}

static void slave_device_set_audio_cb(uint8_t index, const gamepad_pcm_out_t* pcm_out) {
    if (!atomic_load(&four_ch_slave_en)) {
        usb_host_set_audio(index, pcm_out);
    }
}

int main(void) {
    set_sys_clock_khz(240000, true);

    four_ch_index = get_four_ch_index();
    four_ch_role =  (four_ch_index == 0) 
                    ? FOUR_CH_ROLE_MASTER : FOUR_CH_ROLE_SLAVE;

    if (four_ch_role == FOUR_CH_ROLE_MASTER) {
        ogxm_log_init(true);
    }

    settings_init();

    usbd_type_t device_type = USBD_TYPE_XBOXOG_GP;
    usb_device_config_t usbd_config = {
        .multithread = true,
        .count = 1,
        .usb = {
            {
                .type = device_type,
                .addons = 0,
            }
        },
        .rumble_cb = (four_ch_role == FOUR_CH_ROLE_MASTER) 
                     ? master_device_set_rumble_cb : slave_device_set_rumble_cb,
        .audio_cb = (four_ch_role == FOUR_CH_ROLE_MASTER) 
                    ? master_device_set_audio_cb : slave_device_set_audio_cb,
    };

    multicore_reset_core1();
    multicore_launch_core1(core1_entry);

    usb_device_configure(&usbd_config);
    usb_device_connect();

    ogxm_logd("USB device configured and connected\n");

    while (true) {
        usb_device_task();
    }
}

#endif // (OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL)