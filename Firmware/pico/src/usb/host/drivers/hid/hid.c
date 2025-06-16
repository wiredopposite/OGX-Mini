#include <string.h>
#include <pico/stdlib.h>
#include "log/log.h"
#include "gamepad/range.h"
#include "settings/settings.h"
#include "usb/host/host_private.h"
#include "usb/host/tusb_host/tuh_hxx.h"
#include "usb/host/drivers/hid/hid.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

typedef struct {
    hid_field_t             fields[32];
    uint8_t                 field_count;
} hid_report_map_t;

typedef struct __attribute__((aligned(4))) {
    bool                    inited;
    hid_report_map_t        report_map;
    const hidh_driver_t*    driver;
    user_profile_t          profile;
    gamepad_pad_t           gp_report;
    gamepad_pad_t           prev_gp_report;
    uint8_t                 buffer_out[HIDH_BUFFER_OUT_SIZE] __attribute__((aligned(4)));
    struct {
        bool joy_l;
        bool joy_r;
        bool trig_l;
        bool trig_r;
    } map;
} hid_state_t;

static const hidh_driver_t* HIDH_DRIVERS[USBH_TYPE_COUNT] = {
    [USBH_TYPE_NONE]            = NULL,
    [USBH_TYPE_XID]             = NULL,
    [USBH_TYPE_XBLC]            = NULL,
    [USBH_TYPE_XGIP]            = NULL,
    [USBH_TYPE_XGIP_AUDIO]      = NULL,
    [USBH_TYPE_XGIP_CHATPAD]    = NULL,
    [USBH_TYPE_XINPUT]          = NULL,
    [USBH_TYPE_XINPUT_AUDIO]    = NULL,
    [USBH_TYPE_XINPUT_CHATPAD]  = NULL,
    [USBH_TYPE_XINPUT_WL]       = NULL,
    [USBH_TYPE_XINPUT_WL_AUDIO] = NULL,
    [USBH_TYPE_HID_GENERIC]     = &HIDH_DRIVER_GENERIC,
    [USBH_TYPE_HID_SWITCH_PRO]  = NULL,
    [USBH_TYPE_HID_SWITCH]      = &HIDH_DRIVER_SWITCH,
    [USBH_TYPE_HID_PSCLASSIC]   = &HIDH_DRIVER_PSCLASSIC,
    [USBH_TYPE_HID_DINPUT]      = &HIDH_DRIVER_DINPUT,
    [USBH_TYPE_HID_PS3]         = NULL,
    [USBH_TYPE_HID_PS4]         = NULL,
    [USBH_TYPE_HID_PS5]         = NULL,
    [USBH_TYPE_HID_N64]         = &HIDH_DRIVER_N64,
};

static hid_state_t hid_state[GAMEPADS_MAX] = {0};
static const uint16_t state_size = sizeof(hid_state);

static void hid_map_init(hid_report_map_t* map, const uint8_t* desc_report, uint16_t desc_len);
static void hid_map_gamepad(hid_state_t* state, const uint8_t* report, uint16_t report_len);

void hid_mounted_cb(usbh_type_t type, uint8_t index, uint8_t daddr, uint8_t itf_num, 
                    const uint8_t* desc_report, uint16_t desc_len, uint8_t* state_buffer) {
    if ((desc_report == NULL) || (desc_len == 0)) {
        ogxm_loge("HID report descriptor is NULL or empty\n");
        return;
    }
    hid_state_t* state = &hid_state[index];
    memset(state, 0, sizeof(hid_state_t));

    settings_get_profile_by_index(index, &state->profile);
    state->map.joy_l = !settings_is_default_joystick(&state->profile.joystick_l);
    state->map.joy_r = !settings_is_default_joystick(&state->profile.joystick_r);
    state->map.trig_l = !settings_is_default_trigger(&state->profile.trigger_l);
    state->map.trig_r = !settings_is_default_trigger(&state->profile.trigger_r);

    state->driver = HIDH_DRIVERS[type];
    if (state->driver == NULL) {
        ogxm_logd("Driver is null, using generic HID driver\n");
        state->driver = &HIDH_DRIVER_GENERIC;
    }
    ogxm_logd("HID host driver: %s\n", state->driver->name);
    if (state->driver->init_cb) {
        state->driver->init_cb(daddr, itf_num, state->buffer_out);
    }
    if (state->driver->set_led_cb) {
        state->driver->set_led_cb(index + 1, daddr, itf_num, state->buffer_out);
    }
    hid_map_init(&state->report_map, desc_report, desc_len);
    usb_host_driver_connect_cb(index, type, true);
    tuh_hxx_receive_report(daddr, itf_num);
}

// void hid_unmounted_cb(uint8_t index, uint8_t daddr, uint8_t itf_num) {

// }

void hid_report_cb(uint8_t index, usbh_periph_t subtype, uint8_t daddr, 
                   uint8_t itf_num, const uint8_t* data, uint16_t len) {
    hid_state_t* state = &hid_state[index];
    hid_map_gamepad(state, data, len);

    if (state->map.joy_l) {
        settings_scale_joysticks(&state->profile.joystick_l, 
                                  &state->gp_report.joystick_lx, 
                                  &state->gp_report.joystick_ly);
    }
    if (state->map.joy_r) {
        settings_scale_joysticks(&state->profile.joystick_r, 
                                  &state->gp_report.joystick_rx, 
                                  &state->gp_report.joystick_ry);
    }
    if (state->map.trig_l) {
        settings_scale_trigger(&state->profile.trigger_l, 
                               &state->gp_report.trigger_l);
    }
    if (state->map.trig_r) {
        settings_scale_trigger(&state->profile.trigger_r, 
                               &state->gp_report.trigger_r);
    }

    if (memcmp(&state->gp_report, &state->prev_gp_report, sizeof(gamepad_pad_t)) != 0) {
        usb_host_driver_pad_cb(index, &state->gp_report, 0);
        memcpy(&state->prev_gp_report, &state->gp_report, sizeof(gamepad_pad_t));
    }
    tuh_hxx_receive_report(daddr, itf_num);
}

void hid_send_rumble(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_rumble_t* rumble) {
    hid_state_t* hid = &hid_state[index];
    if (hid->driver->set_rumble_cb != NULL) {
        hid->driver->set_rumble_cb(daddr, itf_num, rumble, hid->buffer_out);
    }
}

void hid_send_audio(uint8_t index, uint8_t daddr, uint8_t itf_num, const gamepad_pcm_out_t* pcm) {
    (void)index;
    (void)daddr;
    (void)itf_num;
    (void)pcm;
}

const usb_host_driver_t USBH_DRIVER_HID = {
    .name = "HID",
    .mounted_cb = hid_mounted_cb,
    .unmounted_cb = NULL,
    .task_cb = NULL,
    .report_cb = hid_report_cb,
    .report_ctrl_cb = NULL,
    .send_rumble = hid_send_rumble,
    .send_audio = NULL,
};

static inline uint32_t hid_extract_field_value(const uint8_t* report, uint16_t bit_offset, uint8_t bit_size) {
    uint32_t result = 0;
    uint16_t byte_index = bit_offset / 8;
    uint8_t bit_index = bit_offset % 8;
    // Handle bit field that could span multiple bytes
    for (uint8_t i = 0; i < bit_size; i++) {
        uint16_t byte_idx = (bit_offset + i) / 8;
        uint8_t bit_idx = (bit_offset + i) % 8;
        
        if (report[byte_idx] & (1 << bit_idx)) {
            result |= (1 << i);
        }
    }
    return result;
}

static void hid_map_gamepad(hid_state_t* state, const uint8_t* report, uint16_t report_len) {
    gamepad_pad_t* pad = &state->gp_report;
    const hid_report_map_t* report_map = &state->report_map;
    const hid_usage_map_t* usage_map = state->driver->usage_map;
    const user_profile_t* profile = &state->profile;
    memset(pad, 0, sizeof(gamepad_pad_t));

    for (uint8_t i = 0; i < report_map->field_count; i++) {
        const hid_field_t* field = &report_map->fields[i];
        if ((field->bit_offset + field->bit_size) > (report_len * 8)) {
            continue;
        }
        uint32_t value = hid_extract_field_value(report, field->bit_offset, field->bit_size);

        switch (field->usage_page) {
        case HID__USAGE_PAGE_GENERIC_DESKTOP:
            if (state->driver->desktop_quirk_cb != NULL) {
                if (state->driver->desktop_quirk_cb(field->usage, value, pad, profile)) {
                    break;
                }
            }
            switch (field->usage) {
            case HID_DESKTOP_USAGE_X:
            case HID_DESKTOP_USAGE_Y:
            case HID_DESKTOP_USAGE_Z:
            case HID_DESKTOP_USAGE_RX:
            case HID_DESKTOP_USAGE_RY:
            case HID_DESKTOP_USAGE_RZ:
                // ogxm_logv("HID Usage Desktop: %u, value: %u\n", field->usage, value);
                // ogxm_logv("HID Usage Desktop: %u, logical min: %d, logical max: %d\n", 
                //        field->usage, field->logical_min, field->logical_max);
                for (uint8_t j = 0; j < ARRAY_SIZE(usage_map->desktop_map); j++) {
                    if (field->usage != usage_map->desktop_map[j].usage) {
                        continue;
                    }
                    switch (usage_map->desktop_map[j].type) {
                    case GP_TYPE_UINT8:
                        {
                        uint8_t* gp_value = (uint8_t*)((uint8_t*)pad + usage_map->desktop_map[j].offset);
                        gp_value[0] = range_free_scale_uint8(value, field->logical_min, field->logical_max);
                        }
                        break;
                    case GP_TYPE_INT16:
                        {
                        int16_t* gp_value = (int16_t*)((uint8_t*)pad + usage_map->desktop_map[j].offset);
                        gp_value[0] = range_free_scale_int16(value, field->logical_min, field->logical_max);
                        if (usage_map->desktop_map[j].invert) {
                            gp_value[0] = range_invert_int16(gp_value[0]);
                        }
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                }
                break;
            case HID_DESKTOP_USAGE_HAT:
                switch (value) {
                case HID_HAT_UP:
                    pad->dpad = GP_BIT8(profile->d_up);
                    break;
                case HID_HAT_UP_RIGHT:
                    pad->dpad = GP_BIT8(profile->d_up) | GP_BIT8(profile->d_right);
                    break;
                case HID_HAT_RIGHT:
                    pad->dpad = GP_BIT8(profile->d_right);
                    break;
                case HID_HAT_DOWN_RIGHT:
                    pad->dpad = GP_BIT8(profile->d_down) | GP_BIT8(profile->d_right);
                    break;
                case HID_HAT_DOWN:
                    pad->dpad = GP_BIT8(profile->d_down);
                    break;
                case HID_HAT_DOWN_LEFT:
                    pad->dpad = GP_BIT8(profile->d_down) | GP_BIT8(profile->d_left);
                    break;
                case HID_HAT_LEFT:
                    pad->dpad = GP_BIT8(profile->d_left);
                    break;
                case HID_HAT_UP_LEFT:
                    pad->dpad = GP_BIT8(profile->d_up) | GP_BIT8(profile->d_left);
                    break;
                default:
                    pad->dpad = 0;
                    break;
                }
                break;
            case HID_DESKTOP_USAGE_DPAD_UP:
                pad->dpad |= (value ? GP_BIT8(profile->d_up) : 0);
                break;
            case HID_DESKTOP_USAGE_DPAD_DOWN:
                pad->dpad |= (value ? GP_BIT8(profile->d_down) : 0);
                break;   
            case HID_DESKTOP_USAGE_DPAD_RIGHT:
                pad->dpad |= (value ? GP_BIT8(profile->d_right) : 0);
                break;
            case HID_DESKTOP_USAGE_DPAD_LEFT:
                pad->dpad |= (value ? GP_BIT8(profile->d_left) : 0);
                break;
            }
            break;
        case HID__USAGE_PAGE_VENDOR:
            if (state->driver->vendor_quirk_cb != NULL) {
                state->driver->vendor_quirk_cb(field->usage, value, pad, profile);
            }
            break;
        case HID__USAGE_PAGE_BUTTON:
            if (state->driver->button_quirk_cb != NULL) {
                if (state->driver->button_quirk_cb(field->usage, value, pad, profile)) {
                    break;
                }
            }
            if (!value || (field->usage >= HID_USAGE_BUTTON_COUNT)) {
                break;
            }
            // ogxm_logv("HID Usage Button: %u\n", field->usage);
            pad->buttons |= GP_BIT16(profile->btns[usage_map->buttons[field->usage]]);
            break;
        }
    }
    if ((pad->buttons & GAMEPAD_BTN_LT) && !pad->trigger_l) {
        pad->trigger_l = R_UINT8_MAX;
    }
    if ((pad->buttons & GAMEPAD_BTN_RT) && !pad->trigger_r) {
        pad->trigger_r = R_UINT8_MAX;
    }
}

static void hid_map_init(hid_report_map_t* map, const uint8_t* desc_report, uint16_t desc_len) {
    uint32_t usage_page = 0;
    int32_t logical_min = 0;
    int32_t logical_max = 0;
    uint8_t report_size = 0;
    uint8_t report_count = 0;
    uint16_t bit_offset = 0;

    uint32_t usages[32] = {0};
    uint8_t usage_count = 0;
    uint32_t usage_min = 0, usage_max = 0;
    bool have_usage_min = false, have_usage_max = false;

    map->field_count = 0;

    for (uint16_t i = 0; i < desc_len;) {
        uint8_t prefix = desc_report[i++];
        if (prefix == 0xFE) { // Long item, skip
            if (i + 1 < desc_len) {
                uint8_t data_size = desc_report[i++];
                i += data_size + 1; // skip long item tag
            }
            continue;
        }

        uint8_t bSize = prefix & 0x03;
        uint8_t bType = (prefix >> 2) & 0x03;
        uint8_t bTag  = (prefix >> 4) & 0x0F;
        bSize = (bSize == 3) ? 4 : bSize;

        uint32_t data = 0;
        for (uint8_t j = 0; j < bSize && i < desc_len; j++) {
            data |= ((uint32_t)desc_report[i++]) << (8 * j);
        }

        switch (bType) {
        case HID_ITEM_GLOBAL:
            switch (bTag) {
            case HID_GLOBAL_USAGE_PAGE:
                usage_page = data;
                break;
            case HID_GLOBAL_LOG_MIN:
                // Sign-extend based on size
                if (bSize == 1) logical_min = (int32_t)(int8_t)data;
                else if (bSize == 2) logical_min = (int32_t)(int16_t)data;
                else logical_min = (int32_t)data;
                break;
            case HID_GLOBAL_LOG_MAX:
                if (bSize == 1) logical_max = (int32_t)(int8_t)data;
                else if (bSize == 2) logical_max = (int32_t)(int16_t)data;
                else logical_max = (int32_t)data;
                break;
            case HID_GLOBAL_REPORT_SIZE:
                report_size = data;
                break;
            case HID_GLOBAL_REPORT_COUNT:
                report_count = data;
                break;
            }
            break;

        case HID_ITEM_LOCAL:
            switch (bTag) {
            case HID_LOCAL_USAGE:
                if (usage_count < 32) usages[usage_count++] = data;
                break;
            case HID_LOCAL_USAGE_MIN:
                usage_min = data;
                have_usage_min = true;
                break;
            case HID_LOCAL_USAGE_MAX:
                usage_max = data;
                have_usage_max = true;
                break;
            }
            break;

        case HID_ITEM_MAIN:
            if (bTag == HID_MAIN_INPUT && report_count > 0 && report_size > 0) {
                // Skip constant fields
                if (data & 0x01) {
                    bit_offset += report_count * report_size;
                    break;
                }

                // Usage range
                if (have_usage_min && have_usage_max && usage_min <= usage_max) {
                    for (uint32_t u = usage_min; u <= usage_max && map->field_count < 32; u++) {
                        hid_field_t* field = &map->fields[map->field_count++];
                        field->bit_offset = bit_offset;
                        field->bit_size = report_size;
                        field->logical_min = logical_min;
                        field->logical_max = logical_max;
                        field->usage_page = usage_page;
                        field->usage = u;
                        field->button_index = 0;
                        bit_offset += report_size;
                    }
                }
                // Multiple usages
                else if (usage_count > 0) {
                    for (uint8_t u = 0; u < usage_count && map->field_count < 32; u++) {
                        hid_field_t* field = &map->fields[map->field_count++];
                        field->bit_offset = bit_offset;
                        field->bit_size = report_size;
                        field->logical_min = logical_min;
                        field->logical_max = logical_max;
                        field->usage_page = usage_page;
                        field->usage = usages[u];
                        field->button_index = 0;
                        bit_offset += report_size;
                    }
                }
                // No usage, just fill fields
                else {
                    for (uint8_t rc = 0; rc < report_count && map->field_count < 32; rc++) {
                        hid_field_t* field = &map->fields[map->field_count++];
                        field->bit_offset = bit_offset;
                        field->bit_size = report_size;
                        field->logical_min = logical_min;
                        field->logical_max = logical_max;
                        field->usage_page = usage_page;
                        field->usage = 0;
                        field->button_index = 0;
                        bit_offset += report_size;
                    }
                }
                // Reset local state
                usage_count = 0;
                have_usage_min = have_usage_max = false;
            }
            // Reset local usages
            usage_count = 0;
            have_usage_min = have_usage_max = false;
            break;
        }
    }
}