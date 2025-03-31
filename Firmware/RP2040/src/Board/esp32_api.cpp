#include "Board/Config.h"
#if defined(CONFIG_EN_ESP32)

#include <pico/stdlib.h>
#include <hardware/gpio.h>

#include "Board/esp32_api.h"

bool esp32_api::uart_bridge_mode()
{
    gpio_pull_up(MODE_SEL_PIN);
    return (gpio_get(MODE_SEL_PIN) == 0);
}  

void esp32_api::reset() 
{
    gpio_put(ESP_RST_PIN, 0);
    sleep_ms(500);
    gpio_put(ESP_RST_PIN, 1);
    sleep_ms(250);
}

void esp32_api::enter_programming_mode() 
{
    gpio_put(ESP_PROG_PIN, 1);
    sleep_ms(250);
    gpio_put(ESP_PROG_PIN, 0);
	sleep_ms(250);

    reset();

	gpio_put(ESP_PROG_PIN, 1);
}

void esp32_api::init()
{
    gpio_init(ESP_PROG_PIN);
    gpio_set_dir(ESP_PROG_PIN, GPIO_OUT);
    gpio_put(ESP_PROG_PIN, 1);

    gpio_init(ESP_RST_PIN);
    gpio_set_dir(ESP_RST_PIN, GPIO_OUT);
    gpio_put(ESP_RST_PIN, 1);

    gpio_init(MODE_SEL_PIN);
    gpio_set_dir(MODE_SEL_PIN, GPIO_IN);
    gpio_pull_up(MODE_SEL_PIN);
}

#endif // defined(CONFIG_EN_ESP32)