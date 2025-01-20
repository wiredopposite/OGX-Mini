#ifndef _USER_SETTINGS_H_
#define _USER_SETTINGS_H_

#include <cstdint>
#include <atomic>

#include "sdkconfig.h"
#include "I2CDriver/I2CDriver.h"
#include "UserSettings/UserProfile.h"
#include "UserSettings/DeviceDriverTypes.h"
#include "UserSettings/NVSHelper.h"

class UserSettings
{
public:
    static constexpr uint8_t MAX_PROFILES = 8;
    static constexpr uint32_t GP_CHECK_DELAY_MS = 1000;

    static UserSettings& get_instance()
    {
        static UserSettings instance;
        return instance;
    }

    void initialize_flash();

    DeviceDriverType get_current_driver();
    bool check_for_driver_change(const I2CDriver::PacketIn& packet_in);
    
    UserProfile get_profile_by_index(const uint8_t index);
    UserProfile get_profile_by_id(const uint8_t profile_id);
    uint8_t get_active_profile_id(const uint8_t index);

    void store_driver_type(DeviceDriverType new_driver_type);
    void store_profile(uint8_t index, UserProfile& profile);
    void store_profile_and_driver_type(DeviceDriverType new_driver_type, uint8_t index, UserProfile& profile);

private:
    UserSettings() = default;
    ~UserSettings() = default;
    UserSettings(const UserSettings&) = delete;
    UserSettings& operator=(const UserSettings&) = delete;

    static constexpr uint8_t GP_CHECK_COUNT = 3000 / GP_CHECK_DELAY_MS;
    static constexpr uint8_t INIT_FLAG = 0x12;

    NVSHelper& nvs_helper_{NVSHelper::get_instance()};
    DeviceDriverType current_driver_{DeviceDriverType::NONE};

    bool is_valid_driver(DeviceDriverType mode);

    DeviceDriverType DEFAULT_DRIVER();
    const std::string INIT_FLAG_KEY();
    const std::string PROFILE_KEY(const uint8_t profile_id);
    const std::string ACTIVE_PROFILE_KEY(const uint8_t index);
    const std::string DRIVER_TYPE_KEY();
    const std::string FIRMWARE_VER_KEY();
};

#endif // _USER_SETTINGS_H_