#ifndef _I2C_DRIVER_H_
#define _I2C_DRIVER_H_

#include <cstdint>
#include <cstring>
#include <functional>
#include <driver/i2c.h>

#include "sdkconfig.h"
#include "RingBuffer.h"

class I2CDriver 
{
public:
    static constexpr bool MULTI_SLAVE = 
#if CONFIG_MULTI_SLAVE_MODE == 0
        false;
#else
        true;
#endif

    enum class PacketID : uint8_t { UNKNOWN = 0, SET_PAD, GET_PAD };

    #pragma pack(push, 1)
    struct PacketIn
    {
        uint8_t packet_len;
        uint8_t packet_id;
        uint8_t index;
        uint8_t dpad;
        uint16_t buttons;
        uint8_t trigger_l;
        uint8_t trigger_r;
        int16_t joystick_lx;
        int16_t joystick_ly;
        int16_t joystick_rx;
        int16_t joystick_ry;

        PacketIn()
        {
            std::memset(this, 0, sizeof(PacketIn));
            packet_len = sizeof(PacketIn);
            packet_id = static_cast<uint8_t>(PacketID::SET_PAD);
        }
    };
    static_assert(sizeof(PacketIn) == 16, "PacketIn is misaligned");

    struct PacketOut
    {
        uint8_t packet_len;
        uint8_t packet_id;
        uint8_t index;
        uint8_t rumble_l;
        uint8_t rumble_r;
        uint8_t reserved[3];

        PacketOut()
        {
            std::memset(this, 0, sizeof(PacketOut));
            packet_len = sizeof(PacketOut);
            packet_id = static_cast<uint8_t>(PacketID::GET_PAD);
        }
    };
    static_assert(sizeof(PacketOut) == 8, "PacketOut is misaligned");
    #pragma pack(pop)

    I2CDriver() = default;
    ~I2CDriver();

    void initialize_i2c();

    //Does not return
    void run_tasks();

    inline void i2c_write_blocking_safe(uint8_t address, const PacketIn& packet_in) 
    {
        task_queue_.push([this, address, packet_in]() 
        {
            i2c_write_blocking(address, reinterpret_cast<const uint8_t*>(&packet_in), sizeof(PacketIn));
        });
    }

    inline void i2c_read_blocking_safe(uint8_t address, std::function<void(const PacketOut&)> callback) 
    {
        task_queue_.push([this, address, callback]() 
        {
            PacketOut packet_out;
            if (i2c_read_blocking(address, reinterpret_cast<uint8_t*>(&packet_out), sizeof(PacketOut)) == ESP_OK)
            {
                callback(packet_out);
            }
        });
    }

private:
    using TaskQueue = RingBuffer<std::function<void()>, 6>;
    TaskQueue task_queue_;

    inline esp_err_t i2c_write_blocking(uint8_t address, const uint8_t* buffer, size_t len) 
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

    inline esp_err_t i2c_read_blocking(uint8_t address, uint8_t* buffer, size_t len) 
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