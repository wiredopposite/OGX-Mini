#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "I2CDriver/I2CDriver.h"
#include "Bluepad32/Bluepad32.h"

I2CDriver::~I2CDriver()
{
    i2c_driver_delete(I2C_NUM_0);
}

void I2CDriver::initialize_i2c()
{
    i2c_config_t conf;
    std::memset(&conf, 0, sizeof(i2c_config_t));

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_21;
    conf.scl_io_num = GPIO_NUM_22;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 1000 * 1000;

    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

void I2CDriver::run_tasks()
{
    std::function<void()> task;

    while (true)
    {   
        while (task_queue_.pop(task))
        {
            task();
        }

        vTaskDelay(1);
    }
}