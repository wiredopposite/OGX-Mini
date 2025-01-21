#include <cstring>
#include <array>
#include <string>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_err.h>
#include <esp_log.h>

#include "Gamepad/Gamepad.h"
#include "UserSettings/NVSHelper.h"

static constexpr uint32_t BUTTON_COMBO(const uint16_t& buttons, const uint8_t& dpad = 0)
{
    return (static_cast<uint32_t>(buttons) << 16) | static_cast<uint32_t>(dpad);
}

namespace ButtonCombo 
{
    static constexpr uint32_t PS3       = BUTTON_COMBO(Gamepad::BUTTON_START, Gamepad::DPAD_LEFT);
    static constexpr uint32_t DINPUT    = BUTTON_COMBO(Gamepad::BUTTON_START | Gamepad::BUTTON_RB, Gamepad::DPAD_LEFT);
    static constexpr uint32_t XINPUT    = BUTTON_COMBO(Gamepad::BUTTON_START, Gamepad::DPAD_UP);
    static constexpr uint32_t SWITCH    = BUTTON_COMBO(Gamepad::BUTTON_START, Gamepad::DPAD_DOWN);
    static constexpr uint32_t XBOXOG    = BUTTON_COMBO(Gamepad::BUTTON_START, Gamepad::DPAD_RIGHT);
    static constexpr uint32_t XBOXOG_SB = BUTTON_COMBO(Gamepad::BUTTON_START | Gamepad::BUTTON_RB, Gamepad::DPAD_RIGHT);
    static constexpr uint32_t XBOXOG_XR = BUTTON_COMBO(Gamepad::BUTTON_START | Gamepad::BUTTON_LB, Gamepad::DPAD_RIGHT);
    static constexpr uint32_t PSCLASSIC = BUTTON_COMBO(Gamepad::BUTTON_START | Gamepad::BUTTON_A);
    static constexpr uint32_t WEBAPP    = BUTTON_COMBO(Gamepad::BUTTON_START | Gamepad::BUTTON_LB | Gamepad::BUTTON_RB);
};

static constexpr DeviceDriverType VALID_DRIVER_TYPES[] =
{
#if MAX_GAMEPADS > 1
    DeviceDriverType::DINPUT, 
    DeviceDriverType::SWITCH, 

#else // MAX_GAMEPADS == 1
    DeviceDriverType::XBOXOG, 
    // DeviceDriverType::XBOXOG_SB, 
    DeviceDriverType::XBOXOG_XR,
    DeviceDriverType::DINPUT, 
    DeviceDriverType::SWITCH, 
    DeviceDriverType::PS3,
    DeviceDriverType::PSCLASSIC, 
    DeviceDriverType::XINPUT,

#endif
};

struct ComboMap { uint32_t combo; DeviceDriverType driver; };
static constexpr std::array<ComboMap, 9> BUTTON_COMBO_MAP = 
{{
    { ButtonCombo::XBOXOG,    DeviceDriverType::XBOXOG    },
    { ButtonCombo::XBOXOG_SB, DeviceDriverType::XBOXOG_SB },
    { ButtonCombo::XBOXOG_XR, DeviceDriverType::XBOXOG_XR },
    { ButtonCombo::WEBAPP,    DeviceDriverType::WEBAPP    },
    { ButtonCombo::DINPUT,    DeviceDriverType::DINPUT    },
    { ButtonCombo::SWITCH,    DeviceDriverType::SWITCH    },
    { ButtonCombo::XINPUT,    DeviceDriverType::XINPUT    },
    { ButtonCombo::PS3,       DeviceDriverType::PS3       },
    { ButtonCombo::PSCLASSIC, DeviceDriverType::PSCLASSIC }
}};

const std::string UserSettings::INIT_FLAG_KEY()
{
    return std::string("init_flag");
}

const std::string UserSettings::PROFILE_KEY(const uint8_t profile_id)
{
    return std::string("profile_") + std::to_string(profile_id);
}

const std::string UserSettings::ACTIVE_PROFILE_KEY(const uint8_t index)
{
    return std::string("active_id_") + std::to_string(index);
}

const std::string UserSettings::DRIVER_TYPE_KEY()
{
    return std::string("driver_type");
}

const std::string UserSettings::FIRMWARE_VER_KEY()
{
    return std::string("firmware_ver");
}

void UserSettings::initialize_flash()
{
    ESP_LOGD("UserSettings", "Checking for UserSettings init flag");

    uint8_t init_flag = 0;

    ESP_ERROR_CHECK(nvs_helper_.read(INIT_FLAG_KEY(), &init_flag, sizeof(init_flag)));

    if (init_flag == INIT_FLAG)
    {
        ESP_LOGD("UserSettings", "UserSettings already initialized");
        return;
    }

    ESP_ERROR_CHECK(nvs_helper_.erase_all());
    OGXM_LOG("Initializing UserSettings\n");

    current_driver_ = DEFAULT_DRIVER();
    uint8_t driver_type = static_cast<uint8_t>(current_driver_);
    ESP_ERROR_CHECK(nvs_helper_.write(DRIVER_TYPE_KEY(), &driver_type, sizeof(driver_type)));

    uint8_t active_id = 1;
    for (uint8_t i = 0; i < MAX_GAMEPADS; i++)
    {
        ESP_ERROR_CHECK(nvs_helper_.write(ACTIVE_PROFILE_KEY(i), &active_id, sizeof(active_id)));
    }

    UserProfile profile = UserProfile();
    for (uint8_t i = 0; i < MAX_PROFILES; i++)
    {
        profile.id = i + 1;
        ESP_ERROR_CHECK(nvs_helper_.write(PROFILE_KEY(profile.id), &profile, sizeof(UserProfile)));
    }

    init_flag = INIT_FLAG;
    ESP_ERROR_CHECK(nvs_helper_.write(INIT_FLAG_KEY(), &init_flag, sizeof(init_flag)));
}

DeviceDriverType UserSettings::get_current_driver()
{
    if (current_driver_ != DeviceDriverType::NONE)
    {
        return current_driver_;
    }

    uint8_t stored_type = 0;
    nvs_helper_.read(DRIVER_TYPE_KEY(), &stored_type, sizeof(stored_type));

    if (is_valid_driver(static_cast<DeviceDriverType>(stored_type)))
    {
        current_driver_ = static_cast<DeviceDriverType>(stored_type);
        return current_driver_;
    }

    current_driver_ = DEFAULT_DRIVER();
    return current_driver_;
}

//Checks if button combo has been held for 3 seconds, returns true if mode has been changed
bool UserSettings::check_for_driver_change(const I2CDriver::PacketIn& packet_in)
{
    static uint32_t last_button_combo = BUTTON_COMBO(packet_in.buttons, packet_in.dpad);
    static uint8_t call_count = 0;

    uint32_t current_button_combo = BUTTON_COMBO(packet_in.buttons, packet_in.dpad);

    if (!(current_button_combo & (static_cast<uint32_t>(Gamepad::BUTTON_START) << 16)) || 
        last_button_combo != current_button_combo)
    {
        last_button_combo = current_button_combo;
        call_count = 0;
        return false;
    }

    ++call_count;

    if (call_count < GP_CHECK_COUNT)
    {
        return false;
    }

    call_count = 0;

    DeviceDriverType new_driver = DeviceDriverType::NONE;

    for (const auto& combo : BUTTON_COMBO_MAP)
    {
        if (combo.combo == current_button_combo &&
            is_valid_driver(combo.driver))
        {
            new_driver = combo.driver;
            break;
        }
    }

    if (new_driver == DeviceDriverType::NONE || new_driver == current_driver_)
    {
        return false;
    }

    current_driver_ = new_driver;
    return true;
}

void UserSettings::store_driver_type(DeviceDriverType new_driver_type)
{
    if (!is_valid_driver(new_driver_type))
    {
        return;
    }

    uint8_t new_driver = static_cast<uint8_t>(new_driver_type);
    nvs_helper_.write(DRIVER_TYPE_KEY(), &new_driver, sizeof(new_driver));
}

void UserSettings::store_profile(const uint8_t index, UserProfile& profile)
{
    if (index > MAX_GAMEPADS || profile.id < 1 || profile.id > MAX_PROFILES)
    {
        return;
    }

    OGXM_LOG("Storing profile %d for gamepad %d\n", profile.id, index);

    if (nvs_helper_.write(PROFILE_KEY(profile.id), &profile, sizeof(UserProfile)) == ESP_OK)
    {
        OGXM_LOG("Profile %d stored successfully\n", profile.id);

        nvs_helper_.write(ACTIVE_PROFILE_KEY(index), &profile.id, sizeof(profile.id));
    }
}

void UserSettings::store_profile_and_driver_type(DeviceDriverType new_driver_type, const uint8_t index, UserProfile& profile)
{
    store_driver_type(new_driver_type);
    store_profile(index, profile);
}

uint8_t UserSettings::get_active_profile_id(const uint8_t index)
{
    uint8_t read_profile_id = 0;

    if (nvs_helper_.read(ACTIVE_PROFILE_KEY(index), &read_profile_id, sizeof(read_profile_id)) != ESP_OK ||
        read_profile_id < 1 || 
        read_profile_id > MAX_PROFILES)
    {
        return 0x01;
    }
    return read_profile_id;
}

UserProfile UserSettings::get_profile_by_index(const uint8_t index)
{
    return get_profile_by_id(get_active_profile_id(index));
}

UserProfile UserSettings::get_profile_by_id(const uint8_t profile_id)
{
    UserProfile profile;

    if (profile_id < 1 || 
        profile_id > MAX_PROFILES ||
        nvs_helper_.read(PROFILE_KEY(profile_id), &profile, sizeof(UserProfile)) != ESP_OK)
    {
        return UserProfile();
    }

    return profile;
}

DeviceDriverType UserSettings::DEFAULT_DRIVER()
{
    return VALID_DRIVER_TYPES[0];
}

bool UserSettings::is_valid_driver(DeviceDriverType mode)
{
    for (const auto& driver : VALID_DRIVER_TYPES)
    {
        if (mode == driver)
        {
            return true;
        }
    }
    return false;
}