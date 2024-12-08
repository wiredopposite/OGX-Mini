#ifndef _USER_SETTINGS_H_
#define _USER_SETTINGS_H_

#include <cstdint>
#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <hardware/flash.h>
#include <hardware/sync.h>

#include "board_config.h"
#include "USBDevice/DeviceDriver/DeviceDriver.h"
#include "UserSettings/UserProfile.h"
#include "Gamepad.h"

class UserSettings
{
public:
    static constexpr uint8_t MAX_PROFILES = 8;
    static constexpr int32_t GP_CHECK_DELAY_MS = 500;

    UserSettings() = default;
    ~UserSettings() = default;

    void initialize_flash();

    bool verify_firmware_version();
    bool write_firmware_version_safe();

    DeviceDriver::Type get_current_driver();
    bool check_for_driver_change(Gamepad& gamepad);
    
    UserProfile get_profile_by_index(const uint8_t index);
    UserProfile get_profile_by_id(const uint8_t profile_id);
    uint8_t get_active_profile_id(const uint8_t index);

    void store_driver_type_safe(DeviceDriver::Type new_driver_type);
    bool store_profile_safe(uint8_t index, const UserProfile& profile);
    bool store_profile_and_driver_type_safe(DeviceDriver::Type new_driver_type, uint8_t index, const UserProfile& profile);

private:
    static constexpr uint8_t GP_CHECK_COUNT = 3000 / GP_CHECK_DELAY_MS;

    //Sectors are counted backwards from the end of the flash, indexed from 1
    static constexpr uint32_t FLASH_SIZE_BYTES = PICO_FLASH_SIZE_BYTES;
    static constexpr uint8_t INIT_FLAG_SECTOR = 1;
    static constexpr uint8_t FIRMWARE_VER_FLAG_SECTOR = 2;
    static constexpr uint8_t DRIVER_TYPE_SECTOR = 3;
    static constexpr uint8_t ACTIVE_PROFILES_SECTOR = 4;
    static constexpr uint8_t PROFILES_START_SECTOR = 6;
    static constexpr uint8_t INIT_FLAG = 0x88;

    static mutex_t flash_mutex_; 
    static DeviceDriver::Type current_driver_;
    
    void store_active_profile_id_unsafe(const uint8_t index, const uint8_t profile_id);
    void store_profile_unsafe(const UserProfile& profile);
    void store_driver_type_unsafe(const DeviceDriver::Type new_mode);
    DeviceDriver::Type get_default_driver();

    static inline uint32_t flash_offset(const uint8_t sector)
    {
        return FLASH_SIZE_BYTES - static_cast<uint32_t>(FLASH_SECTOR_SIZE * sector);
    }

    //Offset relative to sector start, 1 profile per page
    static inline uint32_t profile_offset(const uint8_t profile_id)
    {
        return (static_cast<uint32_t>(profile_id - 1) * FLASH_PAGE_SIZE);
    }
};

#endif // _USER_SETTINGS_H_