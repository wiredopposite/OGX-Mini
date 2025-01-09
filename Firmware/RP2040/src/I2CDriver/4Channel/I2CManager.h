#ifndef I2C_4CH_MANAGER_H
#define I2C_4CH_MANAGER_H

#include <cstdint>
#include <memory>
#include <atomic>   
#include <hardware/gpio.h>

#include "board_config.h"
#include "Gamepad.h"
#include "I2CDriver/4Channel/I2CMaster.h"
#include "I2CDriver/4Channel/I2CSlave.h"
#include "I2CDriver/4Channel/I2CDriver.h"

class I2CManager
{
public:
    I2CManager(const I2CManager&) = delete;
    I2CManager& operator=(const I2CManager&) = delete;

    static I2CManager& get_instance()
    {
        static I2CManager instance;
        return instance;
    }

    bool initialize_driver()
    {
        uint8_t i2c_address = get_i2c_address();
        if (i2c_address == 0xFF)
        {
            return false;
        }
        if (i2c_address < 1)
        {
            is_master_.store(true);
            driver_ = std::make_unique<I2CMaster>();
        }
        else
        {
            driver_ = std::make_unique<I2CSlave>();
        }
        driver_->initialize(i2c_address);
        return true;
    }

    I2CDriver* get_driver() 
    { 
        return driver_.get(); 
    }

    bool is_master()
    {
        return is_master_.load();
    }

private:
    I2CManager() {};
    ~I2CManager() {};

    std::unique_ptr<I2CDriver> driver_{nullptr};
    std::atomic<bool> is_master_{false};

    uint8_t get_i2c_address()
    {
        gpio_init(SLAVE_ADDR_PIN_1);
        gpio_init(SLAVE_ADDR_PIN_2);
        gpio_pull_up(SLAVE_ADDR_PIN_1);
        gpio_pull_up(SLAVE_ADDR_PIN_2);

        if (gpio_get(SLAVE_ADDR_PIN_1) == 1 && gpio_get(SLAVE_ADDR_PIN_2) == 1)
        {
            return 0x00;
        }
        else if (gpio_get(SLAVE_ADDR_PIN_1) == 1 && gpio_get(SLAVE_ADDR_PIN_2) == 0)
        {
            return 0x01;
        }
        else if (gpio_get(SLAVE_ADDR_PIN_1) == 0 && gpio_get(SLAVE_ADDR_PIN_2) == 1)
        {
            return 0x02;
        }
        else if (gpio_get(SLAVE_ADDR_PIN_1) == 0 && gpio_get(SLAVE_ADDR_PIN_2) == 0)
        {
            return 0x03;
        }

        return 0xFF;
    }
};

#endif // I2C_4CH_MANAGER_H