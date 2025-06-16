#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void nvs_init(void);
bool nvs_read(const char* key, void* value, size_t len);
bool nvs_write(const char* key, const void* value, size_t len);
void nvs_erase(void);

#ifdef __cplusplus
}
#endif