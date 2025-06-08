#include "board_config.h"
#if OGXM_LOG_ENABLED

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pico/stdio_uart.h>
#include <pico/mutex.h>
#include "log/log.h"

static mutex_t log_mutex;
static bool multi_core = false;

void ogxm_log_init(bool multicore) {
    multi_core = multicore;
    if (multi_core) {
        mutex_init(&log_mutex);
    }
    stdio_uart_init();
}

void ogxm_log_level(uint8_t level, const char* fmt, ...) {
    if (level > OGXM_LOG_LEVEL) {
        return;
    }
    if (multi_core) {
        mutex_enter_blocking(&log_mutex);
    }

    va_list args;
    va_start(args, fmt);
    
    switch (level) {
        case 0: // Error
            fprintf(stderr, "OGXM ERROR: ");
            vfprintf(stderr, fmt, args);
            break;
        case 1: // Info
            printf("OGXM INFO: ");
            vprintf(fmt, args);
            break;
        case 2: // Debug
            printf("OGXM DEBUG: ");
            vprintf(fmt, args);
            break;
        case 3: // Verbose
            printf("OGXM VERBOSE: ");
            vprintf(fmt, args);
            break;
    }
    va_end(args);
    if (multi_core) {
        mutex_exit(&log_mutex);
    }
}

void ogxm_log_hex_level(uint8_t level, const void* data, uint16_t len, const char* fmt, ...) {
    if (level > OGXM_LOG_LEVEL) {
        return;
    }
    if (multi_core) {
        mutex_enter_blocking(&log_mutex);
    }

    va_list args;
    va_start(args, fmt);
    
    switch (level) {
        case 0: // Error
            fprintf(stderr, "OGXM ERROR: ");
            vfprintf(stderr, fmt, args);
            break;
        case 1: // Info
            printf("OGXM INFO: ");
            vprintf(fmt, args);
            break;
        case 2: // Debug
            printf("OGXM DEBUG: ");
            vprintf(fmt, args);
            break;
        case 3: // Verbose
            printf("OGXM VERBOSE: ");
            vprintf(fmt, args);
            break;
    }
    va_end(args);
    printf("\n");

    for (uint16_t i = 0; i < len; i++) {
        if (i % 8 == 0 && i != 0) {
            printf("\n");
        }
        printf(" %02X", ((uint8_t*)data)[i]);
    }
    printf("\n");

    if (multi_core) {
        mutex_exit(&log_mutex);
    }
}

#endif // OGXM_LOG_ENABLED