#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "c_wrapper.h"
#include "sdkconfig.h"
#include "Bluepad32/Bluepad32.h"

void bp32_run_task(void* parameter)
{
    BP32::run_task();
}

void cpp_main()
{
    xTaskCreatePinnedToCore(
        bp32_run_task,
        "bp32",
        2048 * 4,
        NULL,
        configMAX_PRIORITIES-4,
        NULL,
        0 );
}