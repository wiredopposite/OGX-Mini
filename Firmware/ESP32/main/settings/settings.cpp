#include <string>
#include <cstring>
#include "log/log.h"
#include "gamepad/gamepad.h"
#include "settings/nvs.h"
#include "settings/settings.h"

#define INIT_FLAG           ((uint16_t)0xFFA3)

typedef struct __attribute__((aligned(4))) {
    volatile uint16_t    init_flag;
    volatile usbd_type_t device_type;
    volatile uint32_t    addons;
    volatile uint8_t     active_profile_id[GAMEPADS_MAX];
} settings_t;

static settings_t settings = {};

static inline const std::string SETTINGS_KEY() {
    return std::string("settings");
}

static inline const std::string PROFILE_KEY(uint8_t id) {
    return std::string("profile_") + std::to_string(id);
}

void settings_init(void) {
    if (settings.init_flag == INIT_FLAG) {
        ogxm_logd("Settings already initialized\n");
        return;
    }
    ogxm_logd("Initializing settings\n");

    nvs_init();
    nvs_read(SETTINGS_KEY().c_str(), &settings, sizeof(settings));
    if (settings.init_flag == INIT_FLAG) {
        ogxm_logd("Settings already initialized with flag: %04X\n", settings.init_flag);
        return;
    }

    ogxm_logd("Settings not initialized, setting default values\n");

    settings.init_flag = INIT_FLAG;
    settings.device_type = USBD_TYPE_XBOXOG_GP;
    settings.addons = 0;
    for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
        settings.active_profile_id[i] = 1;
    }
    nvs_erase();
    
    user_profile_t profile = {};
    settings_get_default_profile(&profile); 

    for (uint8_t i = 0; i < USER_PROFILES_MAX; i++) {
        profile.id = i + 1;
        nvs_write(PROFILE_KEY(i + 1).c_str(), &profile, sizeof(profile));
    }

    nvs_write(SETTINGS_KEY().c_str(), &settings, sizeof(settings));

    ogxm_logd("Settings initialized successfully\n");
}

usbd_type_t settings_get_device_type(void) {
    return settings.device_type;
}

void settings_get_profile_by_index(uint8_t index, user_profile_t* profile) {
    if ((settings.init_flag != INIT_FLAG) || (index > GAMEPADS_MAX)) {
        ogxm_logd("returning default profile\n");
        settings_get_default_profile(profile);
        return;
    }
    ogxm_logd("Getting profile by index: %d\n", index);
    nvs_read(PROFILE_KEY(settings.active_profile_id[index]).c_str(), 
             profile, sizeof(user_profile_t));
}

void settings_get_profile_by_id(uint8_t id, user_profile_t* profile) {
    if ((settings.init_flag != INIT_FLAG) || 
        (id > USER_PROFILES_MAX) || (id == 0)) {
        settings_get_default_profile(profile);
        return;
    }
    ogxm_logd("Getting profile by ID: %d\n", id);
    nvs_read(PROFILE_KEY(id).c_str(), profile, sizeof(user_profile_t));
}

uint8_t settings_get_active_profile_id(uint8_t index) {
    if ((settings.init_flag != INIT_FLAG) || (index > GAMEPADS_MAX)) {
        return 1;
    }
    return settings.active_profile_id[index];
}

void settings_store_device_type(usbd_type_t type) {
    if ((settings.init_flag != INIT_FLAG) ||
        (type >= USBD_TYPE_COUNT)) {
        return;
    }
    settings.device_type = type;
    ogxm_logd("Storing device type: %d\n", type);
    nvs_write(SETTINGS_KEY().c_str(), &settings, sizeof(settings));
}

void settings_store_profile(uint8_t index, const user_profile_t* profile) {
    if ((settings.init_flag != INIT_FLAG) || (index > USER_PROFILES_MAX)) {
        return;
    }
    ogxm_logd("Storing profile, index: %d", index);
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
    joystick_settings_t default_joy = {};
    settings_get_default_joystick(&default_joy);
    return (memcmp(joy_set, &default_joy, sizeof(joystick_settings_t)) == 0);
}

bool settings_is_default_trigger(const trigger_settings_t* trig_set) {
    if (trig_set == NULL) {
        return false;
    }
    trigger_settings_t default_trig = {};
    settings_get_default_trigger(&default_trig);
    return (memcmp(trig_set, &default_trig, sizeof(trigger_settings_t)) == 0);
}