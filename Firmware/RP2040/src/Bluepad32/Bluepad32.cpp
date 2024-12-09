#include <atomic>
#include <cstring>
#include <functional>
#include <pico/mutex.h>
#include <pico/cyw43_arch.h>

#include "btstack_run_loop.h"
#include "uni.h"

#include "sdkconfig.h"
#include "Bluepad32/Bluepad32.h"
#include "Board/board_api.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

static_assert((CONFIG_BLUEPAD32_MAX_DEVICES == MAX_GAMEPADS), "Mismatch between BP32 and Gamepad max devices");

namespace bluepad32 {

static constexpr uint32_t FEEDBACK_TIME_MS = 200;
static constexpr uint32_t LED_CHECK_TIME_MS = 500;

struct Device
{
    bool connected{false};
    Gamepad* gamepad{nullptr};
};

std::array<Device, MAX_GAMEPADS> devices_;

//This solves a null function pointer issue with bluepad32, device->report_parser.play_dual_rumble() becomes null before the disconnect callback
void set_rumble(uni_hid_device_t* bp_device, uint16_t length, uint8_t rumble_l, uint8_t rumble_r)
{
    if (!bp_device || !bp_device->report_parser.play_dual_rumble)
    {
        return;
    }
    switch (bp_device->controller_type)
    {
        case CONTROLLER_TYPE_XBoxOneController:
            uni_hid_parser_xboxone_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_AndroidController:
            if (bp_device->vendor_id == UNI_HID_PARSER_STADIA_VID && bp_device->product_id == UNI_HID_PARSER_STADIA_PID) 
            {
                uni_hid_parser_stadia_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            }
            break;
        case CONTROLLER_TYPE_PSMoveController:
            uni_hid_parser_psmove_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_PS3Controller:
            uni_hid_parser_ds3_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_PS4Controller:
            uni_hid_parser_ds4_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_PS5Controller:
            uni_hid_parser_ds5_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_WiiController:
            uni_hid_parser_wii_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        case CONTROLLER_TYPE_SwitchProController:
        case CONTROLLER_TYPE_SwitchJoyConRight:
        case CONTROLLER_TYPE_SwitchJoyConLeft:
            uni_hid_parser_switch_play_dual_rumble(bp_device, 0, length, rumble_l, rumble_r);
            break;
        default:
            break;
    }
}

static void send_feedback_cb(btstack_timer_source *ts)
{
    uni_hid_device_t* bp_device = nullptr;

    for (uint8_t i = 0; i < devices_.size(); ++i)
    {
        if (!devices_[i].connected || 
            !(bp_device = uni_hid_device_get_instance_for_idx(i)))
        {
            continue;
        }

        uint8_t rumble_l = devices_[i].gamepad->get_rumble_l().uint8();
        uint8_t rumble_r = devices_[i].gamepad->get_rumble_r().uint8();
        if (rumble_l > 0 || rumble_r > 0)
        {
            // bp_device->report_parser.play_dual_rumble(bp_device, 0, static_cast<uint16_t>(FEEDBACK_TIME_MS), rumble_l, rumble_r);
            set_rumble(bp_device, static_cast<uint16_t>(FEEDBACK_TIME_MS), rumble_l, rumble_r);
        }
    }

    btstack_run_loop_set_timer(ts, FEEDBACK_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

static void check_led_cb(btstack_timer_source *ts)
{
    static bool led_state = false;

    led_state = !led_state;

    board_api::set_led(any_connected() ? true : led_state);

    btstack_run_loop_set_timer(ts, LED_CHECK_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

//BT Driver

static void init(int argc, const char** arg_V)
{

}

static void init_complete_cb(void) 
{
    uni_bt_enable_new_connections_unsafe(true);

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();

    uni_property_dump_all();
}

static uni_error_t device_discovered_cb(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) 
{
    if (!((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_GAMEPAD))
    {
        return UNI_ERROR_IGNORE_DEVICE;
    }
    return UNI_ERROR_SUCCESS;
}

static void device_connected_cb(uni_hid_device_t* device) 
{

}

static void device_disconnected_cb(uni_hid_device_t* device) 
{
    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_GAMEPADS || idx < 0)
    {
        return;
    }

    devices_[idx].connected = false;
    devices_[idx].gamepad->reset_pad();
}

static uni_error_t device_ready_cb(uni_hid_device_t* device) 
{    
    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_GAMEPADS || idx < 0)
    {
        return UNI_ERROR_SUCCESS;
    }

    devices_[idx].connected = true;
    return UNI_ERROR_SUCCESS;
}

static void oob_event_cb(uni_platform_oob_event_t event, void* data) 
{
	return;
}

static void controller_data_cb(uni_hid_device_t* device, uni_controller_t* controller) 
{
    static uni_gamepad_t prev_uni_gp[MAX_GAMEPADS] = {};

    if (controller->klass != UNI_CONTROLLER_CLASS_GAMEPAD)
    {
        return;
    }

    uni_gamepad_t *uni_gp = &controller->gamepad;
    int idx = uni_hid_device_get_idx_for_instance(device);

    if (idx >= MAX_GAMEPADS || idx < 0 || std::memcmp(uni_gp, &prev_uni_gp[idx], sizeof(uni_gamepad_t)) == 0)
    {
        return;
    }

    Gamepad* gamepad = devices_[idx].gamepad;
    gamepad->reset_pad();

    switch (uni_gp->dpad) 
    {
        case DPAD_UP:
            gamepad->set_dpad_up();
            break;
        case DPAD_DOWN:
            gamepad->set_dpad_down();
            break;
        case DPAD_LEFT:
            gamepad->set_dpad_left();
            break;
        case DPAD_RIGHT:
            gamepad->set_dpad_right();
            break;
        case DPAD_UP | DPAD_RIGHT:
            gamepad->set_dpad_up_right();
            break;
        case DPAD_DOWN | DPAD_RIGHT:
            gamepad->set_dpad_down_right();
            break;
        case DPAD_DOWN | DPAD_LEFT:
            gamepad->set_dpad_down_left();
            break;
        case DPAD_UP | DPAD_LEFT:
            gamepad->set_dpad_up_left();
            break;
        default:
            break;
    }

    if (uni_gp->buttons & BUTTON_A) gamepad->set_button_a();
    if (uni_gp->buttons & BUTTON_B) gamepad->set_button_b();
    if (uni_gp->buttons & BUTTON_X) gamepad->set_button_x();
    if (uni_gp->buttons & BUTTON_Y) gamepad->set_button_y();
    if (uni_gp->buttons & BUTTON_SHOULDER_L) gamepad->set_button_lb();
    if (uni_gp->buttons & BUTTON_SHOULDER_R) gamepad->set_button_rb();
    if (uni_gp->buttons & BUTTON_THUMB_L)    gamepad->set_button_l3();
    if (uni_gp->buttons & BUTTON_THUMB_R)    gamepad->set_button_r3();
    if (uni_gp->misc_buttons & MISC_BUTTON_BACK)    gamepad->set_button_back();
    if (uni_gp->misc_buttons & MISC_BUTTON_START)   gamepad->set_button_start();
    if (uni_gp->misc_buttons & MISC_BUTTON_SYSTEM)  gamepad->set_button_sys();
    if (uni_gp->misc_buttons & MISC_BUTTON_CAPTURE) gamepad->set_button_misc();

    gamepad->set_trigger_l_uint10(uni_gp->brake);
    gamepad->set_trigger_r_uint10(uni_gp->throttle);

    gamepad->set_joystick_lx_int10(uni_gp->axis_x);
    gamepad->set_joystick_ly_int10(uni_gp->axis_y);
    gamepad->set_joystick_rx_int10(uni_gp->axis_rx);
    gamepad->set_joystick_ry_int10(uni_gp->axis_ry);

    std::memcpy(uni_gp, &prev_uni_gp[idx], sizeof(uni_gamepad_t));
}

const uni_property_t* get_property_cb(uni_property_idx_t idx) 
{
    return nullptr;
}

uni_platform* get_driver() 
{
    static uni_platform driver = 
    {
        .name = "OGXMiniW",
        .init = init,
        .on_init_complete = init_complete_cb,
        .on_device_discovered = device_discovered_cb,
        .on_device_connected = device_connected_cb,
        .on_device_disconnected = device_disconnected_cb,
        .on_device_ready = device_ready_cb,
        .on_controller_data = controller_data_cb,
        .get_property = get_property_cb,
        .on_oob_event = oob_event_cb,
    };
    return &driver;
}

//Public API

void run_task(std::array<Gamepad, MAX_GAMEPADS>& gamepads)
{
    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        devices_[i].gamepad = &gamepads[i];
    }

    uni_platform_set_custom(get_driver());
    uni_init(0, nullptr);

    btstack_timer_source_t feedback_timer;
    feedback_timer.process = send_feedback_cb;
    feedback_timer.context = nullptr;
    btstack_run_loop_set_timer(&feedback_timer, FEEDBACK_TIME_MS);
    btstack_run_loop_add_timer(&feedback_timer);

    btstack_timer_source_t led_timer;
    led_timer.process = check_led_cb;
    led_timer.context = nullptr;
    btstack_run_loop_set_timer(&led_timer, LED_CHECK_TIME_MS);
    btstack_run_loop_add_timer(&led_timer);

    btstack_run_loop_execute();
}

std::array<bool, MAX_GAMEPADS> get_connected_map()
{
    std::array<bool, MAX_GAMEPADS> mounted_map;
    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        mounted_map[i] = devices_[i].connected;
    }
    return mounted_map;
}

bool any_connected()
{
    for (auto& device : devices_)
    {
        if (device.connected)
        {
            return true;
        }
    }
    return false;
}

} // namespace bluepad32 