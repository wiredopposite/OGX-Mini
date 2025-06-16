#pragma once

#include "board_config.h"
#if OGXM_LOG_ENABLED

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ogxm_log_init(bool multithread);
void ogxm_log_level(uint8_t level, const char* fmt, ...);
void ogxm_log_hex_level(uint8_t level, const void* data, uint16_t len, const char* fmt, ...);

#define ogxm_loge(fmt, ...) ogxm_log_level(0, fmt, ##__VA_ARGS__)
#define ogxm_logi(fmt, ...) ogxm_log_level(1, fmt, ##__VA_ARGS__)
#define ogxm_logd(fmt, ...) ogxm_log_level(2, fmt, ##__VA_ARGS__)
#define ogxm_logv(fmt, ...) ogxm_log_level(3, fmt, ##__VA_ARGS__)
#define ogxm_loge_hex(data, len, fmt, ...) ogxm_log_hex_level(0, data, len, fmt, ##__VA_ARGS__)
#define ogxm_logi_hex(data, len, fmt, ...) ogxm_log_hex_level(1, data, len, fmt, ##__VA_ARGS__)
#define ogxm_logd_hex(data, len, fmt, ...) ogxm_log_hex_level(2, data, len, fmt, ##__VA_ARGS__)
#define ogxm_logv_hex(data, len, fmt, ...) ogxm_log_hex_level(3, data, len, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#else 

#define ogxm_log_init(...)      ((void)0)
#define ogxm_loge(fmt, ...)     ((void)0)
#define ogxm_logi(fmt, ...)     ((void)0)
#define ogxm_logd(fmt, ...)     ((void)0)
#define ogxm_logv(fmt, ...)     ((void)0)
#define ogxm_loge_hex(fmt, ...) ((void)0)
#define ogxm_logi_hex(fmt, ...) ((void)0)
#define ogxm_logd_hex(fmt, ...) ((void)0)
#define ogxm_logv_hex(fmt, ...) ((void)0)

#endif // OGXM_LOG_ENABLED