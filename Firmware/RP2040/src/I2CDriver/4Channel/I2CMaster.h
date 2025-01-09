#ifndef I2C_MASTER_4CH_H
#define I2C_MASTER_4CH_H

#include <cstdint>
#include <atomic>
#include <array>
#include <hardware/i2c.h>

#include "board_config.h"
#include "Gamepad.h"
#include "I2CDriver/4Channel/I2CDriver.h"

class I2CMaster : public I2CDriver
{
public:
    ~I2CMaster() = default;
    void initialize(uint8_t address) override;
    void process(Gamepad (&gamepads)[MAX_GAMEPADS]) override;
    void notify_tuh(bool mounted, HostDriverType host_type) override;
    void notify_xbox360w(bool connected, uint8_t idx) override;

private:
    struct Slave
    {
        uint8_t address{0xFF};
        Status status{Status::NC};
        std::atomic<bool> enabled{false};
    };

    static constexpr size_t NUM_SLAVES = MAX_GAMEPADS - 1;
    static_assert(NUM_SLAVES > 0, "I2CMaster::NUM_SLAVES must be greater than 0 to use I2C");

    std::atomic<bool> i2c_enabled_{false};
    std::array<Slave, NUM_SLAVES> slaves_; 

    void notify_disable(uint8_t address);

    static bool slave_detected(uint8_t address);
    
    static inline bool read_blocking(uint8_t address, void* buffer, size_t len)
    {
        return (i2c_read_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(buffer), len, false) == static_cast<int>(len));
    }
    static inline bool write_blocking(uint8_t address, void* buffer, size_t len)
    {
        return (i2c_write_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(buffer), len, false) == static_cast<int>(len));
    }
};

#endif // I2C_MASTER_4CH_H