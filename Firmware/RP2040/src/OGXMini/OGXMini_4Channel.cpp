#include "board_config.h"
#if OGXM_BOARD == INTERNAL_4CH || OGXM_BOARD == EXTERNAL_4CH

#include <pico/multicore.h>

#include "tusb.h"
#include "bsp/board_api.h"
#include "pio_usb.h"

#include "USBDevice/DeviceManager.h"
#include "USBHost/HostManager.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"
#include "OGXMini/OGXMini.h"
#include "I2CDriver/4Channel/I2CManager.h"
#include "Gamepad.h"
#include "TaskQueue/TaskQueue.h"

namespace OGXMini {

Gamepad gamepads_[MAX_GAMEPADS];

//Called by tusb host or i2c driver so we know to connect or disconnect usb
void host_mounted(bool host_mounted)
{
    static std::atomic<bool> tud_is_inited = false;

    board_api::set_led(host_mounted);

    if (!host_mounted && tud_is_inited.load())
    {
        TaskQueue::Core0::queue_task([]()
        {
            OGXM_LOG("Disconnecting USB and rebooting.\n");
            board_api::usb::disconnect_all();
            board_api::reboot();
        });
    }
    else if (!tud_is_inited.load() && host_mounted)
    {
        TaskQueue::Core0::queue_task([]() 
        { 
            OGXM_LOG("Initializing USB stack.\n");
            tud_init(BOARD_TUD_RHPORT); 
            tud_is_inited.store(true);
        });
    }
}

void core1_task()
{
    HostManager& host_manager = HostManager::get_instance();
    host_manager.initialize(gamepads_);

    //Pico-PIO-USB will not reliably detect a hot plug on some boards, so monitor and init host stack after connection
    while(!board_api::usb::host_connected())
    {
        sleep_ms(100);
    }

    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tuh_init(BOARD_TUH_RHPORT);

    uint32_t tid_feedback = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(tid_feedback, FEEDBACK_DELAY_MS, true, [&host_manager]
    {
        host_manager.send_feedback();
    });

    while (true) 
    {
        TaskQueue::Core1::process_tasks();
        tuh_task();
    }
}

void set_gp_check_timer(uint32_t task_id, UserSettings& user_settings)
{
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true, 
        [&user_settings]
        {
            //Check gamepad inputs for button combo to change usb device driver
            if (user_settings.check_for_driver_change(gamepads_[0]))
            {
                //This will store the new mode and reboot the pico
                user_settings.store_driver_type(user_settings.get_current_driver());
            }
        });
}

void run_program()
{
    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();

    board_api::init_board();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads_[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver(), gamepads_);
    
    I2CManager& i2c_manager = I2CManager::get_instance();
    i2c_manager.initialize_driver();
    
    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    //Wait for something to call tud_init
    while (!tud_inited())
    {
        TaskQueue::Core0::process_tasks();
        sleep_ms(10);
    }

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check, user_settings);

    DeviceDriver* device_driver = device_manager.get_driver();
    I2CDriver* i2c_driver = i2c_manager.get_driver();

    while (true)
    {
        TaskQueue::Core0::process_tasks();

        i2c_driver->process(gamepads_);
        device_driver->process(0, gamepads_[0]);

        tud_task();
        sleep_us(100);
    }
}

} // namespace OGXMini

#endif // OGXM_BOARD == INTERNAL_4CH || OGXM_BOARD == EXTERNAL_4CH