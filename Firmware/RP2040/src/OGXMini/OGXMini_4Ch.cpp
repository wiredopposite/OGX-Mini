#include "board_config.h"
#if OGXM_BOARD == INTERNAL_4CH || OGXM_BOARD == EXTERNAL_4CH

#include <array>
#include <pico/multicore.h>

#include "tusb.h"
#include "bsp/board_api.h"
#include "pio_usb.h"

#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "OGXMini/OGXMini.h"
#include "I2CDriver/i2c_driver_4ch.h"
#include "Gamepad.h"

namespace OGXMini {

std::array<Gamepad, MAX_GAMEPADS> gamepads_;

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

bool tud_status_cb(repeating_timer_t* rt)
{
    TUDStatusContext* tud_status_ctx = static_cast<TUDStatusContext*>(rt->user_data);
    bool host_mounted = HostManager::get_instance().mounted() || i2c_driver::is_active();
    bool device_ininted = tud_inited();
    board_api::set_led(host_mounted);

    if (host_mounted && !device_ininted)
    {
        tud_status_ctx->status = TUDStatus::INIT;
    }
    else if (!host_mounted && device_ininted)
    {
        tud_status_ctx->status = TUDStatus::DEINIT;
    }
    else
    {
        tud_status_ctx->status = TUDStatus::NOCHANGE;
    }

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

    i2c_driver::initialize(gamepads_);

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    GPCheckContext gp_check_ctx = { false, user_settings };
    repeating_timer_t gp_check_timer;
    add_repeating_timer_ms(UserSettings::GP_CHECK_DELAY_MS, gp_check_cb, &gp_check_ctx, &gp_check_timer);

    TUDStatusContext tud_status_ctx = { TUDStatus::NOCHANGE, &HostManager::get_instance() };
    repeating_timer_t tud_status_timer;
    add_repeating_timer_ms(500, tud_status_cb, &tud_status_ctx, &tud_status_timer);

    DeviceDriver* device_driver = device_manager.get_driver();

    while (true)
    {
        switch (tud_status_ctx.status)
        {
            case TUDStatus::NOCHANGE:
                break;
            case TUDStatus::INIT:
                tud_init(BOARD_TUD_RHPORT);
                break;
            case TUDStatus::DEINIT:
                tud_disconnect();
                sleep_ms(300);
                board_api::reboot();
                break;
        }

        device_driver->process(0, gamepads_[0]);
        tud_task();
        sleep_us(100);
        i2c_driver::process();

        if (gp_check_ctx.driver_changed)
        {
            cancel_repeating_timer(&gp_check_timer);
            user_settings.store_driver_type_safe(user_settings.get_current_driver());
        }
    }
}

} // namespace OGXMini

#endif // OGXM_BOARD == INTERNAL_4CH || OGXM_BOARD == EXTERNAL_4CH