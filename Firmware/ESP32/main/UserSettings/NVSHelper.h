#ifndef _NVS_HELPER_H_
#define _NVS_HELPER_H_

#include <cstdint>
#include <array>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_err.h>

class NVSHelper
{
public:
    static NVSHelper& get_instance()
    {
        static NVSHelper instance;
        return instance;
    }

    esp_err_t write(const std::string& key, const void* value, size_t len)
    {
        esp_err_t err;
        nvs_handle_t handle;

        xSemaphoreTake(nvs_mutex_, portMAX_DELAY);

        if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)) != ESP_OK) 
        {
            xSemaphoreGive(nvs_mutex_);
            return err;
        }

        if ((err = nvs_set_blob(handle, key.c_str(), value, len)) != ESP_OK) 
        {
            nvs_close(handle);
            xSemaphoreGive(nvs_mutex_);
            return err;
        } 

        err = nvs_commit(handle);
        nvs_close(handle);

        xSemaphoreGive(nvs_mutex_);
        return err;
    }

    esp_err_t read(const std::string& key, void* value, size_t len)
    {
        esp_err_t err;
        nvs_handle_t handle;

        xSemaphoreTake(nvs_mutex_, portMAX_DELAY);

        if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)) != ESP_OK) 
        {
            xSemaphoreGive(nvs_mutex_);
            return err;
        }

        err = nvs_get_blob(handle, key.c_str(), value, &len);
        nvs_close(handle);

        xSemaphoreGive(nvs_mutex_);
        return err;
    }

    esp_err_t erase_all()
    {
        esp_err_t err;
        nvs_handle_t handle;

        xSemaphoreTake(nvs_mutex_, portMAX_DELAY);

        if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)) != ESP_OK) 
        {
            xSemaphoreGive(nvs_mutex_);
            return err;
        }

        err = nvs_erase_all(handle);
        nvs_close(handle);

        xSemaphoreGive(nvs_mutex_);
        return err;
    }

private:
    NVSHelper()
    {
        nvs_mutex_ = xSemaphoreCreateMutex();
        xSemaphoreTake(nvs_mutex_, portMAX_DELAY);

        if (nvs_flash_init() != ESP_OK) 
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        }

        xSemaphoreGive(nvs_mutex_);
    }
    ~NVSHelper() = default;
    NVSHelper(const NVSHelper&) = delete;
    NVSHelper& operator=(const NVSHelper&) = delete; 

    SemaphoreHandle_t nvs_mutex_;

    static constexpr char NVS_NAMESPACE[] = "user_data";
    
}; // class NVSHelper

#endif // _NVS_HELPER_H_