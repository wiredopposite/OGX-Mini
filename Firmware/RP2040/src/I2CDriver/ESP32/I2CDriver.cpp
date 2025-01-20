#include <array>
#include <cstring>
#include <pico/i2c_slave.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "board_config.h"
#include "Board/board_api.h"
#include "I2CDriver/ESP32/I2CDriver.h"
#include "UserSettings/UserSettings.h"
#include "TaskQueue/TaskQueue.h"
#include "Board/ogxm_log.h"

namespace I2CDriver {

enum class PacketID : uint8_t { UNKNOWN = 0, SET_PAD, GET_PAD, SET_DRIVER };

#pragma pack(push, 1)
struct PacketIn
{
    uint8_t packet_len{sizeof(PacketIn)};
    PacketID packet_id{PacketID::SET_PAD};
    uint8_t index{0};
    DeviceDriverType device_type{DeviceDriverType::NONE};
    Gamepad::PadIn pad_in{Gamepad::PadIn()};
    std::array<uint8_t, 5> reserved{0};
};
static_assert(sizeof(PacketIn) == 32, "i2c_driver_esp::PacketIn size mismatch");

struct PacketOut
{
    uint8_t packet_len{sizeof(PacketOut)};
    PacketID packet_id{PacketID::GET_PAD};
    uint8_t index{0};
    Gamepad::PadOut pad_out{Gamepad::PadOut()};
    std::array<uint8_t, 3> reserved{0};
};
static_assert(sizeof(PacketOut) == 8, "i2c_driver_esp::PacketOut size mismatch");
#pragma pack(pop)

static constexpr size_t MAX_BUFFER_SIZE = std::max(sizeof(PacketOut), sizeof(PacketIn));
static constexpr uint8_t I2C_ADDR = 0x01;

Gamepad* gamepads_[MAX_GAMEPADS];

static inline void slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    static size_t count = 0;
    static PacketIn packet_in;
    static PacketOut packet_out;
    static DeviceDriverType current_device_type = UserSettings::get_instance().get_current_driver();

    switch (event) 
    {
        case I2C_SLAVE_RECEIVE:
            if (count < sizeof(PacketIn))
            {
                reinterpret_cast<uint8_t*>(&packet_in)[count] = i2c_read_byte_raw(i2c);
                ++count;
            }
            break;

        case I2C_SLAVE_FINISH:
            if (count == 0)
            {
                break;
            }
            switch (packet_in.packet_id)
            {
                case PacketID::SET_PAD:
                    if (packet_in.index < MAX_GAMEPADS)
                    {
                        gamepads_[packet_in.index]->set_pad_in(packet_in.pad_in);
                    }
                    break;

                case PacketID::SET_DRIVER:
                    if (packet_in.device_type != DeviceDriverType::NONE &&
                        packet_in.device_type != current_device_type)
                    {
                        OGXM_LOG("I2C: Driver change detected.\n");

                        //Any writes to flash should be done on Core0
                        TaskQueue::Core0::queue_delayed_task(TaskQueue::Core0::get_new_task_id(), 1000, false, 
                        [new_device_type = packet_in.device_type]
                        { 
                            UserSettings::get_instance().store_driver_type(new_device_type);
                        });
                    }
                    break;
                    
                default:
                    break;
            }
            count = 0;
            break;

        case I2C_SLAVE_REQUEST:
            if (packet_in.index < MAX_GAMEPADS)
            {
                packet_out.index = packet_in.index;
                packet_out.pad_out = gamepads_[packet_in.index]->get_pad_out();
            }
            // switch (packet_in.packet_id)
            // {
            //     case PacketID::SET_PAD:
                    
                    i2c_write_raw_blocking(i2c, reinterpret_cast<const uint8_t*>(&packet_out), packet_out.packet_len);
            //         break;

            //     default:
                    // i2c_write_raw_blocking(i2c, reinterpret_cast<const uint8_t*>(&packet_out), 1);
            //         break;
            // }
            
            // packet_in = PacketIn();
            break;

        default:
            break;
    }
}

void initialize(Gamepad (&gamepads)[MAX_GAMEPADS])
{
    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads_[i] = &gamepads[i];
    }

    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_slave_init(I2C_PORT, I2C_ADDR, &slave_handler);

    OGXM_LOG("I2C Driver initialized\n");
}

} // namespace i2c_driver_esp32