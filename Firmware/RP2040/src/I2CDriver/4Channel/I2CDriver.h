#ifndef I2C_DRIVER_4CH_H
#define I2C_DRIVER_4CH_H

#include <cstdint>
#include <algorithm>

#include "board_config.h"
#include "Gamepad.h"

#include "USBHost/HostDriver/HostDriver.h"

class I2CDriver
{
public:
    virtual ~I2CDriver() = default;
    virtual void initialize(uint8_t address) = 0;
    virtual void process(Gamepad (&gamepads)[MAX_GAMEPADS]) = 0;
    virtual void notify_tuh_mounted(HostDriver::Type host_type = HostDriver::Type::UNKNOWN) = 0;
    virtual void notify_tuh_unmounted(HostDriver::Type host_type = HostDriver::Type::UNKNOWN) = 0;
    virtual void notify_xbox360w_connected(uint8_t idx) = 0;
    virtual void notify_xbox360w_disconnected(uint8_t idx) = 0;

protected:
    enum class PacketID : uint8_t { UNKNOWN = 0, PAD, STATUS, ENABLE, DISABLE };
    enum class SlaveStatus : uint8_t { NC = 0, NOT_READY, READY, RESP_OK };

#pragma pack(push, 1)
    struct PacketIn
    {
        uint8_t packet_len;
        uint8_t packet_id;
        Gamepad::PadIn pad_in;

        PacketIn()
        {
            std::memset(this, 0, sizeof(PacketIn));
            packet_len = sizeof(PacketIn);
            packet_id = static_cast<uint8_t>(PacketID::PAD);
        }
    };
static_assert(sizeof(PacketIn) == 28, "I2CDriver::PacketIn is misaligned");

    struct PacketOut
    {
        uint8_t packet_len;
        uint8_t packet_id;
        Gamepad::PadOut pad_out;    

        PacketOut()
        {
            std::memset(this, 0, sizeof(PacketOut));
            packet_len = sizeof(PacketOut);
            packet_id = static_cast<uint8_t>(PacketID::PAD);
        }
    };
static_assert(sizeof(PacketOut) == 4, "I2CDriver::PacketOut is misaligned");

    struct PacketStatus
    {
        uint8_t packet_len;
        uint8_t packet_id;
        uint8_t status;

        PacketStatus()
        {
            packet_len = sizeof(PacketStatus);
            packet_id = static_cast<uint8_t>(PacketID::STATUS);
            status = static_cast<uint8_t>(SlaveStatus::NC);
        }
    };
static_assert(sizeof(PacketStatus) == 3, "I2CDriver::PacketStatus is misaligned");
#pragma pack(pop)

    static constexpr size_t MAX_PACKET_SIZE = std::max(sizeof(PacketStatus), std::max(sizeof(PacketIn), sizeof(PacketOut)));
};

#endif // I2C_DRIVER_4CH_H