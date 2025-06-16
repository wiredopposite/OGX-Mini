#include "log/log.h"
#if CONFIG_LOG_ENABLED

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t log_mutex;
static bool multithread = false;

void ogxm_log_init(bool multithreaded) {
    multithread = multithreaded;
    if (multithread) {
        log_mutex = xSemaphoreCreateMutex();
    }
}

void ogxm_log_level(uint8_t level, const char* fmt, ...) {
    if (level > CONFIG_LOG_LEVEL) {
        return;
    }
    if (multithread) {
        xSemaphoreTake(log_mutex, portMAX_DELAY);
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
    if (multithread) {
        xSemaphoreGive(log_mutex);
    }
}

void ogxm_log_hex_level(uint8_t level, const void* data, uint16_t len, const char* fmt, ...) {
    if (level > CONFIG_LOG_LEVEL) {
        return;
    }
    if (multithread) {
        xSemaphoreTake(log_mutex, portMAX_DELAY);
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

    if (multithread) {
        xSemaphoreGive(log_mutex);
    }
}

#endif // CONFIG_LOG_ENABLED