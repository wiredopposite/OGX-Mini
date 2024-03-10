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

#include "input_mode.h"

Gamepad gamepad;
GamepadOut gamepadOut;

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

    Gamepad previous_gamepad = gamepad;
    absolute_time_t last_time_gamepad_changed = get_absolute_time();
    absolute_time_t last_time_gamepad_checked = get_absolute_time();

    while (1) 
    {
        uint8_t outBuffer[64];
        GPDriver* driver = driverManager.getDriver();
        driver->process(&gamepad, outBuffer);

        if (absolute_time_diff_us(last_time_gamepad_checked, get_absolute_time()) >= 200000) 
        {
            // check if digital buttons have changed (first 16 bytes of gamepad.state)
            if (memcmp(&gamepad.state, &previous_gamepad.state, 16) != 0)
            {
                memcpy(&previous_gamepad.state, &gamepad.state, sizeof(gamepad.state));
                last_time_gamepad_changed = get_absolute_time();
            }
            // haven't changed for 3 seconds
            else if (absolute_time_diff_us(last_time_gamepad_changed, get_absolute_time()) >= 3000000) 
            {
                change_input_mode(previous_gamepad);
                last_time_gamepad_changed = get_absolute_time();
            }

            last_time_gamepad_checked = get_absolute_time();
        }

        sleep_ms(1);
        tud_task();
    }

    return 0;
}