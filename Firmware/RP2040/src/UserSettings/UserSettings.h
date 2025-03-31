#ifndef _USER_SETTINGS_H_
#define _USER_SETTINGS_H_

#include <cstdint>
#include <string>

#include "Board/Config.h"
#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"
#include "UserSettings/UserProfile.h"
#include "UserSettings/NVSTool.h"
#include "Gamepad/Gamepad.h"

/* Only write/store flash from Core0 */
class UserSettings
{
public:
    static constexpr uint8_t MAX_PROFILES = 8;
    static constexpr int32_t GP_CHECK_DELAY_MS = 600;

    static UserSettings& get_instance()
    {
        static UserSettings instance;
        return instance;
    }

    void initialize_flash();

    bool is_valid_driver(DeviceDriverType driver);
    bool verify_datetime();
    void write_datetime();

    DeviceDriverType get_current_driver();
    bool check_for_driver_change(Gamepad& gamepad);
    
    UserProfile get_profile_by_index(const uint8_t index);
    UserProfile get_profile_by_id(const uint8_t profile_id);
    uint8_t get_active_profile_id(const uint8_t index);

    void store_driver_type(DeviceDriverType new_driver_type);
    bool store_profile(uint8_t index, const UserProfile& profile);
    bool store_profile_and_driver_type(DeviceDriverType new_driver_type, uint8_t index, const UserProfile& profile);

private:
    UserSettings() = default;
    ~UserSettings() = default;
    UserSettings(const UserSettings&) = delete;
    UserSettings& operator=(const UserSettings&) = delete;

    static constexpr uint8_t GP_CHECK_COUNT = 3000 / GP_CHECK_DELAY_MS;
    static constexpr uint8_t FLASH_INIT_FLAG = 0xF8;
    const std::string DATETIME_TAG = BUILD_DATETIME; 
    
    NVSTool& nvs_tool_{NVSTool::get_instance()};
    DeviceDriverType current_driver_{DeviceDriverType::NONE};
    
    DeviceDriverType DEFAULT_DRIVER();
    const std::string INIT_FLAG_KEY();
    const std::string PROFILE_KEY(const uint8_t profile_id);
    const std::string ACTIVE_PROFILE_KEY(const uint8_t index);
    const std::string DRIVER_TYPE_KEY();
    const std::string DATETIME_KEY();
};

#endif // _USER_SETTINGS_H_