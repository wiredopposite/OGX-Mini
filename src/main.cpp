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

Gamepad gamepad[MAX_GAMEPADS];
GamepadOut gamepad_out[MAX_GAMEPADS];

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

    Gamepad previous_gamepad = gamepad[0];
    absolute_time_t last_time_gamepad_changed = get_absolute_time();
    absolute_time_t last_time_gamepad_checked = get_absolute_time();

    while (1) 
    {
        for (int i = 0; i < MAX_GAMEPADS; i++)
        {
            uint8_t outBuffer[64];
            GPDriver* driver = driverManager.getDriver();
            driver->process((uint8_t)i, &gamepad[i], outBuffer);
            driver->update_rumble((uint8_t)i, &gamepad_out[i]);
        }

        if (absolute_time_diff_us(last_time_gamepad_checked, get_absolute_time()) >= 500000) 
        {
            // check if digital buttons have changed (first 16 bytes of gamepad.state)
            if (memcmp(&gamepad[0].state, &previous_gamepad.state, 16) != 0)
            {
                memcpy(&previous_gamepad.state, &gamepad[0].state, sizeof(gamepad[0].state));
                last_time_gamepad_changed = get_absolute_time();
            }
            // haven't changed for 3 seconds
            else if (absolute_time_diff_us(last_time_gamepad_changed, get_absolute_time()) >= 3000000) 
            {
                if (!change_input_mode(previous_gamepad))
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