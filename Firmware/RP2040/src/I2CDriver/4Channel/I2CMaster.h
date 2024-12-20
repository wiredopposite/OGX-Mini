#ifndef I2C_MASTER_4CH_H
#define I2C_MASTER_4CH_H

#include <cstdint>
#include <atomic>
#include <array>

#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "board_config.h"
#include "Gamepad.h"
#include "I2CDriver/4Channel/I2CDriver.h"

class I2CMaster : public I2CDriver
{
public:
    ~I2CMaster() override;
    void initialize(uint8_t address) override;
    void process(Gamepad (&gamepads)[MAX_GAMEPADS]) override;
    void notify_tuh_mounted(HostDriver::Type host_type) override;
    void notify_tuh_unmounted(HostDriver::Type host_type) override;
    void notify_xbox360w_connected(uint8_t idx) override;
    void notify_xbox360w_disconnected(uint8_t idx) override;

private:
    struct Slave
    {
        uint8_t address{0xFF};
        SlaveStatus status{SlaveStatus::NC};
        std::atomic<bool> enabled{false};
    };

    static constexpr size_t NUM_SLAVES = MAX_GAMEPADS - 1;
    static_assert(NUM_SLAVES > 0, "I2CMaster::NUM_SLAVES must be greater than 0 to use I2C");

    uint32_t tid_update_slave_;
    bool update_slave_status_{false};

    std::atomic<bool> i2c_enabled_{false};
    // std::atomic<bool> notify_deinit_{false};
    std::array<Slave, NUM_SLAVES> slaves_;  

    static bool slave_detected(uint8_t address);

    void check_slave_status(Slave& slave);
    bool notify_disable(Slave& slave);
    SlaveStatus notify_enable(uint8_t address);
    // bool send_packet_status(uint8_t address, PacketID packet_id);
    SlaveStatus get_slave_status(uint8_t address);

    bool send_packet_in(Slave& slave, Gamepad& gamepad);
    bool get_packet_out(Slave& slave, Gamepad& gamepad);
};

#endif // I2C_MASTER_4CH_H