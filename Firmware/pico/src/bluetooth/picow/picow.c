#include "board_config.h"
#if (BLUETOOTH_HARDWARE == BLUETOOTH_HARDWARE_PICOW)

#include <pico/mutex.h>
#include <pico/cyw43_arch.h>
#include "btstack_run_loop.h"
#include "uni.h"

#include "bluetooth/bluetooth.h"
#include "bluetooth/picow/sdkconfig.h"
#include "led/led.h"
#include "gamepad/range.h"
#include "settings/settings.h"

#define LED_INTERVAL_MS 500U

typedef struct {
    volatile bool       connected;
    uint8_t             index;
    gamepad_pad_t       report;
    gamepad_pad_t       prev_report;
    gamepad_rumble_t    rumble;
    mutex_t             rumble_mutex;
    user_profile_t      profile;
    struct {
        bool joy_l;
        bool joy_r;
        bool trigger_l;
        bool trigger_r;
    } map;
} gp_context_t;

typedef struct {
    bool led_state;
    bool led_timer_running;
    btstack_timer_source_t led_timer;
    gp_context_t gp_ctx[GAMEPADS_MAX];
    bt_connect_cb_t connect_cb;
    bt_gamepad_cb_t gamepad_cb;
    btstack_context_callback_registration_t rumble_cb_reg;
} bt_context_t;

static bt_context_t bt_context = {0};

static inline bool any_connected(void) {
    for (int i = 0; i < GAMEPADS_MAX; i++) {
        if (bt_context.gp_ctx[i].connected) {
            return true;
        }
    }
    return false;
}

static void led_timer_cb(btstack_timer_source_t *ts) {
    bt_context.led_state = !bt_context.led_state;
    led_set_on(bt_context.led_state);
    btstack_run_loop_set_timer(ts, LED_INTERVAL_MS);
    btstack_run_loop_add_timer(ts);
}

static void set_rumble_cb(void* ctx) {
    gp_context_t* gp_ctx = (gp_context_t*)ctx;
    mutex_enter_blocking(&gp_ctx->rumble_mutex);
    gamepad_rumble_t rumble = gp_ctx->rumble;
    mutex_exit(&gp_ctx->rumble_mutex);
    uni_hid_device_t* bp_dev = uni_hid_device_get_instance_for_idx(gp_ctx->index);
    if ((bp_dev == NULL) || !gp_ctx->connected || 
        (bp_dev->report_parser.play_dual_rumble == NULL)) {
        return;
    }
    uint8_t duration = 100;
    if ((rumble.l_duration > 0) || (rumble.r_duration > 0)) {
        duration = MAX(rumble.l_duration, rumble.r_duration);
    } 
    // else if (bp_dev->controller_type == CONTROLLER_TYPE_XBoxOneController) {
    //     duration += 10;
    // }
    bp_dev->report_parser.play_dual_rumble(bp_dev, 0, duration, rumble.l, rumble.r);
}

/* ---- Bluepad32 driver ---- */

static void bp32_init_cb(int argc, const char** arg_V) {
    (void)argc;
    (void)arg_V;
}

static void bp32_init_complete_cb(void) {
    // uni_bt_enable_new_connections_unsafe(true);
    uni_bt_start_scanning_and_autoconnect_unsafe();
    // uni_bt_del_keys_unsafe();
    uni_property_dump_all();
}

static uni_error_t bp32_device_discovered_cb(bd_addr_t addr, const char* name, 
                                                    uint16_t cod, uint8_t rssi) {
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_GAMEPAD) == 0) {
        return UNI_ERROR_IGNORE_DEVICE;
    }
    return UNI_ERROR_SUCCESS;
}

static void bp32_device_connected_cb(uni_hid_device_t* device) {

}

static uni_error_t bp32_device_ready_cb(uni_hid_device_t* device) {
    int index = uni_hid_device_get_idx_for_instance(device);
    if (index < 0 || index >= GAMEPADS_MAX) {
        return UNI_ERROR_INVALID_CONTROLLER;
    }
    bt_context.gp_ctx[index].connected = true;
    if (bt_context.led_timer_running) {
        bt_context.led_timer_running = false;
        btstack_run_loop_remove_timer(&bt_context.led_timer);
        led_set_on(true);
    }
    if (bt_context.connect_cb) {
        bt_context.connect_cb(index, true);
    }
}

static void bp32_device_disconnect_cb(uni_hid_device_t* device) {
    int index = uni_hid_device_get_idx_for_instance(device);
    if (index < 0 || index >= GAMEPADS_MAX) {
        return;
    }
    bt_context.gp_ctx[index].connected = false;
    if (!bt_context.led_timer_running && !any_connected()) {
        bt_context.led_timer_running = true;
        bt_context.led_timer.process = led_timer_cb;
        btstack_run_loop_set_timer(&bt_context.led_timer, LED_INTERVAL_MS);
        btstack_run_loop_add_timer(&bt_context.led_timer);
    }
    if (bt_context.connect_cb) {
        bt_context.connect_cb(index, false);
    }
}

static void bp32_oob_event_cb(uni_platform_oob_event_t event, void* data) {
    (void)data;
    (void)event;
}

static void bp32_controller_data_cb(uni_hid_device_t* device, uni_controller_t* controller) {
    if (controller->klass != UNI_CONTROLLER_CLASS_GAMEPAD){
        return;
    }
    const uni_gamepad_t* uni = &controller->gamepad;
    int index = uni_hid_device_get_idx_for_instance(device);
    if (index < 0 || index >= GAMEPADS_MAX) {
        return;
    }
    gamepad_pad_t* pad = &bt_context.gp_ctx[index].report;
    user_profile_t* profile = &bt_context.gp_ctx[index].profile;
    memset(pad, 0, sizeof(gamepad_pad_t));

    switch (uni->dpad) {
        case DPAD_UP:
            pad->dpad |= GP_BIT8(profile->d_up);
            break;
        case DPAD_DOWN:
            pad->dpad |= GP_BIT8(profile->d_down);
            break;
        case DPAD_LEFT:
            pad->dpad |= GP_BIT8(profile->d_left);
            break;
        case DPAD_RIGHT:
            pad->dpad |= GP_BIT8(profile->d_right);
            break;
        case DPAD_UP | DPAD_RIGHT:
            pad->dpad |= (GP_BIT8(profile->d_up) | GP_BIT8(profile->d_right));
            break;
        case DPAD_DOWN | DPAD_RIGHT:
            pad->dpad |= (GP_BIT8(profile->d_down) | GP_BIT8(profile->d_right));
            break;
        case DPAD_DOWN | DPAD_LEFT:
            pad->dpad |= (GP_BIT8(profile->d_down) | GP_BIT8(profile->d_left));
            break;
        case DPAD_UP | DPAD_LEFT:
            pad->dpad |= (GP_BIT8(profile->d_up) | GP_BIT8(profile->d_left));
            break;
        default:
            break;
    }

    if (uni->buttons & BUTTON_A)                { pad->buttons |= GP_BIT16(profile->btn_a); }
    if (uni->buttons & BUTTON_B)                { pad->buttons |= GP_BIT16(profile->btn_b); }
    if (uni->buttons & BUTTON_X)                { pad->buttons |= GP_BIT16(profile->btn_x); }
    if (uni->buttons & BUTTON_Y)                { pad->buttons |= GP_BIT16(profile->btn_y); }
    if (uni->buttons & BUTTON_SHOULDER_L)       { pad->buttons |= GP_BIT16(profile->btn_lb); }
    if (uni->buttons & BUTTON_SHOULDER_R)       { pad->buttons |= GP_BIT16(profile->btn_rb); }
    if (uni->buttons & BUTTON_THUMB_L)          { pad->buttons |= GP_BIT16(profile->btn_l3); }
    if (uni->buttons & BUTTON_THUMB_R)          { pad->buttons |= GP_BIT16(profile->btn_r3); }
    if (uni->misc_buttons & MISC_BUTTON_BACK)   { pad->buttons |= GP_BIT16(profile->btn_back); }
    if (uni->misc_buttons & MISC_BUTTON_START)  { pad->buttons |= GP_BIT16(profile->btn_start); }
    if (uni->misc_buttons & MISC_BUTTON_SYSTEM) { pad->buttons |= GP_BIT16(profile->btn_sys); }

    pad->trigger_l = range_uint10_to_uint8(uni->brake);
    pad->trigger_r = range_uint10_to_uint8(uni->throttle);
    pad->joystick_lx = range_int10_to_int16(uni->axis_x);
    pad->joystick_ly = range_int10_to_int16(uni->axis_y);
    pad->joystick_rx = range_int10_to_int16(uni->axis_rx);
    pad->joystick_ry = range_int10_to_int16(uni->axis_ry);

    if (bt_context.gp_ctx[index].map.joy_l) {
        settings_scale_joysticks(&profile->joystick_l, &pad->joystick_lx, &pad->joystick_ly, false);
    }
    if (bt_context.gp_ctx[index].map.joy_r) {
        settings_scale_joysticks(&profile->joystick_r, &pad->joystick_rx, &pad->joystick_ry, false);
    }
    if (bt_context.gp_ctx[index].map.trigger_l) {
        settings_scale_trigger(&profile->trigger_l, &pad->trigger_l);
    }
    if (bt_context.gp_ctx[index].map.trigger_r) {
        settings_scale_trigger(&profile->trigger_r, &pad->trigger_r);
    }
    if (memcmp(pad, &bt_context.gp_ctx[index].prev_report, sizeof(gamepad_pad_t)) == 0) {
        return;
    }
    if (bt_context.gamepad_cb) {
        bt_context.gamepad_cb(index, pad, GAMEPAD_FLAG_IN_PAD);
    }
    memccpy(&bt_context.gp_ctx[index].prev_report, pad, 0, sizeof(gamepad_pad_t));
}

static const uni_property_t* bp32_get_property_cb(uni_property_idx_t idx) {
    (void)idx;
    return NULL;
}

static struct uni_platform BP32_DRIVER = {
    .name = "OGX-Mini",
    .init = bp32_init_cb,
    .on_init_complete = bp32_init_complete_cb,
    .on_device_discovered = bp32_device_discovered_cb,
    .on_device_connected = bp32_device_connected_cb,
    .on_device_disconnected = bp32_device_disconnect_cb,
    .on_device_ready = bp32_device_ready_cb,
    .on_controller_data = bp32_controller_data_cb,
    .get_property = bp32_get_property_cb,
    .on_oob_event = bp32_oob_event_cb,
};

/* ---- Public ---- */

void bluetooth_set_rumble(uint8_t index, const gamepad_rumble_t* rumble) {
    gp_context_t* gp_ctx = &bt_context.gp_ctx[index];
    if (!gp_ctx->connected || (rumble == NULL)) {
        return;
    }
    mutex_enter_blocking(&gp_ctx->rumble_mutex);
    memcpy(&gp_ctx->rumble, rumble, sizeof(gamepad_rumble_t));
    bt_context.rumble_cb_reg.callback = set_rumble_cb;
    bt_context.rumble_cb_reg.context = &bt_context.gp_ctx[index];
    mutex_exit(&gp_ctx->rumble_mutex);
    btstack_run_loop_execute_on_main_thread(&bt_context.rumble_cb_reg);
}

void bluetooth_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm) {
    (void)index;
    (void)pcm;
    // Bluetooth audio handling is not implemented in this version
}

void bluetooth_task(void) {}

bool bluetooth_init(bt_connect_cb_t connect_cb, bt_gamepad_cb_t gamepad_cb, bt_audio_cb_t audio_cb) {
    // for (uint8_t i = 0; i < GAMEPADS_MAX; i++) {
    //     bt_context.gp_ctx[i].index = i;
    //     bt_context.gp_ctx[i].connected = false;
    //     // settings_get_profile_by_index(i, &bt_context.gp_ctx[i].profile);
    //     settings_get_default_profile(&bt_context.gp_ctx[i].profile);
    //     bt_context.gp_ctx[i].map.joy_l = 
    //         !settings_is_default_joystick(&bt_context.gp_ctx[i].profile.joystick_l);
    //     bt_context.gp_ctx[i].map.joy_r = 
    //         !settings_is_default_joystick(&bt_context.gp_ctx[i].profile.joystick_r);
    //     bt_context.gp_ctx[i].map.trigger_l = 
    //         !settings_is_default_trigger(&bt_context.gp_ctx[i].profile.trigger_l);
    //     bt_context.gp_ctx[i].map.trigger_r = 
    //         !settings_is_default_trigger(&bt_context.gp_ctx[i].profile.trigger_r);
    //     mutex_init(&bt_context.gp_ctx[i].rumble_mutex);
    // }
    bt_context.connect_cb = connect_cb;
    bt_context.gamepad_cb = gamepad_cb;

    if (cyw43_arch_init() != 0) {  
        return false;
    }

    uni_platform_set_custom(&BP32_DRIVER);
    uni_init(0, NULL);

    // led_init();
    // bt_context.led_timer_running = true;
    // bt_context.led_timer.process = led_timer_cb;
    // btstack_run_loop_set_timer(&bt_context.led_timer, LED_INTERVAL_MS);
    // btstack_run_loop_add_timer(&bt_context.led_timer);

    btstack_run_loop_execute();
    return true;
}

#endif // (BLUETOOTH_HARDWARE == BLUETOOTH_HARDWARE_PICOW)