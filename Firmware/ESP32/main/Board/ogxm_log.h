#ifndef _OGXM_LOG_H_
#define _OGXM_LOG_H_

#include <esp_log.h>
#if ESP_LOG_LEVEL >= ESP_LOG_INFO

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdarg.h>

#include "uni.h"

namespace OGXM 
{
    static inline void log(const std::string& message)
    {
        logi(message.c_str());
    }

    static inline void log(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        uni_logv(fmt, args);
        va_end(args);
    }

    static inline void log_hex(const std::string& message, const uint8_t* data, const size_t len)
    {
        log(message);

        std::ostringstream hex_stream;
        hex_stream << std::hex << std::setfill('0');
        int char_num = 0;
        for (uint16_t i = 0; i < len; ++i)
        {
            hex_stream << std::setw(2) << static_cast<int>(data[i]) << " ";
            char_num++;
            if (char_num == 16)
            {
                hex_stream << "\n";
                char_num = 0;
            }
        }
        if (char_num != 0)
        {
            hex_stream << "\n";
        }

        log(hex_stream.str());
    }

} // namespace OGXM

#define OGXM_LOG OGXM::log
#define OGXM_LOG_HEX OGXM::log_hex

#else // ESP_LOG_LEVEL >= ESP_LOG_INFO

#define OGXM_LOG(...)
#define OGXM_LOG_HEX(...)

#endif // ESP_LOG_LEVEL >= ESP_LOG_INFO

#endif // _OGXM_LOG_H_