// #include <cstring>
// #include <atomic>
// #include <array>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <freertos/timers.h>
// #include <driver/i2c.h>
// #include <driver/gpio.h>
// #include <esp_log.h>

// #include "sdkconfig.h"
// #include "Board/board_api.h"
// #include "I2CDriver/I2CDriver.h"
// #include "Bluepad32/Bluepad32.h"

// namespace i2c_driver {

// static constexpr bool MULTI_SLAVE = 
// #if CONFIG_MULTI_SLAVE_MODE == 0
//     false;
// #else
//     true;
// #endif

// std::atomic<bool> poll_rumble_{false};

// void i2c_initialize()
// {
//     i2c_config_t conf;
//     std::memset(&conf, 0, sizeof(i2c_config_t));
//     conf.mode = I2C_MODE_MASTER;
//     conf.sda_io_num = GPIO_NUM_21;
//     conf.scl_io_num = GPIO_NUM_22;
//     conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
//     conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
//     conf.master.clk_speed = 400 * 1000;

//     i2c_param_config(I2C_NUM_0, &conf);
//     i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

//     ESP_LOGD("I2C", "I2C initialized at pins {SDA: %d, SCL: %d}", conf.sda_io_num, conf.scl_io_num);
// }

// static inline esp_err_t i2c_write_blocking(uint8_t slave_address, uint8_t* data, size_t data_len) 
// {
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (slave_address << 1) | I2C_MASTER_WRITE, true);
//     i2c_master_write(cmd, data, data_len, true);
//     i2c_master_stop(cmd);

//     esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(2));
//     i2c_cmd_link_delete(cmd);
//     return ret;
// }

// static inline esp_err_t i2c_read_blocking(uint8_t slave_address, uint8_t* data, size_t data_len) 
// {
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (slave_address << 1) | I2C_MASTER_READ, true);
    
//     if (data_len > 1) 
//     {
//         i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
//     }

//     i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK);
//     i2c_master_stop(cmd);
    
//     esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(2));
//     i2c_cmd_link_delete(cmd);
//     return ret;
// }

// void run_loop(void* queue)
// {
//     ReportIn report_in = ReportIn();
//     ReportOut report_out = ReportOut();

//     i2c_initialize();

//     ESP_LOGD("I2C Stack", "Remaining stack: %d", uxTaskGetStackHighWaterMark(NULL));

//     while (true)
//     {
//         for (uint8_t i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
//         {
//             vTaskDelay(1);

//             if (!bluepad32::connected(i))
//             {
//                 continue;
//             }

//             report_in = bluepad32::get_report_in(i);
//             if (i2c_write_blocking(MULTI_SLAVE ? (report_in.index + 1) : 0x01, reinterpret_cast<uint8_t*>(&report_in), sizeof(ReportIn)) != ESP_OK)
//             {
//                 ESP_LOGD("I2C", "Failed to write report_in");
//                 continue;
//             }

//             vTaskDelay(1);

//             if (i2c_read_blocking(MULTI_SLAVE ? (report_in.index + 1) : 0x01, reinterpret_cast<uint8_t*>(&report_out), sizeof(ReportOut)) != ESP_OK)
//             {
//                 ESP_LOGD("I2C", "Failed to read report_out");
//                 continue;
//             }

//             bluepad32::set_report_out(report_out);
//         }
//     }
// }

// // void run_loop()
// // {
// //     ESP_LOGD("Stack", "Remaining stack: %d", uxTaskGetStackHighWaterMark(NULL));

// //     ReportIn report_in = ReportIn();

// //     ESP_LOGD("I2C", "ReportIn 1 created: size: %d", sizeof(report_in));
// //     ESP_LOGD("Stack", "Remaining stack: %d", uxTaskGetStackHighWaterMark(NULL));

// //     ReportIn different_rep = ReportIn();

// //     ESP_LOGD("I2C", "ReportIn 2 created: size: %d", sizeof(different_rep));  
// //     ESP_LOGD("Stack", "Remaining stack: %d", uxTaskGetStackHighWaterMark(NULL));

// //     ReportOut report_out = ReportOut();

// //     ESP_LOGD("I2C", "ReportOut created: size: %d", sizeof(report_out));  
// //     ESP_LOGD("Stack", "Remaining stack: %d", uxTaskGetStackHighWaterMark(NULL));

// //     i2c_initialize();
// //     ESP_LOGD("Stack", "Remaining stack: %d", uxTaskGetStackHighWaterMark(NULL));

// //     while (true)
// //     {
// //         i2c_write_blocking(MULTI_SLAVE ? (report_in.index + 1) : 0x01, reinterpret_cast<uint8_t*>(&report_in), sizeof(ReportIn));
// //         vTaskDelay(1);
// //     }
// // }

// } // namespace I2CDriver

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
            // if (bluepad32::get_report_in(i, report_in))
            // {
            report_in = bluepad32::get_report_in(i);
            if (i2c_write_blocking(MULTI_SLAVE ? (i + 1) : 0x01, reinterpret_cast<const uint8_t*>(&report_in), sizeof(ReportIn)) != ESP_OK)
            {
                continue;
            }
            // }

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