#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "btstack_stdio_esp32.h"
#include "uni.h"

#include "sdkconfig.h"
#include "Board/ogxm_log.h"
#include "Board/board_api.h"
#include "BTManager/BTManager.h"
#include "BLEServer/BLEServer.h"

void BTManager::run_task()
{
    board_api::init_board(); 
    UserSettings::get_instance().initialize_flash();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        UserProfile profile = UserSettings::get_instance().get_profile_by_index(i);
        devices_[i].mapper.set_profile(profile);
    }

    i2c_driver_.initialize_i2c(
        static_cast<i2c_port_t>(CONFIG_I2C_PORT), 
        static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN), 
        static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN),  
        CONFIG_I2C_BAUDRATE
    );

    xTaskCreatePinnedToCore(
        [](void* parameter)
        { 
            get_instance().i2c_driver_.run_tasks(); 
        },
        "i2c",
        2048 * 2,
        nullptr,
        configMAX_PRIORITIES-8,
        nullptr,
        1 
    );

    btstack_init();

    uni_platform_set_custom(get_bp32_driver());
    uni_init(0, nullptr);

    btstack_timer_source_t led_timer;
    led_timer.process = check_led_cb;
    led_timer.context = nullptr;
    btstack_run_loop_set_timer(&led_timer, LED_TIME_MS);
    btstack_run_loop_add_timer(&led_timer);

    btstack_timer_source_t driver_update_timer;
    driver_update_timer.process = driver_update_timer_cb;
    driver_update_timer.context = nullptr;
    btstack_run_loop_set_timer(&driver_update_timer, UserSettings::GP_CHECK_DELAY_MS);
    btstack_run_loop_add_timer(&driver_update_timer);

    BLEServer::init_server();

    //Doesn't return
    btstack_run_loop_execute();
}

bool BTManager::is_connected(uint8_t index)
{
    if (index >= devices_.size())
    {
        return false;
    }
    return devices_[index].connected.load();
}

bool BTManager::any_connected()
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

void BTManager::check_led_cb(btstack_timer_source *ts)
{
    static bool led_state = false;
    led_state = !led_state;

    if constexpr (board_api::NUM_LEDS == 1)
    {
        board_api::set_led(get_instance().any_connected() ? true : (led_state ? true : false));
    }
    else
    {
        for (uint8_t i = 0; i < board_api::NUM_LEDS; ++i)
        {
            board_api::set_led(i, get_instance().is_connected(i) ? true : (led_state ? true : false));
        }
    }

    btstack_run_loop_set_timer(ts, LED_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

void BTManager::send_driver_type(DeviceDriverType driver_type)
{
    if constexpr (I2CDriver::MULTI_SLAVE)
    {
        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
        {
            I2CDriver::PacketIn packet_in = devices_[i].packet_in;
            packet_in.packet_id = I2CDriver::PacketID::SET_DRIVER;
            packet_in.index = i;
            packet_in.device_driver = driver_type;
            i2c_driver_.write_packet(i + 1, packet_in);
        }
    }
    else
    {
        I2CDriver::PacketIn packet_in = devices_[0].packet_in;
        packet_in.packet_id = I2CDriver::PacketID::SET_DRIVER;
        packet_in.index = 0;
        packet_in.device_driver = driver_type;
        i2c_driver_.write_packet(0x01, packet_in);
    }
}

uni_hid_device_t* BTManager::get_connected_bp32_device(uint8_t index)
{
    uni_hid_device_t* bp_device = nullptr;
    if (!(bp_device = uni_hid_device_get_instance_for_idx(index)) || 
        !uni_bt_conn_is_connected(&bp_device->conn) ||
        !bp_device->report_parser.play_dual_rumble)
    {
        return nullptr;
    }
    return bp_device;
}

void BTManager::driver_update_timer_cb(btstack_timer_source *ts)
{
    BTManager& bt_manager = get_instance();
    I2CDriver::PacketIn& packet_in = bt_manager.devices_.front().packet_in;

    if (get_connected_bp32_device(0) &&
        UserSettings::get_instance().check_for_driver_change(packet_in))
    {
        OGXM_LOG("BP32: Driver change detected\n");
        UserSettings::get_instance().store_driver_type(UserSettings::get_instance().get_current_driver());
    }

    //Notify pico of current driver type regardless of change
    bt_manager.send_driver_type(UserSettings::get_instance().get_current_driver());

    btstack_run_loop_set_timer(ts, UserSettings::GP_CHECK_DELAY_MS);
    btstack_run_loop_add_timer(ts);
}

void BTManager::send_feedback_cb(void* context)
{
    FBContext* fb_context = reinterpret_cast<FBContext*>(context);
    uni_hid_device_t* bp_device = nullptr;

    if (!(bp_device = get_connected_bp32_device(fb_context->index)))
    {
        return;
    }

    I2CDriver::PacketOut packet_out = fb_context->packet_out->load();

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
void BTManager::feedback_timer_cb(btstack_timer_source *ts)
{
    static FBContext fb_contexts[MAX_GAMEPADS];
    BTManager& bt_manager = get_instance();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        if (!get_connected_bp32_device(i))
        {
            continue;
        }

        FBContext& fb_context = fb_contexts[i];
        fb_context.index = i;
        fb_context.packet_out = &bt_manager.devices_[i].packet_out;
        fb_context.cb_reg.callback = send_feedback_cb;
        fb_context.cb_reg.context = reinterpret_cast<void*>(&fb_context);

        //Register a read on i2c thread, with callback to send feedback on btstack thread
        bt_manager.i2c_driver_.read_packet(I2CDriver::MULTI_SLAVE ? i + 1 : 0x01,
            [&fb_context](const I2CDriver::PacketOut& packet_out)
            {
                fb_context.packet_out->store(packet_out);
                btstack_run_loop_execute_on_main_thread(&fb_context.cb_reg);
            });
    }

    btstack_run_loop_set_timer(ts, FEEDBACK_TIME_MS);
    btstack_run_loop_add_timer(ts);
}

void BTManager::manage_connection(uint8_t index, bool connected)
{
    devices_[index].connected.store(connected);
    if (connected)
    {
        if (!fb_timer_running_)
        {
            fb_timer_running_ = true;
            fb_timer_.process = feedback_timer_cb;
            fb_timer_.context = nullptr;
            btstack_run_loop_set_timer(&fb_timer_, FEEDBACK_TIME_MS);
            btstack_run_loop_add_timer(&fb_timer_);
        }
    }
    else
    {
        if (!any_connected())
        {
            if (fb_timer_running_)
            {
                fb_timer_running_ = false;
                btstack_run_loop_remove_timer(&fb_timer_);
            }
        }

        I2CDriver::PacketIn packet_in = I2CDriver::PacketIn();
        packet_in.packet_id = I2CDriver::PacketID::SET_PAD;
        packet_in.index = index;
        i2c_driver_.write_packet(I2CDriver::MULTI_SLAVE ? packet_in.index + 1 : 0x01, packet_in);
    }
}

I2CDriver::PacketIn BTManager::get_packet_in(uint8_t index)
{
    if (index >= devices_.size())
    {
        return I2CDriver::PacketIn();
    }
    return devices_[index].packet_in;
}