#include "Board/Config.h"
#include "OGXMini/Board/Standard.h"
#if ((OGXM_BOARD == PI_PICO) || (OGXM_BOARD == RP2040_ZERO) || (OGXM_BOARD == ADAFRUIT_FEATHER))

#include <pico/multicore.h>

#include "tusb.h"
#include "bsp/board_api.h"
#include "pio_usb.h"

#include "USBHost/HostManager.h"
#include "USBDevice/DeviceManager.h"
#include "TaskQueue/TaskQueue.h"
#include "Gamepad/Gamepad.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"

constexpr uint32_t FEEDBACK_DELAY_MS = 200;

Gamepad _gamepads[MAX_GAMEPADS];

void core1_task() {
    HostManager& host_manager = HostManager::get_instance();
    host_manager.initialize(_gamepads);

    //Pico-PIO-USB will not reliably detect a hot plug on some boards, 
    //monitor and init host stack after connection
    while(!board_api::usb::host_connected()) {
        sleep_ms(100);
    }

    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tuh_init(BOARD_TUH_RHPORT);

    uint32_t tid_feedback = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(tid_feedback, FEEDBACK_DELAY_MS, true, 
    [&host_manager] {
        host_manager.send_feedback();
    });

    while (true) {
        TaskQueue::Core1::process_tasks();
        tuh_task();
    }
}

void set_gp_check_timer(uint32_t task_id) {
    UserSettings& user_settings = UserSettings::get_instance();
    
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true, 
    [&user_settings] {
        //Check gamepad inputs for button combo to change usb device driver
        if (user_settings.check_for_driver_change(_gamepads[0])) {
            OGXM_LOG("Driver change detected, storing new driver.\n");
            //This will store the new mode and reboot the pico
            user_settings.store_driver_type(user_settings.get_current_driver());
        }
    });
}

//Called by tusb host so we know to connect or disconnect usb
void standard::host_mounted(bool host_mounted) {
    static std::atomic<bool> tud_is_inited = false;
    board_api::set_led(host_mounted);

    if (!host_mounted && tud_is_inited.load()) {
        TaskQueue::Core0::queue_task([]() {
            OGXM_LOG("USB disconnected, rebooting.\n");
            board_api::usb::disconnect_all();
            board_api::reboot();
        });
    } else if (!tud_is_inited.load()) {
        TaskQueue::Core0::queue_task([]() { 
            tud_init(BOARD_TUD_RHPORT); 
            tud_is_inited.store(true);
        });
    }
}

void standard::initialize() {
    board_api::init_board();

    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
        _gamepads[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager::get_instance().initialize_driver(user_settings.get_current_driver(), _gamepads);
}

void standard::run() {
    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    DeviceDriverType current_driver = UserSettings::get_instance().get_current_driver();

    if (current_driver != DeviceDriverType::WEBAPP) {
        // Wait for something to call host_mounted()
        while (!tud_inited()) {
            TaskQueue::Core0::process_tasks();
            sleep_ms(100);
        }
    } else {
        //Connect immediately in WebApp mode 
        host_mounted(true);
    }

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check);

    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();

    while (true) {
        TaskQueue::Core0::process_tasks();

        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
            device_driver->process(i, _gamepads[i]);
        }
        tud_task();
        sleep_ms(1);
    }
}

// #else // OGXM_BOARD == PI_PICO || OGXM_BOARD == RP2040_ZERO || OGXM_BOARD == ADAFRUIT_FEATHER

// void standard::host_mounted(bool host_mounted) {}
// void standard::initialize() {}
// void standard::run() {}

#endif // OGXM_BOARD == PI_PICO || OGXM_BOARD == RP2040_ZERO || OGXM_BOARD == ADAFRUIT_FEATHER