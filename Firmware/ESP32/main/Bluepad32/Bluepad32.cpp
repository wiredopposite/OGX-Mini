#include <functional>

#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "btstack_stdio_esp32.h"
#include "uni.h"

#include "sdkconfig.h"
#include "I2CDriver/I2CDriver.h"
#include "Board/board_api.h"
#include "Bluepad32/Gamepad.h"
#include "Bluepad32/Bluepad32.h"

namespace BP32 {

static constexpr uint8_t  MAX_DEVICES = CONFIG_BLUEPAD32_MAX_DEVICES;
static constexpr uint32_t FEEDBACK_TIME_MS = 200;
static constexpr uint32_t LED_TIME_MS = 500;

I2CDriver i2c_driver_;
btstack_timer_source_t feedback_timer_;
std::atomic<bool> devs_conn_[MAX_DEVICES]{false};

static inline void send_feedback_cb(void* context)
{
    I2CDriver::PacketOut packet_out = reinterpret_cast<std::atomic<I2CDriver::PacketOut>*>(context)->load();
    uni_hid_device_t* bp_device = nullptr;

    if (!(bp_device = uni_hid_device_get_instance_for_idx(packet_out.index)) || 
        !uni_bt_conn_is_connected(&bp_device->conn) ||
        !bp_device->report_parser.play_dual_rumble)
    {
        return;
    }

    if (packet_out.rumble_l || packet_out.rumble_r)
    {
        bp_device->report_parser.play_dual_rumble(
            bp_device, 
            0, 
            FEEDBACK_TIME_MS, 
            packet_out.rumble_l, 
            packet_out.rumble_r
            );
    }
}

//This will have to be changed once full support for multiple devices is added
static inline void feedback_timer_cb(btstack_timer_source *ts)
{
    static btstack_context_callback_registration_t cb_registration[MAX_DEVICES];
    static std::atomic<I2CDriver::PacketOut> packets_out[MAX_DEVICES];

    uni_hid_device_t* bp_device = nullptr;

    for (uint8_t i = 0; i < MAX_DEVICES; ++i)
    {
        if (!(bp_device = uni_hid_device_get_instance_for_idx(i)) || 
            !uni_bt_conn_is_connected(&bp_device->conn))
        {
            continue;
        }

        cb_registration[i].callback = send_feedback_cb;
        cb_registration[i].context = reinterpret_cast<void*>(&packets_out[i]);
        
        //Register a read on i2c thread, with callback to send feedback on btstack thread
        i2c_driver_.i2c_read_blocking_safe( I2CDriver::MULTI_SLAVE ? i + 1 : 0x01,
                                            [i](const I2CDriver::PacketOut& packet_out)
                                            {
                                                packets_out[i].store(packet_out);
                                                btstack_run_loop_execute_on_main_thread(&cb_registration[i]);
                                            });
    }

    btstack_run_loop_set_timer(ts, FEEDBACK_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

inline void check_led_cb(btstack_timer_source *ts)
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
            board_api::set_led(i, devs_conn_[i].load() ? 1 : (led_state ? 1 : 0));
        }
    }

    btstack_run_loop_set_timer(ts, LED_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

//BT Driver

static void init(int argc, const char** arg_V) {}

static void init_complete_cb(void) 
{
    uni_bt_enable_new_connections_unsafe(true);

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
    {
        uni_bt_del_keys_unsafe();
    }
    else
    {
        uni_bt_list_keys_unsafe();
    }

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
}

void device_disconnected_cb(uni_hid_device_t* device) 
{
#ifdef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    logd("BP32", "Device disconnected, addr:  %p, index: %i\n", device, uni_hid_device_get_idx_for_instance(device));
#endif

    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_DEVICES || idx < 0)
    {
        return;
    }

    devs_conn_[idx].store(false);
    if (!any_connected())
    {
        btstack_run_loop_remove_timer(&feedback_timer_);
    }

    I2CDriver::PacketIn packet_in = I2CDriver::PacketIn();
    packet_in.index = static_cast<uint8_t>(idx);
    
    i2c_driver_.i2c_write_blocking_safe(I2CDriver::MULTI_SLAVE ? packet_in.index + 1 : 0x01, packet_in);
}

static uni_error_t device_ready_cb(uni_hid_device_t* device) 
{
    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_DEVICES || idx < 0)
    {
        return UNI_ERROR_IGNORE_DEVICE;
    }

    devs_conn_[idx].store(true);

    feedback_timer_.process = feedback_timer_cb;
    feedback_timer_.context = nullptr;

    btstack_run_loop_set_timer(&feedback_timer_, FEEDBACK_TIME_MS);
    btstack_run_loop_add_timer(&feedback_timer_);

    return UNI_ERROR_SUCCESS;
}

static inline void controller_data_cb(uni_hid_device_t* device, uni_controller_t* controller) 
{
    static uni_gamepad_t prev_uni_gps[MAX_DEVICES]{0};

    if (controller->klass != UNI_CONTROLLER_CLASS_GAMEPAD)
    {
        return;
    }

    uni_gamepad_t *uni_gp = &controller->gamepad;
    int idx = uni_hid_device_get_idx_for_instance(device);

    if (idx < 0 || std::memcmp(uni_gp, &prev_uni_gps[idx], sizeof(uni_gamepad_t)) == 0)
    {
        return;
    }

    I2CDriver::PacketIn packet_in;
    packet_in.index = static_cast<uint8_t>(idx);

    switch (uni_gp->dpad) 
    {
        case DPAD_UP:
            packet_in.dpad = Gamepad::DPad::UP;
            break;
        case DPAD_DOWN:
            packet_in.dpad = Gamepad::DPad::DOWN;
            break;
        case DPAD_LEFT:
            packet_in.dpad = Gamepad::DPad::LEFT;
            break;
        case DPAD_RIGHT:
            packet_in.dpad = Gamepad::DPad::RIGHT;
            break;
        case (DPAD_UP | DPAD_RIGHT):
            packet_in.dpad = Gamepad::DPad::UP_RIGHT;
            break;
        case (DPAD_DOWN | DPAD_RIGHT):
            packet_in.dpad = Gamepad::DPad::DOWN_RIGHT;
            break;
        case (DPAD_DOWN | DPAD_LEFT):
            packet_in.dpad = Gamepad::DPad::DOWN_LEFT;
            break;
        case (DPAD_UP | DPAD_LEFT):
            packet_in.dpad = Gamepad::DPad::UP_LEFT;
            break;
        default:
            break;
    }

    if (uni_gp->buttons & BUTTON_A) packet_in.buttons |= Gamepad::Button::A;
    if (uni_gp->buttons & BUTTON_B) packet_in.buttons |= Gamepad::Button::B;
    if (uni_gp->buttons & BUTTON_X) packet_in.buttons |= Gamepad::Button::X;
    if (uni_gp->buttons & BUTTON_Y) packet_in.buttons |= Gamepad::Button::Y;
    if (uni_gp->buttons & BUTTON_SHOULDER_L) packet_in.buttons |= Gamepad::Button::LB;
    if (uni_gp->buttons & BUTTON_SHOULDER_R) packet_in.buttons |= Gamepad::Button::RB;
    if (uni_gp->buttons & BUTTON_THUMB_L)    packet_in.buttons |= Gamepad::Button::L3;
    if (uni_gp->buttons & BUTTON_THUMB_R)    packet_in.buttons |= Gamepad::Button::R3;
    if (uni_gp->misc_buttons & MISC_BUTTON_BACK)    packet_in.buttons |= Gamepad::Button::BACK;
    if (uni_gp->misc_buttons & MISC_BUTTON_START)   packet_in.buttons |= Gamepad::Button::START;
    if (uni_gp->misc_buttons & MISC_BUTTON_SYSTEM)  packet_in.buttons |= Gamepad::Button::SYS;
    if (uni_gp->misc_buttons & MISC_BUTTON_CAPTURE) packet_in.buttons |= Gamepad::Button::MISC;

    packet_in.trigger_l = Scale::uint10_to_uint8(uni_gp->brake);
    packet_in.trigger_r = Scale::uint10_to_uint8(uni_gp->throttle);

    packet_in.joystick_lx = Scale::int10_to_int16(uni_gp->axis_x);
    packet_in.joystick_ly = Scale::int10_to_int16(uni_gp->axis_y);
    packet_in.joystick_rx = Scale::int10_to_int16(uni_gp->axis_rx);
    packet_in.joystick_ry = Scale::int10_to_int16(uni_gp->axis_ry);

    i2c_driver_.i2c_write_blocking_safe(I2CDriver::MULTI_SLAVE ? packet_in.index + 1 : 0x01, packet_in);

    std::memcpy(&prev_uni_gps[idx], uni_gp, sizeof(uni_gamepad_t));
}

static const uni_property_t* get_property_cb(uni_property_idx_t idx) 
{
    return nullptr;
}

static void oob_event_cb(uni_platform_oob_event_t event, void* data) 
{
    return;
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

void run_i2c_task(void* parameter)
{
    i2c_driver_.initialize_i2c();
    i2c_driver_.run_tasks();
}

//Public

bool any_connected()
{
    for (auto& connected : devs_conn_)
    {
        if (connected.load())
        {
            return true;
        }
    }
    return false;
}

bool connected(uint8_t index)
{
    return devs_conn_[index].load();
}

void run_task()
{
    board_api::init_pins(); 

    xTaskCreatePinnedToCore(
        run_i2c_task,
        "i2c",
        2048 * 2,
        nullptr,
        configMAX_PRIORITIES-8,
        nullptr,
        1 );
    
    btstack_init();

    uni_platform_set_custom(get_driver());
    uni_init(0, nullptr);

    btstack_timer_source_t led_timer;
    led_timer.process = check_led_cb;
    led_timer.context = nullptr;

    btstack_run_loop_set_timer(&led_timer, LED_TIME_MS);
    btstack_run_loop_add_timer(&led_timer);

    btstack_run_loop_execute();
}

} // namespace BP32