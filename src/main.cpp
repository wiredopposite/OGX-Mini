#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "tusb.h"
#include "bsp/board_api.h"

#include "usbh/tusb_host.h"

#include "usbd/drivermanager.h"
#include "usbd/gpdriver.h"

#include "Gamepad.h"
#include "input_mode.h"
#include "board_config.h"

int main(void) 
{
    set_sys_clock_khz(120000, true);

    board_init();

    InputMode mode = get_input_mode();

    DriverManager& driverManager = DriverManager::getInstance();
    driverManager.setup(mode);

    // usb host on core 1
    multicore_reset_core1();
    multicore_launch_core1(usbh_main);

    GamepadButtons prev_gamepad_buttons = gamepad(0).buttons;
    absolute_time_t last_time_gamepad_changed = get_absolute_time();
    absolute_time_t last_time_gamepad_checked = get_absolute_time();

    while (1) 
    {
        for (int i = 0; i < MAX_GAMEPADS; i++)
        {
            uint8_t outBuffer[64];
            GPDriver* driver = driverManager.getDriver();
            driver->process(i, &gamepad(i), outBuffer);
            driver->update_rumble(i, &gamepad(i));
        }

        if (absolute_time_diff_us(last_time_gamepad_checked, get_absolute_time()) >= 500000) 
        {
            // check if digital buttons have changed
            if (memcmp(&gamepad(0).buttons, &prev_gamepad_buttons, sizeof(GamepadButtons)) != 0)
            {
                memcpy(&prev_gamepad_buttons, &gamepad(0).buttons, sizeof(GamepadButtons));
                last_time_gamepad_changed = get_absolute_time();
            }
            // haven't changed for 3 seconds
            else if (absolute_time_diff_us(last_time_gamepad_changed, get_absolute_time()) >= 3000000) 
            {
                if (!change_input_mode(prev_gamepad_buttons))
                {
                    last_time_gamepad_changed = get_absolute_time();
                }
            }

            last_time_gamepad_checked = get_absolute_time();
        }

        sleep_ms(1);
        tud_task();
    }

    return 0;
}