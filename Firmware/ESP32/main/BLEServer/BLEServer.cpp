#include <cstring>
#include <string>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include "att_server.h"
#include "btstack.h"

#include "BTManager/BTManager.h" 
#include "Gamepad/Gamepad.h"
#include "BLEServer/BLEServer.h"
#include "BLEServer/att_delayed_response.h"
#include "UserSettings/UserProfile.h"
#include "UserSettings/UserSettings.h"

namespace BLEServer {

constexpr uint16_t PACKET_LEN_MAX = 20;
constexpr size_t GAMEPAD_LEN = 23;

namespace Handle
{
    constexpr uint16_t FW_VERSION    = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789020_01_VALUE_HANDLE;
    constexpr uint16_t FW_NAME       = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789021_01_VALUE_HANDLE;

    constexpr uint16_t SETUP_READ  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789030_01_VALUE_HANDLE;
    constexpr uint16_t SETUP_WRITE = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789031_01_VALUE_HANDLE;
    constexpr uint16_t GET_SETUP   = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789032_01_VALUE_HANDLE;

    constexpr uint16_t PROFILE  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789040_01_VALUE_HANDLE;

    constexpr uint16_t GAMEPAD  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789050_01_VALUE_HANDLE;
}

namespace ADV
{
    // Flags general discoverable, BR/EDR not supported
    static const uint8_t FLAGS[]    = { 0x02, 0x01, 0x06 };
    static const uint8_t NAME_TYPE  = 0x09;

    #pragma pack(push, 1)
    struct Data
    {
        uint8_t flags[sizeof(FLAGS)];
        uint8_t name_len;
        uint8_t name_type;
        uint8_t name[sizeof(FIRMWARE_NAME) - 1];

        Data()
        {
            std::memcpy(flags, FLAGS, sizeof(flags));
            name_len = sizeof(FIRMWARE_NAME);
            name_type = NAME_TYPE;
            std::string fw_name = FIRMWARE_NAME;
            std::memcpy(name, fw_name.c_str(), std::min(sizeof(name), fw_name.size()));
        }
    };
    static_assert(sizeof(Data) == 5 + sizeof(FIRMWARE_NAME) - 1, "BLEServer::ADV::Data struct size mismatch");
    #pragma pack(pop)
}

#pragma pack(push, 1)
struct SetupPacket
{
    DeviceDriverType device_type{DeviceDriverType::NONE};
    uint8_t max_gamepads{MAX_GAMEPADS};
    uint8_t player_idx{0};
    uint8_t profile_id{0};
};
static_assert(sizeof(SetupPacket) == 4, "BLEServer::SetupPacket struct size mismatch");
#pragma pack(pop)

class ProfileReader
{
public:
    ProfileReader() = default;
    ~ProfileReader() = default;

    void set_setup_packet(const SetupPacket& setup_packet)
    {
        setup_packet_ = setup_packet;
        current_offset_ = 0;
    }
    const SetupPacket& get_setup_packet() const
    {
        return setup_packet_;
    }
    uint16_t get_xfer_len()
    {
        return static_cast<uint16_t>(std::min(static_cast<size_t>(PACKET_LEN_MAX), sizeof(UserProfile) - current_offset_));
    }
    uint16_t get_profile_data(uint8_t* buffer, uint16_t buffer_len)
    {
        size_t copy_len = get_xfer_len();
        if (!buffer || buffer_len < copy_len)
        {
            return 0;
        }

        if (current_offset_ == 0 && !set_profile())
        {
            return 0;
        }

        std::memcpy(buffer, reinterpret_cast<uint8_t*>(&profile_) + current_offset_, copy_len);

        current_offset_ += copy_len;
        if (current_offset_ >= sizeof(UserProfile))
        {
            current_offset_ = 0;
            OGXM_LOG("ProfileReader: Read complete for profile ID: %i\n", profile_.id);
        }
        return copy_len;
    }

private:
    SetupPacket setup_packet_;
    UserProfile profile_;
    size_t current_offset_ = 0;

    bool set_profile()
    {
        if (setup_packet_.profile_id == 0xFF)
        {
            if (setup_packet_.player_idx >= UserSettings::MAX_PROFILES)
            {
                return false;
            }
            profile_ = UserSettings::get_instance().get_profile_by_index(setup_packet_.player_idx);
            OGXM_LOG("ProfileReader: Reading profile for player %d\n", setup_packet_.player_idx);
        }
        else
        {
            if (setup_packet_.profile_id > UserSettings::MAX_PROFILES)
            {
                return false;
            }
            profile_ = UserSettings::get_instance().get_profile_by_id(setup_packet_.profile_id);
            OGXM_LOG("ProfileReader: Reading profile with ID %d\n", setup_packet_.profile_id);
        }
        return true;
    }
};

class ProfileWriter
{
public:
    ProfileWriter() = default;
    ~ProfileWriter() = default;

    void set_setup_packet(const SetupPacket& setup_packet)
    {
        setup_packet_ = setup_packet;
        current_offset_ = 0;
    }
    const SetupPacket& get_setup_packet() const
    {
        return setup_packet_;
    }
    uint16_t get_xfer_len()
    {
        return static_cast<uint16_t>(std::min(static_cast<size_t>(PACKET_LEN_MAX), sizeof(UserProfile) - current_offset_));
    }
    size_t set_profile_data(const uint8_t* buffer, uint16_t buffer_len)
    {
        size_t copy_len = get_xfer_len();
        if (!buffer || buffer_len < copy_len)
        {
            return 0;
        }

        std::memcpy(reinterpret_cast<uint8_t*>(&profile_) + current_offset_, buffer, copy_len);

        current_offset_ += copy_len;
        size_t ret = current_offset_;

        if (current_offset_ >= sizeof(UserProfile))
        {
            current_offset_ = 0;
        }
        return ret;
    }
    bool commit_profile()
    {
        if (setup_packet_.device_type != DeviceDriverType::NONE)
        {
            UserSettings::get_instance().store_profile_and_driver_type(setup_packet_.device_type, setup_packet_.player_idx, profile_);
        }
        else
        {
            UserSettings::get_instance().store_profile(setup_packet_.player_idx, profile_);
        }
        return true;
    }

private:
    SetupPacket setup_packet_;
    UserProfile profile_;
    size_t current_offset_ = 0;
};

ProfileReader profile_reader_;
ProfileWriter profile_writer_;

static int verify_write(const uint16_t buffer_size, const uint16_t expected_size)
{
    if (buffer_size != expected_size)
    {
        return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
    }
    return 0;
}

static uint16_t att_read_callback(  hci_con_handle_t connection_handle,
                                    uint16_t att_handle,
                                    uint16_t offset,
                                    uint8_t *buffer,
                                    uint16_t buffer_size)
{
    std::string fw_version;
    std::string fw_name;

    switch (att_handle)
    {
        case Handle::FW_VERSION:
            fw_version = FIRMWARE_VERSION;
            if (buffer)
            {
                std::memcpy(buffer, reinterpret_cast<const uint8_t*>(fw_version.c_str()), fw_version.size());
            }
            return static_cast<uint16_t>(fw_version.size());

        case Handle::FW_NAME:
            fw_name = FIRMWARE_NAME;
            if (buffer)
            {
                std::memcpy(buffer, reinterpret_cast<const uint8_t*>(fw_name.c_str()), fw_name.size());;
            }
            return static_cast<uint16_t>(fw_name.size());

        case Handle::GET_SETUP:
            if (buffer)
            {
                buffer[0] = static_cast<uint8_t>(UserSettings::get_instance().get_current_driver());
                buffer[1] = MAX_GAMEPADS;
                buffer[2] = 0;
                buffer[3] = UserSettings::get_instance().get_active_profile_id(0);
            }
            return static_cast<uint16_t>(sizeof(SetupPacket));

        case Handle::PROFILE:
            if (buffer)
            {
                return profile_reader_.get_profile_data(buffer, buffer_size);
            }
            return profile_reader_.get_xfer_len();

        case Handle::GAMEPAD:
            if (buffer)
            {
                I2CDriver::PacketIn packet_in = BTManager::get_instance().get_packet_in(0);
                std::memcpy(buffer, &packet_in.dpad, 13);
            }
            return static_cast<uint16_t>(13);

        default:
            break;
    }
    return 0;
}

static int att_write_callback(  hci_con_handle_t connection_handle,
                                uint16_t att_handle,
                                uint16_t transaction_mode,
                                uint16_t offset,
                                uint8_t *buffer,
                                uint16_t buffer_size)
{
    int ret = 0;

    switch (att_handle)
    {
        case Handle::SETUP_READ:
            if ((ret = verify_write(buffer_size, sizeof(SetupPacket))) != 0)
            {
                break;
            }
            profile_reader_.set_setup_packet(*reinterpret_cast<SetupPacket*>(buffer));
            break;

        case Handle::SETUP_WRITE:
            if ((ret = verify_write(buffer_size, sizeof(SetupPacket))) != 0)
            {
                break;
            }
            profile_writer_.set_setup_packet(*reinterpret_cast<SetupPacket*>(buffer));
            break;

        case Handle::PROFILE:
            if ((ret = verify_write(buffer_size, profile_writer_.get_xfer_len())) != 0)
            {
                break;
            }
            if (profile_writer_.set_profile_data(buffer, buffer_size) == sizeof(UserProfile))
            {
                profile_writer_.commit_profile();
            }
            break;

        default:
            break;
    }
    return ret;
}

void init_server()
{
    att_server_init(profile_data, att_read_callback, att_write_callback);

    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;

    bd_addr_t null_addr;
    std::memset(null_addr, 0, sizeof(null_addr));

    static ADV::Data adv_data = ADV::Data();

    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(static_cast<uint8_t>(sizeof(adv_data)), reinterpret_cast<uint8_t*>(&adv_data));
    gap_advertisements_enable(1);
}

} // namespace BLEServer