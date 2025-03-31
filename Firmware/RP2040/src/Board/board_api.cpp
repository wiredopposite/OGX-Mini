#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <pico/multicore.h>
#include <hardware/clocks.h>

#include "tusb.h"

#include "Board/Config.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"
#include "Board/board_api_private/board_api_private.h"
#include "TaskQueue/TaskQueue.h"

namespace board_api {

mutex_t gpio_mutex_;

bool usb::host_connected() {
    if (board_api_usbh::host_connected) {
        return board_api_usbh::host_connected();
    }
    return false;
}

//Only call this from core0
void usb::disconnect_all() {
    OGXM_LOG("Disconnecting USB and resetting Core1\n");

    TaskQueue::suspend_delayed_tasks();
    multicore_reset_core1();
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
    #define AIRCR_REG (*((volatile uint32_t *)(0xE000ED0C)))
    #define AIRCR_SYSRESETREQ (1 << 2)
    #define AIRCR_VECTKEY (0x5FA << 16)

    OGXM_LOG("Rebooting\n");

    AIRCR_REG = AIRCR_VECTKEY | AIRCR_SYSRESETREQ;
    while(1);
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