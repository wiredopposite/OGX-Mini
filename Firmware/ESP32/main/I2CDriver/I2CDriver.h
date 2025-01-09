#ifndef _I2C_DRIVER_H_
#define _I2C_DRIVER_H_

#include <cstdint>
#include <cstring>
#include <functional>
#include <driver/i2c.h>

#include "sdkconfig.h"
#include "RingBuffer.h"
#include "UserSettings/DeviceDriverTypes.h"

class I2CDriver 
{
public:
    static constexpr bool MULTI_SLAVE = 
#if CONFIG_MULTI_SLAVE_MODE == 0
        false;
#else
        true;
#endif

    enum class PacketID : uint8_t { UNKNOWN = 0, SET_PAD, GET_PAD, SET_DRIVER };
    enum class PacketResp : uint8_t { OK = 1, ERROR };

    #pragma pack(push, 1)
    struct PacketIn
    {
        uint8_t packet_len{sizeof(PacketIn)};
        PacketID packet_id{static_cast<uint8_t>(PacketID::SET_PAD)};
        uint8_t index{0};
        DeviceDriverType device_driver{DeviceDriverType::NONE};
        uint8_t dpad{0};
        uint16_t buttons{0};
        uint8_t trigger_l{0};
        uint8_t trigger_r{0};
        int16_t joystick_lx{0};
        int16_t joystick_ly{0};
        int16_t joystick_rx{0};
        int16_t joystick_ry{0};
        std::array<uint8_t, 15> reserved1{0};
    };
    static_assert(sizeof(PacketIn) == 32, "PacketIn is misaligned");

    struct PacketOut
    {
        uint8_t packet_len{0};
        PacketID packet_id{0};
        uint8_t index{0};
        uint8_t rumble_l{0};
        uint8_t rumble_r{0};
        std::array<uint8_t, 3> reserved{0};
    };
    static_assert(sizeof(PacketOut) == 8, "PacketOut is misaligned");
    #pragma pack(pop)

    I2CDriver() = default;
    ~I2CDriver();

    void initialize_i2c(i2c_port_t i2c_port, gpio_num_t sda, gpio_num_t scl, uint32_t clk_speed);

    //Does not return
    void run_tasks();

    void write_packet(uint8_t address, const PacketIn& data_in);
    void read_packet(uint8_t address, std::function<void(const PacketOut&)> callback);

private:
    using TaskQueue = RingBuffer<std::function<void()>, CONFIG_I2C_RING_BUFFER_SIZE>;
    
    TaskQueue task_queue_;
    i2c_port_t i2c_port_ = I2C_NUM_0;
    bool initialized_ = false;

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
}; // class I2CDriver

#endif // _I2C_DRIVER_H_