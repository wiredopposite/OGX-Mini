#include "board_config.h"
#if (OGXM_BOARD == PI_PICOW)

#include <pico/stdlib.h>
#include <hardware/sync.h>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <pico/cyw43_arch.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "Bluepad32/Bluepad32.h"
#include "OGXMini/OGXMini.h"

namespace OGXMini {

Gamepad gamepads_[MAX_GAMEPADS];

void core1_task()
{
    if (cyw43_arch_init() != 0)
    {
        return;
    }

    bluepad32::run_task(gamepads_);
}

bool gp_check_cb(repeating_timer_t* rt)
{
    GPCheckContext* gp_check_ctx = static_cast<GPCheckContext*>(rt->user_data);
    gp_check_ctx->driver_changed = gp_check_ctx->user_settings.check_for_driver_change(gamepads_[0]);
    return true;
}

void run_program()
{
    UserSettings user_settings;
    user_settings.initialize_flash();

    board_init();
    board_api::init_gpio();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads_[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver());

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    GPCheckContext gp_check_ctx = { false, user_settings };
    repeating_timer_t gp_check_timer;
    add_repeating_timer_ms(UserSettings::GP_CHECK_DELAY_MS, gp_check_cb, &gp_check_ctx, &gp_check_timer);

    DeviceDriver* device_driver = device_manager.get_driver();

    tud_init(BOARD_TUD_RHPORT);

    while (true)
    {
        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
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

#endif // (OGXM_BOARD == PI_PICOW)
