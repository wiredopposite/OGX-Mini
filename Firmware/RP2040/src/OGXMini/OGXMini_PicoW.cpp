#include "board_config.h"
#if (OGXM_BOARD == PI_PICOW)

#include <pico/multicore.h>
#include <pico/cyw43_arch.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "Bluepad32/Bluepad32.h"
#include "OGXMini/OGXMini.h"
#include "Gamepad.h"
#include "TaskQueue/TaskQueue.h"

namespace OGXMini {

Gamepad gamepads_[MAX_GAMEPADS];

//Not using this for Pico W currently
void update_tud_status(bool host_mounted) { }

void core1_task()
{
    if (cyw43_arch_init() != 0)
    {  
        panic("CYW43 init failed");
    }

    //Doesn't return, don't do anything with core1 unless it's executing within the BTStack loop
    bluepad32::run_task(gamepads_);
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

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads_[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager::get_instance().initialize_driver(user_settings.get_current_driver(), gamepads_);

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

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

#endif // (OGXM_BOARD == PI_PICOW)
