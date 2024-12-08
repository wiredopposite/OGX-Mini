#include "board_config.h"
#if (OGXM_BOARD == ADA_FEATHER) || (OGXM_BOARD == RP_ZERO) || (OGXM_BOARD == PI_PICO)

#include <array>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>

#include "tusb.h"
#include "bsp/board_api.h"
#include "pio_usb.h"

#include "USBHost/HostManager.h"
#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "OGXMini/OGXMini.h"
#include "Gamepad.h"

namespace OGXMini {

std::atomic<TUDStatus> tud_status_ = TUDStatus::NOCHANGE;
std::array<Gamepad, MAX_GAMEPADS> gamepads_;

void update_tuh_status(bool mounted)
{
    if (mounted)
    {
        tud_status_.store(TUDStatus::INIT);
        board_api::set_led(true);
    }
    else
    {
        tud_status_.store(TUDStatus::DEINIT);
        board_api::set_led(false);
    }
}

bool feedback_cb(repeating_timer_t* rt)
{
    static_cast<HostManager*>(rt->user_data)->send_feedback();
    return true;
}

void core1_task()
{
    HostManager& host_manager = HostManager::get_instance();
    host_manager.initialize(gamepads_);

    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tuh_init(BOARD_TUH_RHPORT);

    repeating_timer_t feedback_timer;
    add_repeating_timer_ms(FEEDBACK_DELAY_MS, feedback_cb, &host_manager, &feedback_timer);

    while (true) 
    {
        tuh_task();
        sleep_us(100);
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

    board_init();

    board_api::init_gpio();
    board_api::set_led(false);

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

    while (true)
    {
        switch (tud_status_.load())
        {
            case TUDStatus::NOCHANGE:
                break;
            case TUDStatus::INIT:
                tud_init(BOARD_TUD_RHPORT);
                tud_status_.store(TUDStatus::NOCHANGE);
                break;
            case TUDStatus::DEINIT:
                tud_disconnect();
                sleep_ms(300);
                multicore_reset_core1();
                sleep_ms(300);
                board_api::reboot();
                break;
        }

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

#endif // (OGXM_BOARD == ADA_FEATHER) || (OGXM_BOARD == RP_ZERO) || (OGXM_BOARD == PI_PICO)