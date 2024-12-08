#include "board_config.h"
#if OGXM_BOARD == W_ESP32

#include <array>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/sync.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "OGXMini/OGXMini.h"
#include "I2CDriver/I2CDriver_ESP.h"
#include "Gamepad.h"

namespace OGXMini {

std::array<Gamepad, MAX_GAMEPADS> gamepads_;

// void init_esp32_gpio()
// {
//     gpio_init(ESP_PROG_PIN);
//     gpio_set_dir(ESP_PROG_PIN, GPIO_IN);
//     gpio_pull_up(ESP_PROG_PIN);

//     gpio_init(ESP_RST_PIN);
//     gpio_set_dir(ESP_RST_PIN, GPIO_IN);
//     gpio_pull_up(ESP_RST_PIN);
// }

// bool uart_passthrough_mode()
// {
//     gpio_init(MODE_SEL_PIN);
//     gpio_set_dir(MODE_SEL_PIN, GPIO_IN);
//     gpio_pull_up(MODE_SEL_PIN);

//     if (gpio_get(MODE_SEL_PIN) == 0) 
//     {
//         return true;
//     } 
//     return false;
// }  

// void reset_esp32() 
// {
//     gpio_init(ESP_RST_PIN);
//     gpio_set_dir(ESP_RST_PIN, GPIO_OUT);
//     gpio_put(ESP_RST_PIN, 0);
//     sleep_ms(500);
//     gpio_put(ESP_RST_PIN, 1);
// 	sleep_ms(250);
// }

// void reset_esp32() 
// {
//     gpio_init(ESP_PROG_PIN);
//     gpio_set_dir(ESP_PROG_PIN, GPIO_OUT);
//     gpio_put(ESP_PROG_PIN, 1);

//     gpio_init(ESP_RST_PIN);
//     gpio_set_dir(ESP_RST_PIN, GPIO_OUT);
//     gpio_put(ESP_PROG_PIN, 1);

//     gpio_put(ESP_PROG_PIN, 0);
// 	sleep_ms(250);

//     gpio_put(ESP_RST_PIN, 0);
//     sleep_ms(500);
//     gpio_put(ESP_RST_PIN, 1);
// 	sleep_ms(250);
// 	gpio_put(ESP_PROG_PIN, 1);
// }

void run_esp32_uart_bridge()
{
    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(DeviceDriver::Type::UART_BRIDGE);
    board_api::enter_esp32_prog_mode();
    // reset_esp32();
    device_manager.get_driver()->process(0, gamepads_.front()); //Runs UART Bridge task
}

void core1_task()
{
    i2c_driver_esp::initialize(gamepads_);

    while (true) 
    {
        __wfi();
    }
}

bool gp_check_cb(repeating_timer_t* rt)
{
    GPCheckContext* gp_check_ctx = static_cast<GPCheckContext*>(rt->user_data);
    gp_check_ctx->driver_changed = gp_check_ctx->user_settings.check_for_driver_change(gamepads_.front());
    return true;
}

void run_program()
{
    UserSettings user_settings;
    user_settings.initialize_flash();

    board_api::init_gpio();

    // init_esp32_gpio();

    if (board_api::uart_bridge_mode())
    // if (uart_passthrough_mode())
    {
        run_esp32_uart_bridge();
        return;
    }
    // else if (!user_settings.verify_firmware_version())
    // {
    //     user_settings.write_firmware_version();
    // }
    
    // user_settings.initialize_flash();

    board_init();

    for (uint8_t i = 0; i < gamepads_.size(); ++i)
    {
        gamepads_[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    // device_manager.initialize_driver(user_settings.get_current_driver());
    device_manager.initialize_driver(DeviceDriver::Type::XINPUT);

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    board_api::reset_esp32();
    // reset_esp32();

    // GPCheckContext gp_check_ctx = { false, user_settings };
    // repeating_timer_t gp_check_timer;
    // add_repeating_timer_ms(UserSettings::GP_CHECK_DELAY_MS, gp_check_cb, &gp_check_ctx, &gp_check_timer);

    DeviceDriver* device_driver = device_manager.get_driver();

    tud_init(BOARD_TUD_RHPORT);

    while (true)
    {
        for (uint8_t i = 0; i < gamepads_.size(); ++i)
        {
            device_driver->process(i, gamepads_[i]);
            tud_task();
        }
        // device_driver->process(0, gamepads_.front());
        // tud_task();
        
        sleep_us(100);

        // if (gp_check_ctx.driver_changed)
        // {
        //     cancel_repeating_timer(&gp_check_timer);
        //     user_settings.store_driver_type_safe(user_settings.get_current_driver());
        // }
    }
}

} // namespace OGXMini

#endif // OGXM_BOARD == W_ESP32