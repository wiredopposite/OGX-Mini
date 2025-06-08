#include <pico/mutex.h>
#include <pico/sync.h>
#include <string.h>
#include "log/log.h"
#include "settings/nvs.h"

#ifndef NVS_SECTOR_COUNT
#define NVS_SECTOR_COUNT        4
#endif

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES   (2*1024*1024)
#endif

#define NVS_START_OFFSET        ((uint32_t)(PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE * NVS_SECTOR_COUNT)))
#define NVS_ENTRIES_MAX_INT     ((uint32_t)(NVS_SECTOR_COUNT * (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)))

typedef struct __attribute__((packed, aligned(4))) {
    char    key[NVS_KEY_LEN_MAX];
    uint8_t value[NVS_VALUE_LEN_MAX];
} nvs_entry_t;
_Static_assert(sizeof(nvs_entry_t) == FLASH_PAGE_SIZE, "NVS entry size mismatch");

static const char INVALID_KEY[] = "INVALID";

static mutex_t nvs_mutex;
static uint32_t ints = 0;
static uint8_t sector_buffer[FLASH_SECTOR_SIZE] __attribute__((aligned(4))) = {0};
static volatile bool nvs_inited = false;

static void __not_in_flash_func(nvs_lock)(void) {
    mutex_enter_blocking(&nvs_mutex);
    ogxm_logd("NVS locked\n");
    // ints = save_and_disable_interrupts();
}

static void __not_in_flash_func(nvs_unlock)(void) {
    // restore_interrupts(ints);
    mutex_exit(&nvs_mutex);
    ogxm_logd("NVS unlocked\n");
}

static const nvs_entry_t* __not_in_flash_func(nvs_get_entry)(uint32_t index) {
    return (const nvs_entry_t*)(XIP_BASE + NVS_START_OFFSET + (index * sizeof(nvs_entry_t)));
}

static bool __not_in_flash_func(nvs_args_valid)(const char* key, const void* value, size_t len) {
    if ((key == NULL) || (value == NULL) || (len > NVS_VALUE_LEN_MAX) || (len == 0)) {
        return false;
    }
    if (strnlen(key, NVS_KEY_LEN_MAX) >= NVS_KEY_LEN_MAX) {
        return false;
    }
    if (strncmp(key, INVALID_KEY, sizeof(INVALID_KEY)) == 0) {
        return false;
    }
    return true;
}

static bool __not_in_flash_func(nvs_entry_valid)(const nvs_entry_t* entry) {
    return (strncmp(entry->key, INVALID_KEY, sizeof(INVALID_KEY)) != 0);
}

static void __not_in_flash_func(nvs_erase_all_internal)(void) {
    for (uint32_t i = 0; i < NVS_SECTOR_COUNT; i++) {
        flash_range_erase(NVS_START_OFFSET + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
    }
    nvs_entry_t entry = {0};
    strncpy(entry.key, INVALID_KEY, sizeof(INVALID_KEY));
    for (uint32_t i = 0; i < NVS_ENTRIES_MAX_INT; i++) {
        flash_range_program(NVS_START_OFFSET + (i * FLASH_PAGE_SIZE), (uint8_t*)&entry, FLASH_PAGE_SIZE);
    }
}

/* Public */

void __not_in_flash_func(nvs_init)(void) {
    mutex_init(&nvs_mutex);
    nvs_lock();
    /* First entry should be marked invalid */
    if (nvs_entry_valid(nvs_get_entry(0))) {
        nvs_erase_all_internal();
    }
    nvs_inited = true;
    nvs_unlock();
}

void __not_in_flash_func(nvs_erase_all)(void) {
    if (!nvs_inited) {
        return;
    }
    nvs_lock();
    nvs_erase_all_internal();
    nvs_unlock();
}

bool __not_in_flash_func(nvs_read)(const char* key, void* value, size_t len) {
    if (!nvs_inited || !nvs_args_valid(key, value, len)) {
        return false;
    }
    nvs_lock();
    for (uint32_t i = 1; i < NVS_ENTRIES_MAX_INT; i++) {
        const nvs_entry_t* entry = nvs_get_entry(i);
        if (strncmp(entry->key, key, NVS_KEY_LEN_MAX) == 0) {
            memcpy(value, entry->value, MIN(len, NVS_VALUE_LEN_MAX));
            nvs_unlock();
            return true;
        }
    }
    nvs_unlock();
    return false;
}

bool __not_in_flash_func(nvs_write)(const char* key, const void* value, size_t len) {
    if (!nvs_inited || !nvs_args_valid(key, value, len)) {
        return false;
    }
    nvs_lock();
    for (uint32_t i = 1; i < NVS_ENTRIES_MAX_INT; i++) {
        const nvs_entry_t* entry = nvs_get_entry(i);
        /* If we found an invalid entry, that's the end of the written entries */
        if (!nvs_entry_valid(entry) || (strncmp(entry->key, key, NVS_KEY_LEN_MAX) == 0)) {
            uint32_t entry_offset = i * FLASH_PAGE_SIZE; /* From NVS start */
            uint32_t sector_offset = 
                ((NVS_START_OFFSET + entry_offset) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;

            memcpy(sector_buffer, (const uint8_t*)(XIP_BASE + sector_offset), FLASH_SECTOR_SIZE);
            flash_range_erase(sector_offset, FLASH_SECTOR_SIZE);

            nvs_entry_t* entry_buf = (nvs_entry_t*)(sector_buffer + entry_offset);
            strncpy(entry_buf->key, key, NVS_KEY_LEN_MAX);
            entry_buf->key[strnlen(key, NVS_KEY_LEN_MAX)] = '\0';
            memset(entry_buf->value, 0, NVS_VALUE_LEN_MAX);
            memcpy(entry_buf->value, value, len);

            for (uint32_t j = 0; j < FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE; j++) {
                flash_range_program(sector_offset + (j * FLASH_PAGE_SIZE), 
                                    sector_buffer + (j * FLASH_PAGE_SIZE), 
                                    FLASH_PAGE_SIZE);
            }
            nvs_unlock();
            return true;
        }
    }
    nvs_unlock();
    return false;
}