#include "board_config.h"
#if (OGXM_BOARD == ADA_FEATHER) || (OGXM_BOARD == RP_ZERO) || (OGXM_BOARD == PI_PICO) || (OGXM_BOARD == PI_PICO2)

#include <pico/multicore.h>

#include "tusb.h"
#include "bsp/board_api.h"
#include "pio_usb.h"

#include "USBHost/HostManager.h"
#include "USBDevice/DeviceManager.h"
#include "OGXMini/OGXMini.h"
#include "TaskQueue/TaskQueue.h"
#include "Gamepad.h"
#include "Board/board_api.h"

namespace OGXMini {

Gamepad gamepads_[MAX_GAMEPADS];
std::atomic<bool> tud_inited_{false};

void core1_task()
{
    HostManager& host_manager = HostManager::get_instance();
    host_manager.initialize(gamepads_);

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

//Called by tusb host or i2c driver so we know to connect or disconnect usb
void update_tud_status(bool host_mounted)
{
    board_api::set_led(host_mounted);

    if (!host_mounted && tud_inited_.load())
    {
        TaskQueue::Core0::queue_task([]()
        {
            tud_disconnect();
            sleep_ms(300);
            multicore_reset_core1();
            sleep_ms(300);
            board_api::reboot();
        });
    }
    else if (!tud_inited_.load())
    {
        TaskQueue::Core0::queue_task([]()
        {
            tud_init(BOARD_TUD_RHPORT);
        });
    }
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

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver(), gamepads_);

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check, user_settings);

    DeviceDriver* device_driver = device_manager.get_driver();

    // Wait for something to call update_tud_status()
    while (!tud_inited())
    {
        TaskQueue::Core0::process_tasks();
        sleep_ms(100);
    }

    tud_inited_.store(true);

    while (true)
    {
        TaskQueue::Core0::process_tasks();

        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
        {
            device_driver->process(i, gamepads_[i]);
        }

        tud_task();
        sleep_us(100);
    }
}

} // namespace OGXMini

#endif // (OGXM_BOARD == ADA_FEATHER) || (OGXM_BOARD == RP_ZERO) || (OGXM_BOARD == PI_PICO) || (OGXM_BOARD == PI_PICO2)