#include <cstdint>
#include <atomic>
#include <cstring>
#include <array>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/queue.h>
#include <esp_log.h>

#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "btstack_stdio_esp32.h"
#include "uni.h"

#include "sdkconfig.h"
#include "Board/board_api.h"
#include "I2CDriver/I2CDriver.h"
#include "Bluepad32/Gamepad.h"
#include "Bluepad32/Bluepad32.h"

namespace bluepad32 {

static constexpr uint32_t FEEDBACK_TIME_MS = 200;
static constexpr uint32_t LED_TIME_MS = 500;

struct Device
{
    std::atomic<bool> connected{false};
    std::atomic<ReportIn> report_in{ReportIn()};
    std::atomic<ReportOut> report_out{ReportOut()};
};

std::array<Device, CONFIG_BLUEPAD32_MAX_DEVICES> devices_;

static inline void send_feedback_cb(btstack_timer_source *ts)
{
    uni_hid_device_t* bp_device;

    for (uint8_t i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
    {
        bp_device = uni_hid_device_get_instance_for_idx(i);
        if (!bp_device || !bp_device->report_parser.play_dual_rumble)
        {
            continue;
        }

        ReportOut report_out = devices_[i].report_out.load();
        if (!report_out.rumble_l && !report_out.rumble_r)
        {
            continue;
        }
        bp_device->report_parser.play_dual_rumble(bp_device, 0, FEEDBACK_TIME_MS, report_out.rumble_l, report_out.rumble_r);
    }

    btstack_run_loop_set_timer(ts, FEEDBACK_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

static inline void check_led_cb(btstack_timer_source *ts)
{
    static bool led_state = false;
    led_state = !led_state;

    if constexpr (board_api::NUM_LEDS == 1)
    {
        board_api::set_led(any_connected() ? 1 : (led_state ? 1 : 0));
    }
    else
    {
        for (uint8_t i = 0; i < board_api::NUM_LEDS; ++i)
        {
            board_api::set_led(i, devices_[i].connected.load() ? 1 : (led_state ? 1 : 0));
        }
    }

    btstack_run_loop_set_timer(ts, LED_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

//BT Driver

static void init(int argc, const char** arg_V)
{

}

static void init_complete_cb(void) 
{
    uni_bt_enable_new_connections_unsafe(true);

    // // Based on runtime condition, you can delete or list the stored BT keys.
    // if (1)
    // {
    //     uni_bt_del_keys_unsafe();
    // }
    // else
    // {
    //     uni_bt_list_keys_unsafe();
    // }

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
#ifdef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    logd("BP32", "Device connected, addr:  %p, index: %i\n", device, uni_hid_device_get_idx_for_instance(device));
#endif

    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= CONFIG_BLUEPAD32_MAX_DEVICES || idx < 0)
    {
        return;
    }
    devices_[idx].connected.store(true);
}

static void device_disconnected_cb(uni_hid_device_t* device) 
{
#ifdef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    logd("BP32", "Device disconnected, addr:  %p, index: %i\n", device, uni_hid_device_get_idx_for_instance(device));
#endif

    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= CONFIG_BLUEPAD32_MAX_DEVICES || idx < 0)
    {
        return;
    }

    ReportIn report_in = ReportIn();
    report_in.index = static_cast<uint8_t>(idx);
    devices_[idx].report_in.store(report_in);
    devices_[idx].connected.store(false);
}

static uni_error_t device_ready_cb(uni_hid_device_t* device) 
{
    return UNI_ERROR_SUCCESS;
}

static void oob_event_cb(uni_platform_oob_event_t event, void* data) 
{
	return;
}

static void controller_data_cb(uni_hid_device_t* device, uni_controller_t* controller) 
{
    static uni_gamepad_t prev_uni_gp[CONFIG_BLUEPAD32_MAX_DEVICES] = {};

    if (controller->klass != UNI_CONTROLLER_CLASS_GAMEPAD)
    {
        return;
    }

    uni_gamepad_t *uni_gp = &controller->gamepad;
    int idx = uni_hid_device_get_idx_for_instance(device);

    if (idx >= CONFIG_BLUEPAD32_MAX_DEVICES || idx < 0 || std::memcmp(uni_gp, &prev_uni_gp[idx], sizeof(uni_gamepad_t)) == 0)
    {
        return;
    }

    ReportIn report_in;
    report_in.index = static_cast<uint8_t>(idx);

    switch (uni_gp->dpad) 
    {
        case DPAD_UP:
            report_in.dpad = Gamepad::DPad::UP;
            break;
        case DPAD_DOWN:
            report_in.dpad = Gamepad::DPad::DOWN;
            break;
        case DPAD_LEFT:
            report_in.dpad = Gamepad::DPad::LEFT;
            break;
        case DPAD_RIGHT:
            report_in.dpad = Gamepad::DPad::RIGHT;
            break;
        case DPAD_UP | DPAD_RIGHT:
            report_in.dpad = Gamepad::DPad::UP_RIGHT;
            break;
        case DPAD_DOWN | DPAD_RIGHT:
            report_in.dpad = Gamepad::DPad::DOWN_RIGHT;
            break;
        case DPAD_DOWN | DPAD_LEFT:
            report_in.dpad = Gamepad::DPad::DOWN_LEFT;
            break;
        case DPAD_UP | DPAD_LEFT:
            report_in.dpad = Gamepad::DPad::UP_LEFT;
            break;
        default:
            break;
    }

    if (uni_gp->buttons & BUTTON_A) report_in.buttons |= Gamepad::Button::A;
    if (uni_gp->buttons & BUTTON_B) report_in.buttons |= Gamepad::Button::B;
    if (uni_gp->buttons & BUTTON_X) report_in.buttons |= Gamepad::Button::X;
    if (uni_gp->buttons & BUTTON_Y) report_in.buttons |= Gamepad::Button::Y;
    if (uni_gp->buttons & BUTTON_SHOULDER_L) report_in.buttons |= Gamepad::Button::LB;
    if (uni_gp->buttons & BUTTON_SHOULDER_R) report_in.buttons |= Gamepad::Button::RB;
    if (uni_gp->buttons & BUTTON_THUMB_L)    report_in.buttons |= Gamepad::Button::L3;
    if (uni_gp->buttons & BUTTON_THUMB_R)    report_in.buttons |= Gamepad::Button::R3;
    if (uni_gp->misc_buttons & MISC_BUTTON_BACK)    report_in.buttons |= Gamepad::Button::BACK;
    if (uni_gp->misc_buttons & MISC_BUTTON_START)   report_in.buttons |= Gamepad::Button::START;
    if (uni_gp->misc_buttons & MISC_BUTTON_SYSTEM)  report_in.buttons |= Gamepad::Button::SYS;
    if (uni_gp->misc_buttons & MISC_BUTTON_CAPTURE) report_in.buttons |= Gamepad::Button::MISC;

    report_in.trigger_l = Gamepad::scale_trigger(uni_gp->brake);
    report_in.trigger_r = Gamepad::scale_trigger(uni_gp->throttle);

    report_in.joystick_lx = static_cast<int16_t>(uni_gp->axis_x);
    report_in.joystick_ly = static_cast<int16_t>(uni_gp->axis_y);
    report_in.joystick_rx = static_cast<int16_t>(uni_gp->axis_rx);
    report_in.joystick_ry = static_cast<int16_t>(uni_gp->axis_ry);

    devices_[idx].report_in.store(report_in);

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

//Public 

ReportIn get_report_in(uint8_t index)
{
    return devices_[index].report_in.load();
}

void set_report_out(const ReportOut& report_out)
{
    if (report_out.index >= CONFIG_BLUEPAD32_MAX_DEVICES)
    {
        return;
    }
    devices_[report_out.index].report_out.store(report_out);
}

void run_task()
{
    for (uint8_t i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i)
    {
        ReportIn report_in;
        report_in.index = i;
        devices_[i].report_in.store(report_in);
        devices_[i].report_out.store(ReportOut());
    }

    board_api::init_pins(); 
    
    btstack_init();

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

    btstack_run_loop_set_timer(&led_timer, LED_TIME_MS);
    btstack_run_loop_add_timer(&led_timer);

    btstack_run_loop_execute();
}

//Thread safe
bool any_connected()
{
    for (auto& device : devices_)
    {
        if (device.connected.load())
        {
            return true;
        }
    }
    return false;
}

bool connected(uint8_t index)
{
    return devices_[index].connected.load();
}

} // namespace bluepad32 