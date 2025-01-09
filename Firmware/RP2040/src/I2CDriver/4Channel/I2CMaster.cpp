#include <cstring>
#include <hardware/gpio.h>

#include "TaskQueue/TaskQueue.h"
#include "I2CDriver/4Channel/I2CMaster.h"

void I2CMaster::initialize(uint8_t address) 
{
    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    for (uint8_t i = 0; i < NUM_SLAVES; ++i)
    {
        slaves_[i].address = address + i + 1;
    }
}

void I2CMaster::process(Gamepad (&gamepads)[MAX_GAMEPADS])
{
    for (uint8_t i = 0; i < NUM_SLAVES; ++i)
    {
        Slave& slave = slaves_[i];

        if (!slave.enabled.load() || !slave_detected(slave.address))
        {
            continue;
        }

        PacketCMD packet_cmd;
        packet_cmd.packet_id = PacketID::COMMAND;
        packet_cmd.command = Command::STATUS;

        if (!write_blocking(slave.address, &packet_cmd, sizeof(PacketCMD)) ||
            !read_blocking(slave.address, &packet_cmd, sizeof(PacketCMD)))
        {
            continue;
        }

        if (packet_cmd.status == Status::READY)
        {
            Gamepad& gamepad = gamepads[i + 1];
            PacketIn packet_in;
            packet_in.pad_in = gamepad.get_pad_in();
            packet_in.chatpad_in = gamepad.get_chatpad_in();

            if (write_blocking(slave.address, &packet_in, sizeof(PacketIn)))
            {
                PacketOut packet_out;
                if (read_blocking(slave.address, &packet_out, sizeof(PacketOut)))
                {
                    gamepad.set_pad_out(packet_out.pad_out);
                }
            }
        }

        sleep_us(100);
    }
}

void I2CMaster::notify_tuh(bool mounted, HostDriverType host_type)
{
    if (host_type != HostDriverType::XBOX360W)
    {
        return;
    }

    i2c_enabled_.store(mounted);

    if (!mounted)
    {
        //Called from core1 so queue on core0
        TaskQueue::Core0::queue_task(
            [this]()
            {
                for (auto& slave : slaves_)
                {
                    notify_disable(slave.address);
                }
            });
    }
}

void I2CMaster::notify_xbox360w(bool connected, uint8_t idx)
{
    if (idx < 1 || idx >= MAX_GAMEPADS)
    {
        return;
    }

    slaves_[idx - 1].enabled.store(connected);

    if (!connected)
    {
        //Called from core1 so queue on core0
        TaskQueue::Core0::queue_task(
            [this]()
            {
                for (auto& slave : slaves_)
                {
                    notify_disable(slave.address);
                }
            });
    }
}

bool I2CMaster::slave_detected(uint8_t address)
{
    uint8_t dummy_data = 0;
    int result = i2c_write_timeout_us(I2C_PORT, address, &dummy_data, 0, false, 1000);
    return (result >= 0);
}

void I2CMaster::notify_disable(uint8_t address)
{
    if (!slave_detected(address))
    {
        return;
    }

    int retries = 10;

    while (retries--)
    {
        PacketCMD packet_cmd;
        packet_cmd.packet_id = PacketID::COMMAND;
        packet_cmd.command = Command::DISABLE;
        if (write_blocking(address, &packet_cmd, sizeof(PacketCMD)))
        {
            if (read_blocking(address, &packet_cmd, sizeof(PacketCMD)))
            {
                if (packet_cmd.status == Status::OK)
                {
                    break;
                }
            }
        }
        sleep_ms(1);
    }
}