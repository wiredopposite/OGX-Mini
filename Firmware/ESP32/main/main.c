#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "log/log.h"
#include "bluetooth/bluetooth.h"
#include "wired/wired.h"

void app_main(void) {
    ogxm_log_init(true);
    bluetooth_config(wired_set_connected, wired_set_pad, wired_set_audio);
    wired_config(bluetooth_set_rumble, bluetooth_set_audio);

    xTaskCreatePinnedToCore(
        wired_task,
        "wired",
        2048U * 2U,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        bluetooth_task,
        "bluetooth",
        2048U * 4U,
        NULL,
        configMAX_PRIORITIES - 4,
        NULL,
        0
    );
}