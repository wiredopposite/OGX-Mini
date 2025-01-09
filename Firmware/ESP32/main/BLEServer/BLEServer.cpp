#include <cstring>
#include <string>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include "att_server.h"
#include "btstack.h"

#include "Gamepad.h"
#include "BLEServer/BLEServer.h"
#include "BLEServer/att_delayed_response.h"
#include "UserSettings/UserProfile.h"
#include "UserSettings/UserSettings.h"

namespace BLEServer {

static constexpr uint16_t PACKET_LEN_MAX = 18;

#pragma pack(push, 1)
struct SetupPacket
{
    uint8_t max_gamepads{1};
    uint8_t index{0};
    uint8_t device_type{0};
    uint8_t profile_id{1};
};
static_assert(sizeof(SetupPacket) == 4, "BLEServer::SetupPacket struct size mismatch");
#pragma pack(pop)

SetupPacket setup_packet_;

namespace Handle
{
    static constexpr uint16_t FW_VERSION    = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789020_01_VALUE_HANDLE;
    static constexpr uint16_t FW_NAME       = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789021_01_VALUE_HANDLE;

    static constexpr uint16_t START_UPDATE  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789030_01_VALUE_HANDLE;
    static constexpr uint16_t COMMIT_UPDATE = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789031_01_VALUE_HANDLE;

    static constexpr uint16_t SETUP_PACKET  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789040_01_VALUE_HANDLE;
    static constexpr uint16_t PROFILE_PT1   = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789041_01_VALUE_HANDLE;
    static constexpr uint16_t PROFILE_PT2   = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789042_01_VALUE_HANDLE;
    static constexpr uint16_t PROFILE_PT3   = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789043_01_VALUE_HANDLE;
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
            std::memcpy(name, FIRMWARE_NAME, sizeof(name));
        }
    };
    static_assert(sizeof(Data) == 5 + sizeof(FIRMWARE_NAME) - 1, "BLEServer::ADV::Data struct size mismatch");
    #pragma pack(pop)
}

static int verify_write(const uint16_t buffer_size, const uint16_t expected_size, bool pending_write = false, bool expected_pending_write = false)
{
    if (buffer_size != expected_size)
    {
        return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
    }
    if (pending_write != expected_pending_write)
    {
        return ATT_ERROR_WRITE_NOT_PERMITTED;
    }
    return 0;
}

static uint16_t att_read_callback(  hci_con_handle_t connection_handle,
                                    uint16_t att_handle,
                                    uint16_t offset,
                                    uint8_t *buffer,
                                    uint16_t buffer_size)
{
    static UserProfile profile;
    SetupPacket setup_packet_resp;
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
                std::memcpy(buffer, reinterpret_cast<const uint8_t*>(fw_name.c_str()), fw_name.size());
            }
            return static_cast<uint16_t>(fw_name.size());

        case Handle::SETUP_PACKET:
            if (buffer)
            {
                //App has already written a setup packet with the index
                setup_packet_resp.max_gamepads = static_cast<uint8_t>(MAX_GAMEPADS);
                setup_packet_resp.index = setup_packet_.index;
                setup_packet_resp.device_type = static_cast<uint8_t>(UserSettings::get_instance().get_current_driver());
                setup_packet_resp.profile_id = UserSettings::get_instance().get_active_profile_id(setup_packet_.index);

                std::memcpy(buffer, &setup_packet_resp, sizeof(setup_packet_resp));
            }
            return sizeof(setup_packet_);

        case Handle::PROFILE_PT1:
            if (buffer)
            {
                //App has already written the profile id it wants to the setup packet
                profile = UserSettings::get_instance().get_profile_by_id(setup_packet_.profile_id);
                std::memcpy(buffer, &profile, PACKET_LEN_MAX);
            }
            return PACKET_LEN_MAX;

        case Handle::PROFILE_PT2:
            if (buffer)
            {
                std::memcpy(buffer, reinterpret_cast<uint8_t*>(&profile) + PACKET_LEN_MAX, PACKET_LEN_MAX);
            }
            return PACKET_LEN_MAX;

        case Handle::PROFILE_PT3:
            if (buffer)
            {
                std::memcpy(buffer, reinterpret_cast<uint8_t*>(&profile) + PACKET_LEN_MAX * 2, sizeof(UserProfile) - PACKET_LEN_MAX * 2);
            }
            return sizeof(UserProfile) - PACKET_LEN_MAX * 2;

        default:
            break;
    }
    return 0;
}

static int att_write_callback(hci_con_handle_t connection_handle,
                              uint16_t att_handle,
                              uint16_t transaction_mode,
                              uint16_t offset,
                              uint8_t *buffer,
                              uint16_t buffer_size)
{
    static UserProfile temp_profile;
    static bool pending_write = false;

    int ret = 0;

    switch (att_handle)
    {
        case Handle::START_UPDATE:
            pending_write = true;
            break;

        case Handle::SETUP_PACKET:
            if ((ret = verify_write(buffer_size, sizeof(SetupPacket))) != 0)
            {
                break;
            }

            std::memcpy(&setup_packet_, buffer, buffer_size);
            if (setup_packet_.index >= MAX_GAMEPADS)
            {
                setup_packet_.index = 0;
                ret = ATT_ERROR_OUT_OF_RANGE;
            }
            if (setup_packet_.profile_id > UserSettings::MAX_PROFILES)
            {
                setup_packet_.profile_id = 1;
                ret = ATT_ERROR_OUT_OF_RANGE;
            }
            if (ret)
            {
                break;
            }

            if (pending_write)
            {
                //App wants to store a new device driver type
                UserSettings::get_instance().store_driver_type(static_cast<DeviceDriverType>(setup_packet_.device_type));
            }
            break;

        case Handle::PROFILE_PT1:
            if ((ret = verify_write(buffer_size, PACKET_LEN_MAX, pending_write, true)) != 0)
            {
                break;
            }
            std::memcpy(&temp_profile, buffer, buffer_size);
            break;

        case Handle::PROFILE_PT2:
            if ((ret = verify_write(buffer_size, PACKET_LEN_MAX, pending_write, true)) != 0)
            {
                break;
            }
            std::memcpy(reinterpret_cast<uint8_t*>(&temp_profile) + PACKET_LEN_MAX, buffer, buffer_size);
            break;

        case Handle::PROFILE_PT3:
            if ((ret = verify_write(buffer_size, sizeof(UserProfile) - PACKET_LEN_MAX * 2, pending_write, true)) != 0)
            {
                break;
            }
            std::memcpy(reinterpret_cast<uint8_t*>(&temp_profile) + PACKET_LEN_MAX * 2, buffer, buffer_size);
            break;

        case Handle::COMMIT_UPDATE:
            if ((ret = verify_write(0, 0, pending_write, true)) != 0)
            {
                break;
            }
            UserSettings::get_instance().store_profile(setup_packet_.index, temp_profile);
            pending_write = false;
            break;

        default:
            break;
    }
    return ret;
}

void init_server()
{
    UserSettings::get_instance().initialize_flash();

    // setup ATT server
    att_server_init(profile_data, att_read_callback, att_write_callback);

    // setup advertisements
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