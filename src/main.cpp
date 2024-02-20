#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "tusb.h"
#include "bsp/board_api.h"

#include "usbh/tusb_host.h"

#include "usbd/drivers/drivermanager.h"
#include "usbd/drivers/gpdriver.h"

#include "Gamepad.h"
#include "input_mode.h"

Gamepad gamepad;
GamepadOut gamepadOut;

int main(void) 
{
    set_sys_clock_khz(120000, true);

    board_init();

    InputMode mode = load_input_mode();

    DriverManager& driverManager = DriverManager::getInstance();
    driverManager.setup(mode);

    // usb host on core 1
    multicore_reset_core1();
    multicore_launch_core1(usbh_main);

    while (1) 
    {
        uint8_t outBuffer[64];
        GPDriver* driver = driverManager.getDriver();
        driver->process(&gamepad, outBuffer);

        sleep_ms(1);
        tud_task();
    }

    return 0;
}