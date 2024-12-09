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
#include "I2CDriver/i2c_driver_esp32.h"
#include "Gamepad.h"

namespace OGXMini {

std::array<Gamepad, MAX_GAMEPADS> gamepads_;

void run_esp32_uart_bridge()
{
    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(DeviceDriver::Type::UART_BRIDGE);
    board_api::enter_esp32_prog_mode();
    device_manager.get_driver()->process(0, gamepads_.front()); //Runs UART Bridge task
}

void core1_task()
{
    i2c_driver_esp32::initialize(gamepads_);

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

    if (board_api::uart_bridge_mode())
    {
        run_esp32_uart_bridge();
        return;
    }
    // else if (!user_settings.verify_firmware_version())
    // {
    //     user_settings.write_firmware_version();
    // }

    board_init();

    for (uint8_t i = 0; i < gamepads_.size(); ++i)
    {
        gamepads_[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver());

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    board_api::reset_esp32();

    GPCheckContext gp_check_ctx = { false, user_settings };
    repeating_timer_t gp_check_timer;
    add_repeating_timer_ms(UserSettings::GP_CHECK_DELAY_MS, gp_check_cb, &gp_check_ctx, &gp_check_timer);

    DeviceDriver* device_driver = device_manager.get_driver();

    tud_init(BOARD_TUD_RHPORT);

    while (true)
    {
        for (uint8_t i = 0; i < gamepads_.size(); ++i)
        {
            device_driver->process(i, gamepads_[i]);
            tud_task();
        }
        
        sleep_us(100);

        if (gp_check_ctx.driver_changed)
        {
            cancel_repeating_timer(&gp_check_timer);
            user_settings.store_driver_type_safe(user_settings.get_current_driver());
        }
    }
}

} // namespace OGXMini

#endif // OGXM_BOARD == W_ESP32