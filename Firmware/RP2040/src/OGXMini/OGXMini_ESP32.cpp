#include "board_config.h"
#if OGXM_BOARD == W_ESP32

#include <pico/multicore.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "OGXMini/OGXMini.h"
#include "I2CDriver/ESP32/I2CDriver.h"
#include "Gamepad.h"
#include "TaskQueue/TaskQueue.h"

namespace OGXMini {

static Gamepad gamepads_[MAX_GAMEPADS];

//Not using this for ESP32 currently
void update_tud_status(bool host_mounted) { }

void core1_task()
{
    I2CDriver::initialize(gamepads_);

    while (true) 
    {
        tight_loop_contents();
    }
}

void run_esp32_uart_bridge()
{
    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(DeviceDriver::Type::UART_BRIDGE, gamepads_);

    board_api::enter_esp32_prog_mode();
    
    device_manager.get_driver()->process(0, gamepads_[0]); //Runs UART Bridge task, doesn't return
}

void set_gp_check_timer(uint32_t task_id, UserSettings& user_settings)
{
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true, [&user_settings]
    {
        //Check gamepad inputs for button combo to change usb device driver
        if (user_settings.check_for_driver_change(gamepads_[0]))
        {
            //This will store the new mode and reboot the pico
            user_settings.store_driver_type_safe(user_settings.get_current_driver());
        }
    });
}

void run_program()
{
    UserSettings user_settings;
    user_settings.initialize_flash();

    board_api::init_board();

    if (board_api::uart_bridge_mode())
    {
        run_esp32_uart_bridge();
        return;
    }

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads_[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager::get_instance().initialize_driver(user_settings.get_current_driver(), gamepads_);

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    board_api::reset_esp32();

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check, user_settings);

    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();

    tud_init(BOARD_TUD_RHPORT);

    while (true)
    {
        TaskQueue::Core0::process_tasks();

        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
        {
            device_driver->process(i, gamepads_[i]);
            tud_task();
        }
        
        sleep_us(100);
    }
}

} // namespace OGXMini

#endif // OGXM_BOARD == W_ESP32