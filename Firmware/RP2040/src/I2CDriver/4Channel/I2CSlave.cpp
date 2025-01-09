#include <cstring>
#include <array>
#include <hardware/gpio.h>

#include "OGXMini/OGXMini.h"
#include "I2CDriver/4Channel/I2CSlave.h"
#include "TaskQueue/TaskQueue.h"

I2CSlave* I2CSlave::instance_ = nullptr;

void I2CSlave::initialize(uint8_t address) 
{
    instance_ = this;

    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_slave_init(I2C_PORT, address, &slave_handler);
}

void I2CSlave::notify_tuh(bool mounted, HostDriverType host_type)
{
    tuh_mounted_.store(mounted);
}

void I2CSlave::process(Gamepad (&gamepads)[MAX_GAMEPADS])
{
    if (tuh_mounted_.load())
    {
        return;
    }
    //Don't want to hang up the i2c bus by doing this in the slave handler
    if (new_pad_in_.load())
    {
        new_pad_in_.store(false);
        gamepads[0].set_pad_in(packet_in_.pad_in);
        gamepads[0].set_chatpad_in(packet_in_.chatpad_in);
    }
    
    if (gamepads[0].new_pad_out())
    {
        packet_out_.pad_out = gamepads[0].get_pad_out();
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

        case PacketID::COMMAND:
            if (buffer_in[0] == sizeof(PacketCMD))
            {
                return PacketID::COMMAND;
            }
            break;

        default:
            break;
    }
    return PacketID::UNKNOWN;
}

void I2CSlave::slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    static size_t count = 0;
    static bool enabled = false;
    static uint8_t buffer_in[MAX_PACKET_SIZE];
    static uint8_t buffer_out[MAX_PACKET_SIZE];

    PacketIn  *packet_in_p = reinterpret_cast<PacketIn*>(buffer_in);
    PacketOut *packet_out_p = reinterpret_cast<PacketOut*>(buffer_out);
    PacketCMD *packet_cmd_in_p = reinterpret_cast<PacketCMD*>(buffer_in);
    PacketCMD *packet_cmd_out_p = reinterpret_cast<PacketCMD*>(buffer_out);

    switch (event) 
    {
        case I2C_SLAVE_RECEIVE: // master has written
            if (count < MAX_PACKET_SIZE)
            {
                buffer_in[count] = i2c_read_byte_raw(i2c);
                ++count;
            }
            break;

        case I2C_SLAVE_FINISH:
            // Each master write has an ID indicating the type of data to send back on the next read request
            // Every write has an associated read
            switch (get_packet_id(buffer_in)) 
            {
                case PacketID::PAD:
                    instance_->packet_in_ = *packet_in_p;
                    *packet_out_p = instance_->packet_out_;
                    instance_->new_pad_in_.store(true);
                    break;

                case PacketID::COMMAND:
                    switch (packet_cmd_in_p->command)
                    {
                        case Command::DISABLE:
                            packet_cmd_out_p->packet_len = sizeof(PacketCMD);
                            packet_cmd_out_p->packet_id = PacketID::COMMAND;
                            packet_cmd_out_p->command = Command::DISABLE;
                            packet_cmd_out_p->status = Status::OK;
                            if (!instance_->tuh_mounted_.load())
                            {
                                OGXMini::host_mounted(false);
                            }
                            break;

                        case Command::STATUS:
                            packet_cmd_out_p->packet_len = sizeof(PacketCMD);
                            packet_cmd_out_p->packet_id = PacketID::COMMAND;
                            packet_cmd_out_p->command = Command::STATUS;
                            packet_cmd_out_p->status = instance_->tuh_mounted_.load() ? Status::NOT_READY : Status::READY;
                            if (!instance_->tuh_mounted_.load() && !enabled)
                            {
                                enabled = true;
                                OGXMini::host_mounted(true);
                            }
                            break;

                        default:
                            break;
                    }
                    break;
            }
            count = 0;
            std::memset(buffer_in, 0, sizeof(buffer_in));
            break;

        case I2C_SLAVE_REQUEST:
            i2c_write_raw_blocking(i2c, buffer_out, buffer_out[0]);
            break;

        default:
            break;
    }
}