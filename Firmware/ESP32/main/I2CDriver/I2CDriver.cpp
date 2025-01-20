#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "I2CDriver/I2CDriver.h"

I2CDriver::~I2CDriver()
{
    i2c_driver_delete(i2c_port_);
}

void I2CDriver::initialize_i2c(i2c_port_t i2c_port, gpio_num_t sda, gpio_num_t scl, uint32_t clk_speed)
{
    if (initialized_)
    {
        return;
    }

    i2c_port_ = i2c_port;

    i2c_config_t conf;
    std::memset(&conf, 0, sizeof(i2c_config_t));

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda;
    conf.scl_io_num = scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = clk_speed;

    i2c_param_config(i2c_port_, &conf);
    i2c_driver_install(i2c_port_, conf.mode, 0, 0, 0);

    initialized_ = true;
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

void I2CDriver::write_packet(uint8_t address, const PacketIn& data_in) 
{
    task_queue_.push([this, address, data_in]() 
    {
        i2c_write_blocking(address, reinterpret_cast<const uint8_t*>(&data_in), sizeof(PacketIn));
    });
}

void I2CDriver::read_packet(uint8_t address, std::function<void(const PacketOut&)> callback) 
{
    task_queue_.push([this, address, callback]() 
    {
        PacketOut data_out;
        if (i2c_read_blocking(address, reinterpret_cast<uint8_t*>(&data_out), sizeof(PacketOut)) == ESP_OK)
        {
            callback(data_out);
        }
    });
}