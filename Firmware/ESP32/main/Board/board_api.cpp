#include <driver/gpio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "sdkconfig.h"
#include "board/board_api.h"

namespace board_api {

SemaphoreHandle_t leds_mutex_ = nullptr;
// SemaphoreHandle_t reset_mutex_ = nullptr;

void init_board()
{
    if (leds_mutex_ == nullptr)
    {
        leds_mutex_ = xSemaphoreCreateMutex();
    }
    // if (reset_mutex_ == nullptr)
    // {
    //     reset_mutex_ = xSemaphoreCreateMutex();
    // }

    if (xSemaphoreTake(leds_mutex_, portMAX_DELAY))
    {
        for (const auto& LED_PIN : LED_PINS)
        {
            gpio_reset_pin(LED_PIN);
            gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(LED_PIN, 0);
        }
        xSemaphoreGive(leds_mutex_);
    }
    // if (xSemaphoreTake(reset_mutex_, portMAX_DELAY))
    // {
    //     gpio_reset_pin(RESET_PIN);
    //     gpio_set_direction(RESET_PIN, GPIO_MODE_INPUT);
    //     gpio_set_level(RESET_PIN, 1);
    //     xSemaphoreGive(reset_mutex_);
    // }
}

//Set LED by index
void set_led(uint8_t index, bool state)
{
    if constexpr (NUM_LEDS < 1)
    {
        return;
    }
    if (index >= NUM_LEDS)
    {
        return;
    }
    if (xSemaphoreTake(leds_mutex_, portMAX_DELAY))
    {
        gpio_set_level(LED_PINS[index], state ? 1 : 0);
        xSemaphoreGive(leds_mutex_);
    }
}

//Set first LED
void set_led(bool state)
{
    if constexpr (NUM_LEDS < 1)
    {
        return;
    }
    if (xSemaphoreTake(leds_mutex_, portMAX_DELAY))
    {
        gpio_set_level(LED_PINS[0], state ? 1 : 0);
        xSemaphoreGive(leds_mutex_);
    }
}

} // namespace board_api