#include "board_config.h"
#if (OGX_TYPE == WIRELESS) && (OGX_MCU == MCU_RP2040)

#include <pico/stdlib.h>
#include "hardware/gpio.h"

#include "usbd/drivers/uartbridge/helpers.h"

bool uart_programming_mode()
{
    gpio_init(MODE_SEL_PIN);
    gpio_set_dir(MODE_SEL_PIN, GPIO_IN);
    gpio_pull_up(MODE_SEL_PIN);

    if (gpio_get(MODE_SEL_PIN) == 0) 
    {
        return true;
    } 

    return false;
}  

void esp32_reset() 
{
    gpio_init(ESP_PROG_PIN);
    gpio_set_dir(ESP_PROG_PIN, GPIO_OUT);
    gpio_put(ESP_PROG_PIN, 1);

    gpio_init(ESP_RST_PIN);
    gpio_set_dir(ESP_RST_PIN, GPIO_OUT);
    gpio_put(ESP_PROG_PIN, 1);

    gpio_put(ESP_PROG_PIN, 0);
	sleep_ms(250);

    gpio_put(ESP_RST_PIN, 0);
    sleep_ms(500);
    gpio_put(ESP_RST_PIN, 1);
	sleep_ms(250);
	gpio_put(ESP_PROG_PIN, 1);
}

#endif