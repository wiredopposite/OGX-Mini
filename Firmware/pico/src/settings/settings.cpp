#include <string>
#include <cstring>
#include "gamepad/gamepad.h"
#include "settings/nvs.h"
#include "settings/settings.h"

#define INIT_FLAG       ((uint8_t)0xA3)
#define PROFILES_MAX    8U

typedef struct {
    uint8_t     init_flag;
    usbd_type_t device_type;
    uint32_t    addons;
    uint8_t     active_profile_id[GAMEPADS_MAX];
} settings_t;

static settings_t settings = {0};

static inline const std::string SETTINGS_KEY() {
    return std::string("settings");
}

static inline const std::string PROFILE_KEY(uint8_t id) {
    return std::string("profile_") + std::to_string(id);
}

static inline const std::string DATETIME_KEY() {
    return std::string("datetime");
}

static inline const std::string DATETIME_STR() {
    return std::string(BUILD_DATETIME);
}

void settings_init(void) {
    if (settings.init_flag == INIT_FLAG) {
        return;
    }
    nvs_init();
    nvs_read(SETTINGS_KEY().c_str(), &settings, sizeof(settings));
    if (settings.init_flag == INIT_FLAG) {
        return;
    }
    settings.init_flag = INIT_FLAG;
    settings.device_type = USBD_TYPE_XBOXOG_GP;
    settings.addons = 0;
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        settings.active_profile_id[i] = 1;
    }
    nvs_erase_all();
    
    user_profile_t profile = {0};
    settings_get_default_profile(&profile); 

    for (uint8_t i = 0; i < PROFILES_MAX; i++) {
        profile.id = i + 1;
        nvs_write(PROFILE_KEY(i + 1).c_str(), &profile, sizeof(profile));
    }

    nvs_write(SETTINGS_KEY().c_str(), &settings, sizeof(settings));
}

usbd_type_t settings_get_device_type(void) {
    return settings.device_type;
}

bool settings_valid_datetime(void) {
    if (settings.init_flag != INIT_FLAG) {
        return false;
    }
    const std::string datetime = DATETIME_STR();
    std::string read_datetime;
    nvs_read(DATETIME_KEY().c_str(), read_datetime.data(), datetime.size());

    if (std::strncmp(datetime.c_str(), read_datetime.c_str(), datetime.size()) == 0) {
        return true;
    }
    return false;
}

void settings_write_datetime(void) {
    if (settings.init_flag != INIT_FLAG) {
        return;
    }
    nvs_write(DATETIME_KEY().c_str(), DATETIME_STR().c_str(), DATETIME_STR().size());
}

void settings_get_profile_by_index(uint8_t index, user_profile_t* profile) {
    if ((settings.init_flag != INIT_FLAG) || (index > GAMEPADS_MAX)) {
        printf("returning default profile\n");
        settings_get_default_profile(profile);
        return;
    }
    nvs_read(PROFILE_KEY(settings.active_profile_id[index]).c_str(), 
             profile, sizeof(user_profile_t));
}

void settings_get_profile_by_id(uint8_t id, user_profile_t* profile) {
    if ((settings.init_flag != INIT_FLAG) || (id > PROFILES_MAX)) {
        settings_get_default_profile(profile);
        return;
    }
    nvs_read(PROFILE_KEY(id).c_str(), profile, sizeof(user_profile_t));
}

uint8_t settings_get_active_profile_id(uint8_t index) {
    if ((settings.init_flag != INIT_FLAG) || (index > GAMEPADS_MAX)) {
        return 1;
    }
    return settings.active_profile_id[index];
}

void settings_store_device_type(usbd_type_t type) {
    if (settings.init_flag != INIT_FLAG) {
        return;
    }
    settings.device_type = type;
    nvs_write(SETTINGS_KEY().c_str(), &settings, sizeof(settings));
}

void settings_store_profile(uint8_t index, const user_profile_t* profile) {
    if ((settings.init_flag != INIT_FLAG) || (index > PROFILES_MAX)) {
        return;
    }
    nvs_write(PROFILE_KEY(settings.active_profile_id[index]).c_str(), 
              profile, sizeof(user_profile_t));
}

void settings_get_default_joystick(joystick_settings_t* joy_set) {
    joy_set->dz_inner = fix16_from_int(0);
    joy_set->dz_outer = fix16_from_int(1);
    joy_set->anti_dz_circle = fix16_from_int(0);
    joy_set->anti_dz_circle_y_scale = fix16_from_int(0);
    joy_set->anti_dz_square = fix16_from_int(0);
    joy_set->anti_dz_square_y_scale = fix16_from_int(0);
    joy_set->anti_dz_angular = fix16_from_int(0);
    joy_set->anti_dz_outer = fix16_from_int(1);
    joy_set->axis_restrict = fix16_from_int(0);
    joy_set->angle_restrict = fix16_from_int(0);
    joy_set->diag_scale_min = fix16_from_int(1);
    joy_set->diag_scale_max = fix16_from_int(1);
    joy_set->curve = fix16_from_int(1);
    joy_set->uncap_radius = 1;
    joy_set->invert_y = 0;
    joy_set->invert_x = 0;
}

void settings_get_default_trigger(trigger_settings_t* trig_set) {
    trig_set->dz_inner = fix16_from_int(0);
    trig_set->dz_outer = fix16_from_int(1);
    trig_set->anti_dz_inner = fix16_from_int(0);
    trig_set->anti_dz_outer = fix16_from_int(1);
    trig_set->curve = fix16_from_int(1);
}

void settings_get_default_profile(user_profile_t* profile) {
    if (profile == NULL) {
        return;
    }
    memset(profile, 0, sizeof(user_profile_t));
    settings_get_default_joystick(&profile->joystick_l);
    memcpy(&profile->joystick_r, &profile->joystick_l, sizeof(joystick_settings_t));
    settings_get_default_trigger(&profile->trigger_l);
    memcpy(&profile->trigger_r, &profile->trigger_l, sizeof(trigger_settings_t));

    for (uint8_t i = 0; i < GAMEPAD_BTN_BIT_COUNT; i++) {
        profile->btns[i] = i;
    }
    for (uint8_t i = 0; i < GAMEPAD_DPAD_BIT_COUNT; i++) {
        profile->dpad[i] = i;
    }
    for (uint8_t i = 0; i < GAMEPAD_ANALOG_COUNT; i++) {
        profile->analog[i] = i;
    }
    profile->analog_en  = 1;
    profile->id = 1;
}

bool settings_is_default_joystick(const joystick_settings_t* joy_set) {
    if (joy_set == NULL) {
        return false;
    }
    joystick_settings_t default_joy = {0};
    settings_get_default_joystick(&default_joy);
    return (memcmp(joy_set, &default_joy, sizeof(joystick_settings_t)) == 0);
}

bool settings_is_default_trigger(const trigger_settings_t* trig_set) {
    if (trig_set == NULL) {
        return false;
    }
    trigger_settings_t default_trig = {0};
    settings_get_default_trigger(&default_trig);
    return (memcmp(trig_set, &default_trig, sizeof(trigger_settings_t)) == 0);
}