#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_err.h>
#include "log/log.h"
#include "settings/nvs.h"

static const char NVS_NAMESPACE[] = "user_data";
static SemaphoreHandle_t nvs_mutex;
static volatile bool nvs_inited = false;

void nvs_init(void) {
    nvs_mutex = xSemaphoreCreateMutex();
    xSemaphoreTake(nvs_mutex, portMAX_DELAY);

    if (nvs_flash_init() != ESP_OK) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    nvs_inited = true;
    xSemaphoreGive(nvs_mutex);
}

void nvs_erase(void) {
    if (!nvs_inited) {
        return;
    }
    nvs_handle_t handle;
    xSemaphoreTake(nvs_mutex, portMAX_DELAY);

    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        xSemaphoreGive(nvs_mutex);
        return;
    }
    nvs_erase_all(handle);
    nvs_close(handle);
    xSemaphoreGive(nvs_mutex);
}

bool nvs_read(const char* key, void* value, size_t len) {
    if (!nvs_inited) {
        return false;
    }
    esp_err_t err;
    nvs_handle_t handle;

    xSemaphoreTake(nvs_mutex, portMAX_DELAY);

    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)) != ESP_OK) {
        xSemaphoreGive(nvs_mutex);
        return (err == ESP_OK);
    }

    err = nvs_get_blob(handle, key, value, &len);
    nvs_close(handle);

    xSemaphoreGive(nvs_mutex);
    return (err == ESP_OK);
}

bool nvs_write(const char* key, const void* value, size_t len) {
    if (!nvs_inited) {
        return false;
    }
    esp_err_t err;
    nvs_handle_t handle;

    xSemaphoreTake(nvs_mutex, portMAX_DELAY);

    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)) != ESP_OK) {
        xSemaphoreGive(nvs_mutex);
        return (err == ESP_OK);
    }
    if ((err = nvs_set_blob(handle, key, value, len)) != ESP_OK) {
        nvs_close(handle);
        xSemaphoreGive(nvs_mutex);
        return (err == ESP_OK);
    } 
    err = nvs_commit(handle);
    nvs_close(handle);

    xSemaphoreGive(nvs_mutex);
    return (err == ESP_OK);
}