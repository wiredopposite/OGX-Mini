#include <cstring>
#include <array>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/flash.h>
#include <hardware/regs/addressmap.h>

#include "tusb.h"

#include "Board/board_api.h"
#include "UserSettings/UserSettings.h"

static constexpr uint32_t button_combo(const uint16_t& buttons, const uint8_t& dpad = 0)
{
    return (static_cast<uint32_t>(buttons) << 16) | static_cast<uint32_t>(dpad);
}

namespace ButtonCombo 
{
    static constexpr uint32_t PS3       = button_combo(Gamepad::Button::START, Gamepad::DPad::LEFT);
    static constexpr uint32_t DINPUT    = button_combo(Gamepad::Button::START | Gamepad::Button::RB, Gamepad::DPad::LEFT);
    static constexpr uint32_t XINPUT    = button_combo(Gamepad::Button::START, Gamepad::DPad::UP);
    static constexpr uint32_t SWITCH    = button_combo(Gamepad::Button::START, Gamepad::DPad::DOWN);
    static constexpr uint32_t XBOXOG    = button_combo(Gamepad::Button::START, Gamepad::DPad::RIGHT);
    static constexpr uint32_t XBOXOG_SB = button_combo(Gamepad::Button::START | Gamepad::Button::RB, Gamepad::DPad::RIGHT);
    static constexpr uint32_t XBOXOG_XR = button_combo(Gamepad::Button::START | Gamepad::Button::LB, Gamepad::DPad::RIGHT);
    static constexpr uint32_t PSCLASSIC = button_combo(Gamepad::Button::START | Gamepad::Button::A);
    static constexpr uint32_t WEBAPP    = button_combo(Gamepad::Button::START | Gamepad::Button::LB | Gamepad::Button::RB);
};

static constexpr DeviceDriver::Type VALID_DRIVER_TYPES[] =
{
#if defined(CONFIG_EN_4CH)
    DeviceDriver::Type::XBOXOG, 
    DeviceDriver::Type::XBOXOG_SB, 
    DeviceDriver::Type::XBOXOG_XR,
    DeviceDriver::Type::WEBAPP,
    
#elif MAX_GAMEPADS > 1
    DeviceDriver::Type::DINPUT, 
    DeviceDriver::Type::SWITCH, 
    DeviceDriver::Type::WEBAPP,

#else
    DeviceDriver::Type::XBOXOG, 
    DeviceDriver::Type::XBOXOG_SB, 
    DeviceDriver::Type::XBOXOG_XR,
    // DeviceDriver::Type::DINPUT, 
    DeviceDriver::Type::SWITCH, 
    DeviceDriver::Type::WEBAPP,
    DeviceDriver::Type::PS3,
    DeviceDriver::Type::PSCLASSIC, 
    DeviceDriver::Type::XINPUT,

#endif
};

struct ComboMap { uint32_t combo; DeviceDriver::Type driver; };
static constexpr std::array<ComboMap, 9> BUTTON_COMBO_MAP = 
{{
    { ButtonCombo::XBOXOG, DeviceDriver::Type::XBOXOG },
    { ButtonCombo::XBOXOG_SB, DeviceDriver::Type::XBOXOG_SB },
    { ButtonCombo::XBOXOG_XR, DeviceDriver::Type::XBOXOG_XR },
    { ButtonCombo::WEBAPP, DeviceDriver::Type::WEBAPP },
    { ButtonCombo::DINPUT, DeviceDriver::Type::DINPUT },
    { ButtonCombo::SWITCH, DeviceDriver::Type::SWITCH },
    { ButtonCombo::XINPUT, DeviceDriver::Type::XINPUT },
    { ButtonCombo::PS3, DeviceDriver::Type::PS3 },
    { ButtonCombo::PSCLASSIC, DeviceDriver::Type::PSCLASSIC }
}};

mutex_t UserSettings::flash_mutex_;
DeviceDriver::Type UserSettings::current_driver_ = DeviceDriver::Type::NONE;

//Checks for first boot and initializes user profiles, call before tusb is inited.
void UserSettings::initialize_flash()
{
    if (!mutex_is_initialized(&flash_mutex_))
    {
        mutex_init(&flash_mutex_);
    }

    mutex_enter_blocking(&flash_mutex_);

    const uint8_t* read_init_flag = reinterpret_cast<const uint8_t*>(XIP_BASE + flash_offset(INIT_FLAG_SECTOR));

    if (*read_init_flag == INIT_FLAG)
    {
        mutex_exit(&flash_mutex_);
        return;
    }

    uint32_t saved_interrupts = save_and_disable_interrupts();

    // {
    //     std::array<uint8_t, FLASH_PAGE_SIZE> device_mode_buffer;
    //     device_mode_buffer.fill(0xFF);

    //     // device_mode_buffer[0] = static_cast<uint8_t>(DeviceDriver::Type::XBOXOG);

    //     flash_range_erase(flash_offset(DEVICE_MODE_SECTOR), FLASH_SECTOR_SIZE);
    //     flash_range_program(flash_offset(DEVICE_MODE_SECTOR), device_mode_buffer.data(), FLASH_PAGE_SIZE);
    // }
    
    {
        std::array<uint8_t, FLASH_PAGE_SIZE> profile_ids;
        profile_ids.fill(0xFF);
        std::memset(profile_ids.data(), 0x01, 4);

        flash_range_erase(flash_offset(ACTIVE_PROFILES_SECTOR), FLASH_SECTOR_SIZE);
        flash_range_program(flash_offset(ACTIVE_PROFILES_SECTOR), profile_ids.data(), FLASH_PAGE_SIZE);
    }

    {    
        std::array<uint8_t, FLASH_PAGE_SIZE> profile_buffer;
        profile_buffer.fill(0);

        flash_range_erase(flash_offset(PROFILES_START_SECTOR), FLASH_SECTOR_SIZE);

        for (uint8_t i = 0; i < MAX_PROFILES; i++)
        {
            UserProfile profile = UserProfile();
            profile.id = i + 1;
            std::memcpy(profile_buffer.data(), reinterpret_cast<const uint8_t*>(&profile), sizeof(UserProfile));
            flash_range_program(flash_offset(PROFILES_START_SECTOR) + profile_offset(profile.id), profile_buffer.data(), FLASH_PAGE_SIZE);
        }
    }

    {
        std::array<uint8_t, FLASH_PAGE_SIZE> init_flag_buffer;
        init_flag_buffer.fill(INIT_FLAG);

        flash_range_erase(flash_offset(INIT_FLAG_SECTOR), FLASH_SECTOR_SIZE);
        flash_range_program(flash_offset(INIT_FLAG_SECTOR), init_flag_buffer.data(), FLASH_PAGE_SIZE);
    }

    restore_interrupts(saved_interrupts);
    mutex_exit(&flash_mutex_);
}

bool UserSettings::verify_firmware_version()
{
    mutex_enter_blocking(&flash_mutex_);

    const char* flash_firmware_version = reinterpret_cast<const char*>(XIP_BASE + flash_offset(FIRMWARE_VER_FLAG_SECTOR));
    bool match = std::memcmp(flash_firmware_version, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION)) == 0;

    mutex_exit(&flash_mutex_);
    return match;
}

bool UserSettings::write_firmware_version_safe()
{
    mutex_enter_blocking(&flash_mutex_);
    uint32_t saved_interrupts = save_and_disable_interrupts();

    flash_range_erase(flash_offset(FIRMWARE_VER_FLAG_SECTOR), FLASH_SECTOR_SIZE);
    flash_range_program(flash_offset(FIRMWARE_VER_FLAG_SECTOR), reinterpret_cast<const uint8_t*>(FIRMWARE_VERSION), sizeof(FIRMWARE_VERSION));

    restore_interrupts(saved_interrupts);
    mutex_exit(&flash_mutex_);

    return verify_firmware_version();
}

DeviceDriver::Type UserSettings::get_current_driver()
{
    // return DeviceDriver::Type::XINPUT;
    
    if (current_driver_ != DeviceDriver::Type::NONE)
    {
        return current_driver_;
    }

    mutex_enter_blocking(&flash_mutex_);

    int stored_value = *reinterpret_cast<const int*>(XIP_BASE + flash_offset(DRIVER_TYPE_SECTOR));

    mutex_exit(&flash_mutex_);

    for (const auto& driver : VALID_DRIVER_TYPES)
    {
        if (stored_value == static_cast<int>(driver))
        {
            current_driver_ = driver;
            return current_driver_;
        }
    }

    current_driver_ = get_default_driver();

    return current_driver_;
}

void UserSettings::store_driver_type_unsafe(const DeviceDriver::Type new_mode)
{
    std::array<int, FLASH_PAGE_SIZE / sizeof(int)> mode_buffer;
    mode_buffer.fill(static_cast<int>(new_mode));

    mutex_enter_blocking(&flash_mutex_);
    uint32_t saved_interrupts = save_and_disable_interrupts();
    
    flash_range_erase(flash_offset(DRIVER_TYPE_SECTOR), FLASH_SECTOR_SIZE);
    flash_range_program(flash_offset(DRIVER_TYPE_SECTOR), reinterpret_cast<uint8_t*>(mode_buffer.data()), FLASH_PAGE_SIZE);

    restore_interrupts(saved_interrupts); 
    mutex_exit(&flash_mutex_);   
}

//Disconnects usb and resets pico, thread safe
void UserSettings::store_driver_type_safe(DeviceDriver::Type new_mode) 
{
    if (tud_connected())
    {
        tud_disconnect();
        sleep_ms(300);
    }

    multicore_reset_core1();

    store_driver_type_unsafe(new_mode);

    board_api::reboot();
}

//Checks if button combo has been held for 3 seconds, returns true if mode has been changed
bool UserSettings::check_for_driver_change(Gamepad& gamepad)
{
    static uint32_t last_button_combo = button_combo(gamepad.get_buttons(), gamepad.get_dpad_buttons());
    static uint8_t call_count = 0;

    uint32_t current_button_combo = button_combo(gamepad.get_buttons(), gamepad.get_dpad_buttons());

    if (!(current_button_combo & (static_cast<uint32_t>(Gamepad::Button::START) << 16)) || 
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

    DeviceDriver::Type new_driver = DeviceDriver::Type::NONE;

    for (const auto& combo : BUTTON_COMBO_MAP)
    {
        if (combo.combo == current_button_combo)
        {
            for (const auto& driver : VALID_DRIVER_TYPES)
            {
                if (combo.driver == driver)
                {
                    new_driver = combo.driver;
                    break;
                }
            }
        }
    }

    if (new_driver == DeviceDriver::Type::NONE || new_driver == current_driver_)
    {
        return false;
    }

    current_driver_ = new_driver;

    return true;
}

void UserSettings::store_profile_unsafe(const UserProfile& profile)
{
    mutex_enter_blocking(&flash_mutex_);

    uint32_t saved_interrupts = save_and_disable_interrupts();
    
    std::array<uint8_t, FLASH_PAGE_SIZE * MAX_PROFILES> profiles_buffer;

    std::memcpy(profiles_buffer.data(), reinterpret_cast<const uint8_t*>(XIP_BASE + flash_offset(PROFILES_START_SECTOR)), FLASH_PAGE_SIZE * MAX_PROFILES);
    std::memcpy(profiles_buffer.data() + profile_offset(profile.id), reinterpret_cast<const uint8_t*>(&profile), sizeof(UserProfile));

    flash_range_erase(flash_offset(PROFILES_START_SECTOR), FLASH_SECTOR_SIZE);

    for (uint8_t i = 0; i < MAX_PROFILES; i++)
    {
        flash_range_program(flash_offset(PROFILES_START_SECTOR) + (i * FLASH_PAGE_SIZE), profiles_buffer.data() + (i * FLASH_PAGE_SIZE), FLASH_PAGE_SIZE);
    }

    restore_interrupts(saved_interrupts);
    mutex_exit(&flash_mutex_);
}

void UserSettings::store_active_profile_id_unsafe(const uint8_t index, const uint8_t profile_id)
{
    mutex_enter_blocking(&flash_mutex_);

    std::array<uint8_t, FLASH_PAGE_SIZE> read_profile_ids;
    read_profile_ids.fill(0xFF);

    std::memcpy(read_profile_ids.data(), reinterpret_cast<const uint8_t*>(XIP_BASE + flash_offset(ACTIVE_PROFILES_SECTOR)), 4);

    read_profile_ids[index] = profile_id;

    uint32_t saved_interrupts = save_and_disable_interrupts();
    
    flash_range_erase(flash_offset(ACTIVE_PROFILES_SECTOR), FLASH_SECTOR_SIZE);
    flash_range_program(flash_offset(ACTIVE_PROFILES_SECTOR), read_profile_ids.data(), FLASH_PAGE_SIZE);

    restore_interrupts(saved_interrupts);
    mutex_exit(&flash_mutex_);
}

//Disconnects usb and resets pico, thread safe
bool UserSettings::store_profile_safe(uint8_t index, const UserProfile& profile)
{
    if (profile.id < 1 || profile.id > MAX_PROFILES)
    {
        return false;
    }
    if (index > MAX_GAMEPADS - 1)
    {
        index = 0;
    }

    if (tud_connected())
    {
        tud_disconnect();
        sleep_ms(300);
    }

    multicore_reset_core1();

    store_active_profile_id_unsafe(index, profile.id);
    store_profile_unsafe(profile);

    board_api::reboot();
    return true;
}

//Disconnects usb and resets pico, thread safe
bool UserSettings::store_profile_and_driver_type_safe(DeviceDriver::Type new_driver_type, uint8_t index, const UserProfile& profile)
{
    if (profile.id < 1 || profile.id > MAX_PROFILES)
    {
        return false;
    }
    if (index > MAX_GAMEPADS - 1)
    {
        index = 0;
    }

    bool found = false;
    for (const auto& driver : VALID_DRIVER_TYPES)
    {
        if (new_driver_type == driver)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        new_driver_type = get_default_driver();
    }

    if (tud_connected())
    {
        tud_disconnect();
        sleep_ms(300);
    }

    multicore_reset_core1();

    store_driver_type_unsafe(new_driver_type);
    store_active_profile_id_unsafe(index, profile.id);
    store_profile_unsafe(profile);

    board_api::reboot();
    
    return true;
}

uint8_t UserSettings::get_active_profile_id(const uint8_t index)
{
    mutex_enter_blocking(&flash_mutex_);

    const uint8_t* base_address = (const uint8_t*)(XIP_BASE + flash_offset(ACTIVE_PROFILES_SECTOR));
    uint8_t read_profile_id = base_address[index];

    mutex_exit(&flash_mutex_);

    if (read_profile_id < 1 || read_profile_id > MAX_PROFILES)
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
    mutex_enter_blocking(&flash_mutex_);

    const uint8_t* base_address = reinterpret_cast<const uint8_t*>(XIP_BASE + flash_offset(PROFILES_START_SECTOR) + profile_offset(profile_id));

    UserProfile profile;
    std::memcpy(&profile, base_address, sizeof(UserProfile));

    mutex_exit(&flash_mutex_);

    if (profile.id != profile_id)
    {
        return UserProfile();
    }
    return profile;
}

DeviceDriver::Type UserSettings::get_default_driver()
{
    return VALID_DRIVER_TYPES[0];
}