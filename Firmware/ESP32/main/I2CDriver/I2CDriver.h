#ifndef _I2C_DRIVER_H_
#define _I2C_DRIVER_H_

#include <cstdint>
#include <cstring>
#include <atomic>
#include <driver/i2c.h>

#include "Reports.h"

//Will probably refactor this to be event driven
class I2CDriver 
{
public:
    I2CDriver() = default;
    ~I2CDriver() { i2c_driver_delete(I2C_NUM_0); }

    void run_task();

private:
    static constexpr bool MULTI_SLAVE = 
#if CONFIG_MULTI_SLAVE_MODE == 0
        false;
#else
        true;
#endif
    // std::array<ReportIn, CONFIG_BLUEPAD32_MAX_DEVICES> report_in_buffer_{};
    // std::array<std::atomic<bool>, CONFIG_BLUEPAD32_MAX_DEVICES> new_report_in_{false};
    // std::array<ReportOut, CONFIG_BLUEPAD32_MAX_DEVICES> report_out_buffer_{};

    void initialize_i2c();

    static inline esp_err_t i2c_write_blocking(uint8_t address, const uint8_t* buffer, size_t len) 
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, buffer, len, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(2));
        i2c_cmd_link_delete(cmd);
        return ret;
    }

    static inline esp_err_t i2c_read_blocking(uint8_t address, uint8_t* buffer, size_t len) 
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
        
        if (len > 1) 
        {
            i2c_master_read(cmd, buffer, len - 1, I2C_MASTER_ACK);
        }

        i2c_master_read_byte(cmd, buffer + len - 1, I2C_MASTER_NACK);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(2));
        i2c_cmd_link_delete(cmd);
        return ret;
    }
};

#endif // _I2C_DRIVER_H_