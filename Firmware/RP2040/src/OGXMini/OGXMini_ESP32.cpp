#include "board_config.h"
#if (OGXM_BOARD == PICO_ESP32)

#include <pico/multicore.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "OGXMini/OGXMini.h"
#include "I2CDriver/ESP32/I2CDriver.h"
#include "Gamepad/Gamepad.h"
#include "TaskQueue/TaskQueue.h"

namespace OGXMini {

static Gamepad gamepads_[MAX_GAMEPADS];

//Not using this for ESP32 currently
void host_mounted(bool host_mounted) { }

void core1_task()
{
    I2CDriver::initialize(gamepads_);

    while (true) 
    {
        tight_loop_contents();
    }
}

void run_uart_bridge(UserSettings& user_settings)
{
    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(DeviceDriverType::UART_BRIDGE, gamepads_);

    board_api::esp32::enter_programming_mode();

    OGXM_LOG("Entering UART Bridge mode\n");
    
    device_manager.get_driver()->process(0, gamepads_[0]); //Runs UART Bridge task, doesn't return unless programming is complete

    OGXM_LOG("Exiting UART Bridge mode\n");

    board_api::usb::disconnect_all(); 
    user_settings.write_datetime();
    board_api::reboot();
}

bool update_needed(UserSettings& user_settings)
{
#if defined(OGXM_ESP32_RETAIL)
    return !user_settings.verify_datetime();
#endif
    return false;
}

void run_program()
{
    board_api::init_board();

    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();

    //MODE_SEL_PIN is used to determine if UART bridge should be run
    if (board_api::esp32::uart_bridge_mode() || update_needed(user_settings))
    {
        run_uart_bridge(user_settings);
        return;
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver(), gamepads_);

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    // board_api::esp32::reset();

    DeviceDriver* device_driver = device_manager.get_driver();

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