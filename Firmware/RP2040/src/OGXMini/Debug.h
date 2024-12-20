#ifndef _OGXM_DEBUG_H_
#define _OGXM_DEBUG_H_

#include "board_config.h"
#if defined(OGXM_DEBUG)

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <pico/mutex.h>
#include <hardware/uart.h>

namespace OGXMini {

    //Don't use this directly, use the OGXM_LOG macro
    static inline void log(const std::string& message)
    {
        static mutex_t log_mutex;

        if (!mutex_is_initialized(&log_mutex))
        {
            mutex_init(&log_mutex);
        }

        mutex_enter_blocking(&log_mutex);
        uart_puts(DEBUG_UART_PORT, message.c_str());
        mutex_exit(&log_mutex);
    }

    //Don't use this directly, use the OGXM_LOG macro
    static inline void log_hex(const uint8_t* data, size_t size)
    {
        std::ostringstream hex_stream;
        hex_stream << std::hex << std::setfill('0');
        for (uint16_t i = 0; i < size; ++i)
        {
            hex_stream << std::setw(2) << static_cast<int>(data[i]) << " ";
        }
        log(hex_stream.str());
    }

} // namespace OGXMini

#endif // defined(OGXM_DEBUG)

#if defined(OGXM_DEBUG)
    #define OGXM_LOG OGXMini::log
    #define OGXM_LOG_HEX OGXMini::log_hex
#else
    #define OGXM_LOG(...)
    #define OGXM_LOG_HEX(...)
#endif // OGXM_DEBUG

#endif // _OGXM_DEBUG_H_