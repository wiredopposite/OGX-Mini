#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <hardware/flash.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NVS_KEY_LEN_MAX     ((size_t)16)
#define NVS_VALUE_LEN_MAX   ((size_t)FLASH_PAGE_SIZE - NVS_KEY_LEN_MAX)

void nvs_init(void);
bool nvs_read(const char* key, void* value, size_t len);
bool nvs_write(const char* key, const void* value, size_t len);
void nvs_erase_all(void);

#ifdef __cplusplus
}
#endif