#include <stdlib.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "sdkconfig.h"
#include "c_wrapper.h"

#define STACK_MULTIPLIER (2048 * CONFIG_BLUEPAD32_MAX_DEVICES)

void app_main(void) 
{
    xTaskCreatePinnedToCore(
        run_bluepad32,
        "bp32",
        STACK_MULTIPLIER * 4,
        NULL,
        configMAX_PRIORITIES-6,
        NULL,
        0 );

    xTaskCreatePinnedToCore(
        run_i2c,
        "i2c",
        STACK_MULTIPLIER * 2,
        NULL,
        configMAX_PRIORITIES-8,
        NULL,
        1 );
}   