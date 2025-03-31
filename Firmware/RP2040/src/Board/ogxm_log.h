#ifndef BOARD_API_LOG_H
#define BOARD_API_LOG_H

#include <cstdint>

#include "Board/Config.h"
#if defined(CONFIG_OGXM_DEBUG)

#include <string>
#include <sstream>
#include <iostream>
#include <stdarg.h>

#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"

std::ostream& operator<<(std::ostream& os, DeviceDriverType type);

namespace ogxm_log {
    void init() __attribute__((weak));
    //Don't use this directly, use the OGXM_LOG macro
    void log(const std::string& message);
    //Don't use this directly, use the OGXM_LOG macro
    void log(const char* fmt, ...);
    //Don't use this directly, use the OGXM_LOG_HEX macro
    void log_hex(const uint8_t* data, size_t size);

    template <typename T>
    std::string to_string(const T& value) {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }
}

#define OGXM_LOG ogxm_log::log
#define OGXM_LOG_HEX ogxm_log::log_hex
#define OGXM_ASSERT(x) if (!(x)) { OGXM_LOG("Assertion failed: " #x); while(1); }
#define OGXM_ASSERT_MSG(x, msg) if (!(x)) { OGXM_LOG("Assertion failed: " #x " " msg); while(1); }
#define OGXM_TO_STRING ogxm_log::to_string

#else // CONFIG_OGXM_DEBUG

namespace ogxm_log {
    void init() __attribute__((weak));
}

#define OGXM_LOG(...)
#define OGXM_LOG_HEX(...)
#define OGXM_ASSERT(x)
#define OGXM_ASSERT_MSG(x, msg)
#define OGXM_TO_STRING(x)
    
#endif // CONFIG_OGXM_DEBUG

#endif // BOARD_API_LOG_H