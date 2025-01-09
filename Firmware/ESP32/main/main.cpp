#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "main.h"
#include "sdkconfig.h"
#include "BTManager/BTManager.h"

void cpp_main()
{
    xTaskCreatePinnedToCore(
        [](void* parameter)
        { 
            BTManager::get_instance().run_task(); 
        },
        "bp32",
        2048 * 4,
        NULL,
        configMAX_PRIORITIES-4,
        NULL,
        0 );
}