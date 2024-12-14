#include <cstring>
#include <array>

#include "Board/board_api.h"
#include "OGXMini/OGXMini.h"
#include "I2CDriver/4Channel/I2CSlave.h"

I2CSlave* I2CSlave::this_instance_ = nullptr;

void I2CSlave::initialize(uint8_t address) 
{
    this_instance_ = this;

    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_slave_init(I2C_PORT, address, &slave_handler);
}

void I2CSlave::notify_tuh_mounted(HostDriver::Type host_type)
{
    i2c_disabled_.store(true);
}

void I2CSlave::notify_tuh_unmounted(HostDriver::Type host_type)
{
    i2c_disabled_.store(false);
}

void I2CSlave::process(Gamepad (&gamepads)[MAX_GAMEPADS])
{
    if (i2c_disabled_.load())
    {
        return;
    }
    if (packet_in_.packet_id == static_cast<uint8_t>(PacketID::PAD))
    {
        gamepads[0].set_pad_in(packet_in_.pad_in);

        if (gamepads[0].new_pad_out())
        {
            packet_out_.pad_out = gamepads[0].get_pad_out();
        } 
    }
}

I2CSlave::PacketID I2CSlave::get_packet_id(uint8_t* buffer_in)
{
    switch (static_cast<PacketID>(buffer_in[1]))
    {
        case PacketID::PAD:
            if (buffer_in[0] == sizeof(PacketIn))
            {
                return PacketID::PAD;
            }
            break;
        case PacketID::DISABLE:
            if (buffer_in[0] == sizeof(PacketStatus))
            {
                return PacketID::DISABLE;
            }
            break;
        case PacketID::ENABLE:
            if (buffer_in[0] == sizeof(PacketStatus))
            {
                return PacketID::ENABLE;
            }
            break;
        case PacketID::STATUS:
            if (buffer_in[0] == sizeof(PacketStatus))
            {
                return PacketID::STATUS;
            }
            break;
        default:
            break;
    }
    return PacketID::UNKNOWN;
}

void I2CSlave::slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    static int count = 0;
    static std::array<uint8_t, MAX_PACKET_SIZE> buffer_in{0};
    static std::array<uint8_t, MAX_PACKET_SIZE> buffer_out{0};

    switch (event) 
    {
        case I2C_SLAVE_RECEIVE: // master has written
            if (count < MAX_PACKET_SIZE)
            {
                buffer_in.data()[count] = i2c_read_byte_raw(i2c);
                ++count;
            }
            // else // Something's wrong, reset
            // {
            //     count = 0;
            //     buffer_in.fill(0);
            //     buffer_out.fill(0);
            // }
            break;

        case I2C_SLAVE_FINISH: // master signalled Stop / Restart
            // Each master write has an ID indicating the type of data to send back on the next read request
            // Every write has an associated read
            switch (get_packet_id(buffer_in.data())) 
            {
                case PacketID::PAD:
                    this_instance_->packet_in_ = *reinterpret_cast<PacketIn*>(buffer_in.data());
                    *reinterpret_cast<PacketOut*>(buffer_out.data()) = this_instance_->packet_out_;
                    break;
                case PacketID::STATUS:
                    buffer_out.data()[0] = sizeof(PacketStatus);
                    buffer_out.data()[1] = static_cast<uint8_t>(PacketID::STATUS);
                    // if something is mounted by tuh, signal to not send gamepad data
                    buffer_out.data()[2] = this_instance_->i2c_disabled_.load() ? static_cast<uint8_t>(SlaveStatus::NOT_READY) : static_cast<uint8_t>(SlaveStatus::READY);
                    break;
                case PacketID::ENABLE:
                    buffer_out.data()[0] = sizeof(PacketStatus);
                    buffer_out.data()[1] = static_cast<uint8_t>(PacketID::STATUS);
                    buffer_out.data()[2] = static_cast<uint8_t>(SlaveStatus::RESP_OK);
                    if (!this_instance_->i2c_disabled_.load())
                    {
                        // If no TUH devices are mounted, signal to connect usb
                        OGXMini::update_tuh_status(true);
                    }
                    break;
                case PacketID::DISABLE:
                    buffer_out.data()[0] = sizeof(PacketStatus);
                    buffer_out.data()[1] = static_cast<uint8_t>(PacketID::STATUS);
                    buffer_out.data()[2] = static_cast<uint8_t>(SlaveStatus::RESP_OK);
                    if (!this_instance_->i2c_disabled_.load())
                    {
                        // If no TUH devices are mounted, signal to disconnect usb reset the pico
                        OGXMini::update_tuh_status(false);
                    }
                    break;
                default:
                    break;
            }
            count = 0;
            break;

        case I2C_SLAVE_REQUEST: // master requesting data
            i2c_write_raw_blocking(i2c, buffer_out.data(), buffer_out.data()[0]);
            buffer_in.fill(0);
            break;

        default:
            break;

    }
}