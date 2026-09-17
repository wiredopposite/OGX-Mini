#ifndef BOARD_API_PRIVATE_H
#define BOARD_API_PRIVATE_H

#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>

namespace board_api_bt {
    void init() __attribute__((weak));
    void set_led(bool state) __attribute__((weak));
}

namespace board_api_led {
    void init() __attribute__((weak));
    void set_led(bool state) __attribute__((weak));
}

namespace board_api_rgb {
    void init() __attribute__((weak));
    void set_led(uint8_t r, uint8_t g, uint8_t b) __attribute__((weak));
}

namespace board_api_usbh {
    void init() __attribute__((weak));
    bool host_connected() __attribute__((weak));
    /** Stop edge IRQs on D+/D− before PIO USB reclaims pins (avoids IO_IRQ storm / BT stall). */
    void suspend_line_irq();
    /** While PIO owns D+/D−, update the same flag GPIO IRQs used (from hcd_port_connect_status). */
    void store_host_line_connected(bool connected);
    /** After tuh_deinit / PIO release path, re-enable D+/D− edge IRQs for the next cable attach. */
    void enable_host_line_irq_monitoring();
    /** Stop PIO USB host (SOF timer, tuh_deinit) before core reset / reboot. Standard boards. */
    void stop_pio_usb_host() __attribute__((weak));
}

#endif // BOARD_API_PRIVATE_H