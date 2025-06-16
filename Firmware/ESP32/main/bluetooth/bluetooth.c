#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "btstack_stdio_esp32.h"
#include "uni.h"
#include "sdkconfig.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/ring_rumble.h"
#include "led/led.h"
#include "log/log.h"
#include "gamepad/range.h"
#include "settings/settings.h"

#define LED_INTERVAL_MS         500U
#define JOY_DIF_THRESHOLD       0x50
#define TRIGGER_DIF_THRESHOLD   0x50

typedef struct {
    volatile bool connected;
    gamepad_pad_t report;
    uni_gamepad_t prev_uni;
    struct {
        bool joy_l;
        bool joy_r;
        bool trigger_l;
        bool trigger_r;
    } map;
} gp_context_t;

typedef struct {
    bool                    led_state;
    bool                    led_timer_running;
    btstack_timer_source_t  led_timer;
    gp_context_t            gp_ctx[GAMEPADS_MAX];
    bt_connect_cb_t         connect_cb;
    bt_gamepad_cb_t         gamepad_cb;
    ring_rumble_t           rumble_ring;
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

static void led_blink_cb(btstack_timer_source_t *ts) {
    bt_context.led_state = !bt_context.led_state;
    led_set(0, bt_context.led_state);
    btstack_run_loop_set_timer(ts, LED_INTERVAL_MS);
    btstack_run_loop_add_timer(ts);
}

static void led_blink_set_enabled(bool enable) {
    if (enable && !bt_context.led_timer_running) {
        bt_context.led_timer_running = true;
        bt_context.led_timer.process = led_blink_cb;
        btstack_run_loop_set_timer(&bt_context.led_timer, LED_INTERVAL_MS);
        btstack_run_loop_add_timer(&bt_context.led_timer);
    } else if (!enable && bt_context.led_timer_running) {
        bt_context.led_timer_running = false;
        btstack_run_loop_remove_timer(&bt_context.led_timer);
    }
}

static void set_rumble_cb(void* ctx) {
    uint8_t index = 0;
    gamepad_rumble_t rumble = {0};
    if (!ring_rumble_pop(&bt_context.rumble_ring, &index, &rumble)) {
        return;
    }
    uni_hid_device_t* bp_dev = uni_hid_device_get_instance_for_idx(index);
    if ((bp_dev == NULL) || !bt_context.gp_ctx[index].connected || 
        (bp_dev->report_parser.play_dual_rumble == NULL)) {
        return;
    }
    uint8_t duration = 100;
    if ((rumble.l_duration > 0) || (rumble.r_duration > 0)) {
        duration = MAX(rumble.l_duration, rumble.r_duration);
    } 
    else if (bp_dev->controller_type == CONTROLLER_TYPE_XBoxOneController) {
        duration += 10;
    }
    bp_dev->report_parser.play_dual_rumble(bp_dev, 0, duration, rumble.l, rumble.r);

    if (!ring_rumble_empty(&bt_context.rumble_ring)) {
        bt_context.rumble_cb_reg.callback = set_rumble_cb;
        bt_context.rumble_cb_reg.context = NULL;
        btstack_run_loop_execute_on_main_thread(&bt_context.rumble_cb_reg);
    }
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
    (void)rssi;
    (void)name;
    (void)addr;

    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_GAMEPAD) == 0) {
        return UNI_ERROR_IGNORE_DEVICE;
    }
    return UNI_ERROR_SUCCESS;
}

static void bp32_device_connected_cb(uni_hid_device_t* device) {
    (void)device;
}

static uni_error_t bp32_device_ready_cb(uni_hid_device_t* device) {
    int index = uni_hid_device_get_idx_for_instance(device);
    if (index < 0 || index >= GAMEPADS_MAX) {
        return UNI_ERROR_INVALID_CONTROLLER;
    }
    bt_context.gp_ctx[index].connected = true;
    led_blink_set_enabled(false);
    led_set(index, true);
    if (bt_context.connect_cb) {
        bt_context.connect_cb(index, true);
    }
    return UNI_ERROR_SUCCESS;
}

static void bp32_device_disconnect_cb(uni_hid_device_t* device) {
    int index = uni_hid_device_get_idx_for_instance(device);
    if (index < 0 || index >= GAMEPADS_MAX) {
        return;
    }
    bt_context.gp_ctx[index].connected = false;
    if (!any_connected()) {
        led_blink_set_enabled(true);
    }
    if (bt_context.connect_cb) {
        bt_context.connect_cb(index, false);
    }
}

static void bp32_oob_event_cb(uni_platform_oob_event_t event, void* data) {
    (void)data;
    (void)event;
}

// Prevent I2C/SPI bus flooding
static inline bool gp_state_change(const uni_gamepad_t* uni_a, const uni_gamepad_t* uni_b) {
    if (memcmp(uni_a, uni_b, sizeof(uni_gamepad_t)) == 0) {
        return false;
    }
    if ((uni_a->dpad != uni_b->dpad) || 
        (uni_a->buttons != uni_b->buttons) ||
        (uni_a->misc_buttons != uni_b->misc_buttons)) {
        return true;
    }
    if ((abs(uni_a->brake - uni_b->brake) > TRIGGER_DIF_THRESHOLD)  ||
        (uni_a->brake <= 0) || (uni_a->brake >= R_UINT10_MAX)) {
        return true;
    }
    if ((abs(uni_a->throttle - uni_b->throttle) > TRIGGER_DIF_THRESHOLD) ||
        (uni_a->throttle <= 0) || (uni_a->throttle >= R_UINT10_MAX)) {
        return true;
    }
    if ((abs(uni_a->axis_x - uni_b->axis_x) > JOY_DIF_THRESHOLD) ||
        (uni_a->axis_x <= R_INT10_MIN) || (uni_a->axis_x >= R_INT10_MAX)) {
        return true;
    }
    if ((abs(uni_a->axis_y - uni_b->axis_y) > JOY_DIF_THRESHOLD) ||
        (uni_a->axis_y <= R_INT10_MIN) || (uni_a->axis_y >= R_INT10_MAX)) {
        return true;
    }
    if ((abs(uni_a->axis_rx - uni_b->axis_rx) > JOY_DIF_THRESHOLD) ||
        (uni_a->axis_rx <= R_INT10_MIN) || (uni_a->axis_rx >= R_INT10_MAX)) {   
        return true;
    }
    if ((abs(uni_a->axis_ry - uni_b->axis_ry) > JOY_DIF_THRESHOLD) ||
        (uni_a->axis_ry <= R_INT10_MIN) || (uni_a->axis_ry >= R_INT10_MAX)) {   
        return true;
    }
    return false;
}

static void bp32_controller_data_cb(uni_hid_device_t* device, uni_controller_t* controller) {
    if (controller->klass != UNI_CONTROLLER_CLASS_GAMEPAD){
        return;
    }

    const uni_gamepad_t* uni = &controller->gamepad;
    int index = uni_hid_device_get_idx_for_instance(device);

    if ((index < 0) || (index >= GAMEPADS_MAX) ||
        !gp_state_change(uni, &bt_context.gp_ctx[index].prev_uni)) {
        return;
    }

    gamepad_pad_t* pad = &bt_context.gp_ctx[index].report;
    memset(pad, 0, sizeof(gamepad_pad_t));

    switch (uni->dpad) {
        case DPAD_UP:
            pad->dpad |= GAMEPAD_D_UP;
            break;
        case DPAD_DOWN:
            pad->dpad |= GAMEPAD_D_DOWN;
            break;
        case DPAD_LEFT:
            pad->dpad |= GAMEPAD_D_LEFT;
            break;
        case DPAD_RIGHT:
            pad->dpad |= GAMEPAD_D_RIGHT;
            break;
        case DPAD_UP | DPAD_RIGHT:
            pad->dpad |= (GAMEPAD_D_UP | GAMEPAD_D_RIGHT);
            break;
        case DPAD_DOWN | DPAD_RIGHT:
            pad->dpad |= (GAMEPAD_D_DOWN | GAMEPAD_D_RIGHT);
            break;
        case DPAD_DOWN | DPAD_LEFT:
            pad->dpad |= (GAMEPAD_D_DOWN | GAMEPAD_D_LEFT);
            break;
        case DPAD_UP | DPAD_LEFT:
            pad->dpad |= (GAMEPAD_D_UP | GAMEPAD_D_LEFT);
            break;
        default:
            break;
    }

    if (uni->buttons & BUTTON_A)                { pad->buttons |= GAMEPAD_BTN_A; }
    if (uni->buttons & BUTTON_B)                { pad->buttons |= GAMEPAD_BTN_B; }
    if (uni->buttons & BUTTON_X)                { pad->buttons |= GAMEPAD_BTN_X; }
    if (uni->buttons & BUTTON_Y)                { pad->buttons |= GAMEPAD_BTN_Y; }
    if (uni->buttons & BUTTON_SHOULDER_L)       { pad->buttons |= GAMEPAD_BTN_LB; }
    if (uni->buttons & BUTTON_SHOULDER_R)       { pad->buttons |= GAMEPAD_BTN_RB; }
    if (uni->buttons & BUTTON_THUMB_L)          { pad->buttons |= GAMEPAD_BTN_L3; }
    if (uni->buttons & BUTTON_THUMB_R)          { pad->buttons |= GAMEPAD_BTN_R3; }
    if (uni->misc_buttons & MISC_BUTTON_BACK)   { pad->buttons |= GAMEPAD_BTN_BACK; }
    if (uni->misc_buttons & MISC_BUTTON_START)  { pad->buttons |= GAMEPAD_BTN_START; }
    if (uni->misc_buttons & MISC_BUTTON_SYSTEM) { pad->buttons |= GAMEPAD_BTN_SYS; }

    pad->trigger_l = range_uint10_to_uint8(uni->brake);
    pad->trigger_r = range_uint10_to_uint8(uni->throttle);

    pad->joystick_lx = range_int10_to_int16(uni->axis_x);
    pad->joystick_ly = range_invert_int16(range_int10_to_int16(uni->axis_y));
    pad->joystick_rx = range_int10_to_int16(uni->axis_rx);
    pad->joystick_ry = range_invert_int16(range_int10_to_int16(uni->axis_ry));

    if (bt_context.gamepad_cb != NULL) {
        bt_context.gamepad_cb(index, pad, GAMEPAD_FLAG_IN_PAD);
    }
    memcpy(&bt_context.gp_ctx[index].prev_uni, uni, sizeof(uni_gamepad_t));
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
    if ((index >= GAMEPADS_MAX) || 
        !bt_context.gp_ctx[index].connected || 
        (rumble == NULL)) {
        return;
    }
    ring_rumble_push(&bt_context.rumble_ring, index, rumble);
    bt_context.rumble_cb_reg.callback = set_rumble_cb;
    bt_context.rumble_cb_reg.context = NULL;
    btstack_run_loop_execute_on_main_thread(&bt_context.rumble_cb_reg);
}

void bluetooth_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm) {
    (void)index;
    (void)pcm;
    // Bluetooth audio handling is not implemented in this version
}

void bluetooth_config(bt_connect_cb_t connect_cb, bt_gamepad_cb_t gamepad_cb, bt_audio_cb_t audio_cb) {
    (void)audio_cb;
    memset(&bt_context, 0, sizeof(bt_context_t));
    bt_context.connect_cb = connect_cb;
    bt_context.gamepad_cb = gamepad_cb;
}

void bluetooth_task(void* args) {
    (void)args;

    btstack_init();

    // ble_server_init();

    uni_platform_set_custom(&BP32_DRIVER);
    uni_init(0, NULL);

    led_init();
    led_blink_set_enabled(true);

    btstack_run_loop_execute();
}