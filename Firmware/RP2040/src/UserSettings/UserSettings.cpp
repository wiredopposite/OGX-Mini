#include <cstring>
#include <array>
#include <memory>
#include <pico/multicore.h>

#include "tusb.h"

#include "Board/ogxm_log.h"
#include "Board/board_api.h"
#include "UserSettings/UserSettings.h"

static constexpr uint32_t BUTTON_COMBO(const uint16_t& buttons, const uint8_t& dpad = 0) {
    return (static_cast<uint32_t>(buttons) << 16) | static_cast<uint32_t>(dpad);
}

namespace ButtonCombo {
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

static constexpr DeviceDriverType VALID_DRIVER_TYPES[] = {
#if defined(CONFIG_EN_4CH)
    DeviceDriverType::XBOXOG, 
    DeviceDriverType::XBOXOG_SB, 
    DeviceDriverType::XINPUT,
    DeviceDriverType::PS3,
    DeviceDriverType::PSCLASSIC, 
    DeviceDriverType::WEBAPP,
    #if defined(XREMOTE_ROM_AVAILABLE)
    DeviceDriverType::XBOXOG_XR,
    #endif

#elif MAX_GAMEPADS > 1
    DeviceDriverType::DINPUT, 
    DeviceDriverType::SWITCH, 
    DeviceDriverType::WEBAPP,

#else // MAX_GAMEPADS == 1
    DeviceDriverType::XBOXOG, 
    DeviceDriverType::XBOXOG_SB, 
    DeviceDriverType::DINPUT, 
    DeviceDriverType::SWITCH, 
    DeviceDriverType::WEBAPP,
    DeviceDriverType::PS3,
    DeviceDriverType::PSCLASSIC, 
    DeviceDriverType::XINPUT,
    #if defined(XREMOTE_ROM_AVAILABLE)
    DeviceDriverType::XBOXOG_XR,
    #endif

#endif
};

struct ComboMap { 
    uint32_t combo; 
    DeviceDriverType driver; 
};

static constexpr std::array<ComboMap, 9> BUTTON_COMBO_MAP = {{
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

const std::string UserSettings::DATETIME_KEY()
{
    return std::string("datetime");
}

DeviceDriverType UserSettings::DEFAULT_DRIVER()
{
    return VALID_DRIVER_TYPES[0];
}

//Checks if button combo has been held for 3 seconds, returns true if mode has been changed
bool UserSettings::check_for_driver_change(Gamepad& gamepad)
{
    Gamepad::PadIn gp_in = gamepad.get_pad_in();
    static uint32_t last_button_combo = BUTTON_COMBO(gp_in.buttons, gp_in.dpad);
    static uint8_t call_count = 0;

    uint32_t current_button_combo = BUTTON_COMBO(gp_in.buttons, gp_in.dpad);

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

    for (const auto& combo_map : BUTTON_COMBO_MAP)
    {
        if (combo_map.combo == current_button_combo && is_valid_driver(combo_map.driver))
        {
            new_driver = combo_map.driver;
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

//Disconnects usb and resets pico, call from core0
bool UserSettings::store_profile(uint8_t index, const UserProfile& profile)
{
    if (profile.id < 1 || profile.id > MAX_PROFILES)
    {
        return false;
    }
    if (index > MAX_GAMEPADS - 1)
    {
        index = 0;
    }

    board_api::usb::disconnect_all();

    nvs_tool_.write(ACTIVE_PROFILE_KEY(index), &profile.id, sizeof(uint8_t));
    nvs_tool_.write(PROFILE_KEY(profile.id), &profile, sizeof(UserProfile));

    board_api::reboot();

    return true;
}

//Disconnects usb and resets pico, call from core0
bool UserSettings::store_profile_and_driver_type(DeviceDriverType new_driver_type, uint8_t index, const UserProfile& profile)
{
    if (profile.id < 1 || profile.id > MAX_PROFILES)
    {
        return false;
    }
    if (index > MAX_GAMEPADS - 1)
    {
        index = 0;
    }

    bool valid_driver = false;
    for (const auto& driver : VALID_DRIVER_TYPES)
    {
        if (new_driver_type == driver)
        {
            valid_driver = true;
            break;
        }
    }
    if (!valid_driver)
    {
        new_driver_type = DEFAULT_DRIVER();
    }

    board_api::usb::disconnect_all();

    nvs_tool_.write(DRIVER_TYPE_KEY(), reinterpret_cast<const uint8_t*>(&new_driver_type), sizeof(new_driver_type));
    nvs_tool_.write(ACTIVE_PROFILE_KEY(index), &profile.id, sizeof(uint8_t));
    nvs_tool_.write(PROFILE_KEY(profile.id), &profile, sizeof(UserProfile));
    
    board_api::reboot();
    
    return true;
}

//Disconnects usb and resets pico if it's a new & valid mode, call from core0
void UserSettings::store_driver_type(DeviceDriverType new_driver) 
{
    if (!is_valid_driver(new_driver))
    {
        OGXM_LOG("Invalid driver type detected during store: " + OGXM_TO_STRING(new_driver) + "\n");
        return;
    }

    OGXM_LOG("Storing new driver type: " + OGXM_TO_STRING(new_driver) + "\n");

    board_api::usb::disconnect_all();

    nvs_tool_.write(DRIVER_TYPE_KEY(), &new_driver, sizeof(uint8_t));

    board_api::reboot();
}

uint8_t UserSettings::get_active_profile_id(const uint8_t index)
{
    if (index > MAX_GAMEPADS - 1)
    {
        OGXM_LOG("UserSettings::get_active_profile_id: Invalid index\n");
        return 0x01;
    }

    uint8_t read_profile_id = 0;
    nvs_tool_.read(ACTIVE_PROFILE_KEY(index), &read_profile_id, sizeof(uint8_t));

    if (read_profile_id < 1 || read_profile_id > MAX_PROFILES)
    {
        OGXM_LOG("UserSettings::get_active_profile_id: Invalid profile id\n");
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
    nvs_tool_.read(PROFILE_KEY(profile_id), &profile, sizeof(UserProfile));

    if (profile.id != profile_id)
    {
        OGXM_LOG("Profile read failed, returning default profile\n");
        return UserProfile();
    }
    return profile;
}

bool UserSettings::is_valid_driver(DeviceDriverType driver)
{
    for (const auto& valid_driver : VALID_DRIVER_TYPES)
    {
        if (driver == valid_driver)
        {
            return true;
        }
    }
    return false;
}

DeviceDriverType UserSettings::get_current_driver()
{
    if (current_driver_ != DeviceDriverType::NONE)
    {
        return current_driver_;
    }

    uint8_t stored_value = 0;
    nvs_tool_.read(DRIVER_TYPE_KEY(), &stored_value, sizeof(uint8_t));

    if (is_valid_driver(static_cast<DeviceDriverType>(stored_value)))
    {
        OGXM_LOG("Driver type read from flash: " + OGXM_TO_STRING(static_cast<DeviceDriverType>(stored_value)) + "\n");

        current_driver_ = static_cast<DeviceDriverType>(stored_value);
        return current_driver_;
    }

    OGXM_LOG("Invalid driver type read from flash, setting default driver\n");

    current_driver_ = DEFAULT_DRIVER();
    return current_driver_;
}

void UserSettings::write_datetime()
{
    nvs_tool_.write(DATETIME_KEY(), DATETIME_TAG.c_str(), DATETIME_TAG.size() + 1);
}

bool UserSettings::verify_datetime()
{
    char read_dt_tag[DATETIME_TAG.size() + 1] = {0};

    if (!nvs_tool_.read(DATETIME_KEY(), read_dt_tag, sizeof(read_dt_tag)) ||
        (std::strcmp(read_dt_tag, DATETIME_TAG.c_str()) != 0))
    {
        return false;
    }
    return true;
}

//Checks for first boot and initializes user profiles, call before tusb is inited.
void UserSettings::initialize_flash()
{
    OGXM_LOG("Initializing flash\n");

    uint8_t read_init_flag = 0;
    nvs_tool_.read(INIT_FLAG_KEY(), &read_init_flag, sizeof(uint8_t));

    if (read_init_flag == FLASH_INIT_FLAG)
    {
        OGXM_LOG("Flash already initialized: %i\n", read_init_flag);
        return;
    }

    OGXM_LOG("Flash not initialized, erasing\n");
    nvs_tool_.erase_all();

    OGXM_LOG("Writing default driver\n");

    uint8_t device_mode_buffer = static_cast<uint8_t>(DEFAULT_DRIVER());
    nvs_tool_.write(DRIVER_TYPE_KEY(), &device_mode_buffer, sizeof(uint8_t));

    OGXM_LOG("Writing default profile ids\n");

    for (uint8_t i = 0; i < MAX_GAMEPADS; i++)
    {
        uint8_t profile_id = i + 1;
        nvs_tool_.write(ACTIVE_PROFILE_KEY(i), &profile_id, sizeof(uint8_t));
    }

    OGXM_LOG("Writing default profiles\n");

    {
        UserProfile profile;
        OGXM_LOG("Profile size: %i\n", sizeof(UserProfile));

        for (uint8_t i = 0; i < MAX_PROFILES; i++)
        {
            profile.id = i + 1;
            nvs_tool_.write(PROFILE_KEY(profile.id), &profile, sizeof(UserProfile));
            OGXM_LOG("Profile " + std::to_string(profile.id) + " written\n");
        }
    }

    OGXM_LOG("Writing init flag\n");    

    uint8_t init_flag_buffer = FLASH_INIT_FLAG;
    nvs_tool_.write(INIT_FLAG_KEY(), &init_flag_buffer, sizeof(uint8_t));

    OGXM_LOG("Flash initialized\n");
}