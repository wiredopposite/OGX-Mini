#include <cstring>

#include "TaskQueue/TaskQueue.h"
#include "OGXMini/OGXMini.h"    
#include "I2CDriver/4Channel/I2CMaster.h"

I2CMaster::~I2CMaster()
{
    TaskQueue::Core0::cancel_delayed_task(tid_update_slave_);
}

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

    tid_update_slave_ = TaskQueue::Core0::get_new_task_id();

    TaskQueue::Core0::queue_delayed_task(tid_update_slave_, 2000, true, [this]
    {
        for (auto& slave : slaves_)
        {
            check_slave_status(slave);
            sleep_us(10);
        }
    });
}

void I2CMaster::process(Gamepad (&gamepads)[MAX_GAMEPADS])
{
    for (uint8_t i = 0; i < NUM_SLAVES; ++i)
    {
        Slave& slave = slaves_[i];

        if (slave.status != SlaveStatus::READY || !slave_detected(slave.address))
        {
            continue;
        }
        if (send_packet_in(slave, gamepads[i + 1]))
        {
            get_packet_out(slave, gamepads[i + 1]);
        }

        sleep_us(100);
    }
}

void I2CMaster::notify_tuh_mounted(HostDriver::Type host_type)
{
    if (host_type == HostDriver::Type::XBOX360W)
    {
        i2c_enabled_.store(true);
    }
}

void I2CMaster::notify_tuh_unmounted(HostDriver::Type host_type)
{
    if (host_type == HostDriver::Type::XBOX360W)
    {
        i2c_enabled_.store(false);

        TaskQueue::Core0::queue_task([this]()
        {
            for (auto& slave : slaves_)
            {
                notify_disable(slave);
            }
        });
    }
}

void I2CMaster::notify_xbox360w_connected(uint8_t idx)
{
    if (idx < 1 || idx >= MAX_GAMEPADS)
    {
        return;
    }
    slaves_[idx - 1].enabled.store(true);

    TaskQueue::Core0::queue_task([this, idx]()
    {
        notify_enable(slaves_[idx - 1].address);
    });
}

void I2CMaster::notify_xbox360w_disconnected(uint8_t idx)
{
    if (idx < 1 || idx >= MAX_GAMEPADS)
    {
        return;
    }
    slaves_[idx - 1].enabled.store(false);

    TaskQueue::Core0::queue_task([this, idx]()
    {
        notify_disable(slaves_[idx - 1]);
    });
}

bool I2CMaster::slave_detected(uint8_t address)
{
    uint8_t dummy_data = 0;
    int result = i2c_write_timeout_us(I2C_PORT, address, &dummy_data, 0, false, 1000);
    return (result >= 0);
}

void I2CMaster::check_slave_status(Slave& slave)
{
    if (!slave_detected(slave.address))
    {
        slave.status = SlaveStatus::NC;
        return;
    }
    if (slave.enabled.load())
    {
        slave.status = get_slave_status(slave.address);
    }
    slave.status = SlaveStatus::NOT_READY;
}

I2CMaster::SlaveStatus I2CMaster::get_slave_status(uint8_t address)
{
    PacketStatus status_packet;
    status_packet.packet_id = static_cast<uint8_t>(PacketID::STATUS);

    int count = i2c_write_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(&status_packet), sizeof(PacketStatus), false);
    if (count == sizeof(PacketStatus))
    {
        count = i2c_read_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(&status_packet), sizeof(PacketStatus), false);
        return static_cast<SlaveStatus>(status_packet.status);
    }
    return SlaveStatus::NC;
}

I2CMaster::SlaveStatus I2CMaster::notify_enable(uint8_t address)
{
    PacketStatus status_packet;
    status_packet.packet_id = static_cast<uint8_t>(PacketID::ENABLE);

    int count = i2c_write_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(&status_packet), sizeof(PacketStatus), false);
    if (count == sizeof(PacketStatus))
    {
        count = i2c_read_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(&status_packet), sizeof(PacketStatus), false);
        return static_cast<SlaveStatus>(status_packet.status);
    }
    return SlaveStatus::NC;
}

bool I2CMaster::notify_disable(Slave& slave)
{
    if (slave_detected(slave.address) && slave.enabled.load())
    {
        int retries = 6;
        bool success = false;

        while (!success && retries--)
        {
            PacketStatus status_packet;
            status_packet.packet_id = static_cast<uint8_t>(PacketID::DISABLE);

            int count = i2c_write_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&status_packet), sizeof(PacketStatus), false);
            if (count == sizeof(PacketStatus))
            {
                count = i2c_read_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&status_packet), sizeof(PacketStatus), false);
                success = (static_cast<SlaveStatus>(status_packet.status) == SlaveStatus::RESP_OK);
            }

            sleep_ms(1);
        }
        return success;
    }
    return false;
}

bool I2CMaster::send_packet_in(Slave& slave, Gamepad& gamepad)
{
    static PacketIn packet_in = PacketIn();

    Gamepad::PadIn pad_in = gamepad.get_pad_in();
    packet_in.pad_in = pad_in;

    int count = i2c_write_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&packet_in), sizeof(packet_in), false);
    return (count == sizeof(PacketIn));
}

bool I2CMaster::get_packet_out(Slave& slave, Gamepad& gamepad)
{
    static PacketOut packet_out = PacketOut();

    int count = i2c_read_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&packet_out), sizeof(PacketOut), false);
    if (count != sizeof(PacketOut))
    {
        return false;
    }

    gamepad.set_pad_out(packet_out.pad_out);
    return true;
}