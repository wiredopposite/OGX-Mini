#include <array>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "I2CDriver/I2CDriver.h"
#include "Bluepad32/Bluepad32.h"

void I2CDriver::initialize_i2c()
{
    i2c_config_t conf;
    std::memset(&conf, 0, sizeof(i2c_config_t));

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_21;
    conf.scl_io_num = GPIO_NUM_22;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400 * 1000;

    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

void I2CDriver::run_task()
{
    ReportIn report_in;
    ReportOut report_out;

    initialize_i2c();

    while (true)
    {
        for (uint8_t i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
        {
            if (!bluepad32::connected(i))
            {
                vTaskDelay(1);
                continue;
            }

            report_in = bluepad32::get_report_in(i);
            if (i2c_write_blocking(MULTI_SLAVE ? (i + 1) : 0x01, reinterpret_cast<const uint8_t*>(&report_in), sizeof(ReportIn)) != ESP_OK)
            {
                continue;
            }

            vTaskDelay(1);

            if (i2c_read_blocking(MULTI_SLAVE ? (i + 1) : 0x01, reinterpret_cast<uint8_t*>(&report_out), sizeof(ReportOut)) != ESP_OK)
            {
                continue;
            }

            bluepad32::set_report_out(report_out);
        }

        vTaskDelay(1);
    }
}