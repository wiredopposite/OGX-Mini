#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <hardware/clocks.h>
#include <hardware/timer.h>
#include <hardware/watchdog.h>

#include "tusb.h"

#if defined(CONFIG_EN_USB_HOST)
#include "pio_usb.h"
#endif

#include "Board/Config.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"
#include "Board/board_api_private/board_api_private.h"
#include "TaskQueue/TaskQueue.h"

#if defined(CONFIG_EN_USB_HOST)
#include "USBHost/HostManager.h"
#endif

extern "C" {

// TUSB_OPT_TIME_CALLBACK=1: TinyUSB expects the platform to supply millis; the SDK helper may
// not be linked in this configuration. Used by our tusb_time_delay_ms_api override and TinyUSB.
uint32_t tusb_time_millis_api(void) {
    return time_us_32() / 1000u;
}

// Overrides TinyUSB's weak default (busy-wait on millis only). With PIO USB host and
// skip_alarm_pool there is no hardware 1 kHz SOF interrupt — pio_usb_host_frame() must run
// about once per millisecond. Enumeration calls tusb_time_delay_ms_api() for 50+ ms resets
// from inside tuh_task(); without servicing PIO here, SOFs stop and control transfers fail
// ("Enumeration attempt N" retries, controller never works).
void tusb_time_delay_ms_api(uint32_t ms) {
    const uint32_t start = tusb_time_millis_api();
    while ((tusb_time_millis_api() - start) < ms) {
#if defined(CONFIG_EN_USB_HOST)
        pio_usb_host_frame();
        const uint32_t now = tusb_time_millis_api();
        while (tusb_time_millis_api() == now) {
            tight_loop_contents();
        }
#endif
    }
}

} // extern "C"

namespace board_api {

mutex_t gpio_mutex_;

bool usb::host_connected() {
    if (board_api_usbh::host_connected) {
        return board_api_usbh::host_connected();
    }
    return false;
}

bool usb::host_any_pad_mounted() {
#if defined(CONFIG_EN_USB_HOST)
    return HostManager::get_instance().any_mounted();
#else
    return false;
#endif
}

//Only call this from core0
void usb::disconnect_all() {
    OGXM_LOG("Disconnecting USB and resetting Core1\n");

    TaskQueue::suspend_delayed_tasks();
#if defined(CONFIG_EN_USB_HOST)
    if (board_api_usbh::stop_pio_usb_host) {
        board_api_usbh::stop_pio_usb_host();
    } else {
        multicore_reset_core1();
    }
#else
    multicore_reset_core1();
#endif
    sleep_ms(500);
    tud_disconnect();
    sleep_ms(500);
}

// If using PicoW, only use this method from the core running btstack and after you've called init_bluetooth
void set_led(bool state) {
    mutex_enter_blocking(&gpio_mutex_);

    if (board_api_led::set_led) {
        board_api_led::set_led(state);
    }
    if (board_api_bt::set_led) {
        board_api_bt::set_led(state);
    }
    if (board_api_rgb::set_led) {
        board_api_rgb::set_led(state ? 0x00 : 0xFF, state ? 0xFF : 0x00, 0x00);
    }

    mutex_exit(&gpio_mutex_);
}

void reboot() {
    OGXM_LOG("Rebooting\n");
    /* Watchdog reset is reliable from either core (BT runs on Core1). */
    watchdog_reboot(0, 0, 0);
    while (1) {
        tight_loop_contents();
    }
}

uint32_t ms_since_boot() {
    return to_ms_since_boot(get_absolute_time());
}

//Call after board is initialized
void init_bluetooth() {
    if (board_api_bt::init) {
        board_api_bt::init();
    }
}

//Call on core0 before any other method
void init_board() {
    if (!set_sys_clock_khz(SYSCLOCK_KHZ, true)) {
        if (!set_sys_clock_khz((SYSCLOCK_KHZ / 2), true)) {
            panic("Failed to set sys clock");
        }
    }

    stdio_init_all();

    if (!mutex_is_initialized(&gpio_mutex_)) {
        mutex_init(&gpio_mutex_); 
        mutex_enter_blocking(&gpio_mutex_);

        if (ogxm_log::init) {
            ogxm_log::init();
        }
        if (board_api_led::init) {
            board_api_led::init();
        }
        if (board_api_rgb::init) {
            board_api_rgb::init();
        }
        if (board_api_usbh::init) {
            board_api_usbh::init();
        }

        mutex_exit(&gpio_mutex_);
    }
    OGXM_LOG("Board initialized\n");
}

} // namespace board_api