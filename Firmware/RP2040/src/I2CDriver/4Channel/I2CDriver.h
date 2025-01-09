#ifndef I2C_DRIVER_4CH_H
#define I2C_DRIVER_4CH_H

#include <cstdint>
#include <algorithm>

#include "board_config.h"
#include "Gamepad.h"
#include "USBHost/HostDriver/HostDriverTypes.h"

//Run on core0
class I2CDriver
{
public:
    virtual ~I2CDriver() {};
    virtual void initialize(uint8_t address) = 0;
    virtual void process(Gamepad (&gamepads)[MAX_GAMEPADS]) = 0;
    virtual void notify_tuh(bool mounted, HostDriverType host_type = HostDriverType::UNKNOWN) = 0;
    virtual void notify_xbox360w(bool connected, uint8_t idx) {};

protected:
    enum class PacketID : uint8_t { UNKNOWN = 0, PAD, COMMAND };
    enum class Command  : uint8_t { UNKNOWN = 0, STATUS, DISABLE };
    enum class Status   : uint8_t { UNKNOWN = 0, NC, ERROR, OK, READY, NOT_READY };

#pragma pack(push, 1)
    struct PacketIn
    {
        uint8_t packet_len{sizeof(PacketIn)};
        PacketID packet_id{PacketID::PAD};
        Gamepad::PadIn pad_in{Gamepad::PadIn()};
        Gamepad::ChatpadIn chatpad_in{0};
        std::array<uint8_t, 4> reserved{0};
    };
    static_assert(sizeof(PacketIn) == 32, "I2CDriver::PacketIn is misaligned");
    static_assert((sizeof(PacketIn) % 8) == 0, "I2CDriver::PacketIn is not a multiple of 8");

    struct PacketOut
    {
        uint8_t packet_len{sizeof(PacketOut)};
        PacketID packet_id{PacketID::PAD};
        Gamepad::PadOut pad_out{Gamepad::PadOut()};
        std::array<uint8_t, 4> reserved{0};
    };
    static_assert(sizeof(PacketOut) == 8, "I2CDriver::PacketOut is misaligned");

    struct PacketCMD
    {
        uint8_t packet_len{sizeof(PacketCMD)};
        PacketID packet_id{PacketID::COMMAND};
        Command command{Command::UNKNOWN};
        Status status{Status::UNKNOWN};
        std::array<uint8_t, 4> reserved{0};
    };
    static_assert(sizeof(PacketCMD) == 8, "I2CDriver::PacketCMD is misaligned");
#pragma pack(pop)

    static constexpr size_t MAX_PACKET_SIZE = sizeof(PacketIn);
};

#endif // I2C_DRIVER_4CH_H