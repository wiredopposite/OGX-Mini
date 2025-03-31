#include "Board/Config.h"
#if defined(CONFIG_OGXM_DEBUG)

#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <pico/mutex.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>

#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"
#include "Board/ogxm_log.h"

std::ostream& operator<<(std::ostream& os, DeviceDriverType type) {
    switch (type) {
        case DeviceDriverType::NONE:          os << "NONE"; break;
        case DeviceDriverType::XBOXOG:        os << "XBOXOG"; break;
        case DeviceDriverType::XBOXOG_SB:     os << "XBOXOG_SB"; break;
        case DeviceDriverType::XBOXOG_XR:     os << "XBOXOG_XR"; break;
        case DeviceDriverType::XINPUT:        os << "XINPUT"; break;
        case DeviceDriverType::PS3:           os << "PS3"; break;
        case DeviceDriverType::DINPUT:        os << "DINPUT"; break;
        case DeviceDriverType::PSCLASSIC:     os << "PSCLASSIC"; break;
        case DeviceDriverType::SWITCH:        os << "SWITCH"; break;
        case DeviceDriverType::WEBAPP:        os << "WEBAPP"; break;
        case DeviceDriverType::UART_BRIDGE:   os << "UART_BRIDGE"; break;
        default:                              os << "UNKNOWN"; break;
    }
    return os;
}

namespace ogxm_log {

void init() {
    uart_init(DEBUG_UART_PORT, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function(PICO_DEFAULT_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PICO_DEFAULT_UART_RX_PIN, GPIO_FUNC_UART);
}

void log(const std::string& message) {
    static mutex_t log_mutex;

    if (!mutex_is_initialized(&log_mutex)) {
        mutex_init(&log_mutex);
    }

    mutex_enter_blocking(&log_mutex);

    std::string formatted_msg = "OGXM: " + message;

    uart_puts(DEBUG_UART_PORT, formatted_msg.c_str());

    mutex_exit(&log_mutex);
}

void log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    std::string formatted_msg = std::string(buffer);

    log(formatted_msg);

    va_end(args);
}

void log_hex(const uint8_t* data, size_t size) {
    std::ostringstream hex_stream;
    hex_stream << std::hex << std::setfill('0');
    int count = 0;
    for (uint16_t i = 0; i < size; ++i) {
        hex_stream << std::setw(2) << static_cast<int>(data[i]) << " ";
        if (++count == 16) {
            hex_stream << "\n";
            count = 0;
        }
    }
    hex_stream << "\n";
    log(hex_stream.str());
}

} // namespace ogxm_log

#endif // defined(CONFIG_OGXM_DEBUG)