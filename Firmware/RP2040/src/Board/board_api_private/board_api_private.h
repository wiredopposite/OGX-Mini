#ifndef BOARD_API_PRIVATE_H
#define BOARD_API_PRIVATE_H

#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>

namespace board_api_bt
{
    void init() __attribute__((weak));
    void set_led(bool state) __attribute__((weak));
}

namespace board_api_led
{
    void init() __attribute__((weak));
    void set_led(bool state) __attribute__((weak));
}

namespace board_api_rgb
{
    void init() __attribute__((weak));
    void set_led(uint8_t r, uint8_t g, uint8_t b) __attribute__((weak));
}

namespace board_api_esp32
{
    void init() __attribute__((weak));
    bool uart_bridge_mode() __attribute__((weak));
    void reset() __attribute__((weak));
    void enter_programming_mode() __attribute__((weak));
}

namespace board_api_usbh
{
    void init() __attribute__((weak));
    bool host_connected() __attribute__((weak));
}

#endif // BOARD_API_PRIVATE_H