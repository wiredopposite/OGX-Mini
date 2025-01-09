#ifndef _BOARD_API_H_
#define _BOARD_API_H_

#include <cstdint>
#include <driver/gpio.h>

#include "sdkconfig.h"

#define MAX_GPIO_NUM 40

namespace board_api
{
    static constexpr gpio_num_t RESET_PIN = static_cast<gpio_num_t>(CONFIG_RESET_PIN);
    static_assert(RESET_PIN < MAX_GPIO_NUM, "Invalid RESET_PIN");

    static constexpr gpio_num_t LED_PINS[] =
    {
    #if defined(CONFIG_ENABLE_LED_1) && (CONFIG_LED_PIN_1 < MAX_GPIO_NUM) && (CONFIG_BLUEPAD32_MAX_DEVICES > 0)
        static_cast<gpio_num_t>(CONFIG_LED_PIN_1),

    #if defined(CONFIG_ENABLE_LED_2) && (CONFIG_LED_PIN_2 < MAX_GPIO_NUM) && (CONFIG_BLUEPAD32_MAX_DEVICES > 1)
        static_cast<gpio_num_t>(CONFIG_LED_PIN_2),

    #if defined(CONFIG_ENABLE_LED_3) && (CONFIG_LED_PIN_3 < MAX_GPIO_NUM) && (CONFIG_BLUEPAD32_MAX_DEVICES > 2)
        static_cast<gpio_num_t>(CONFIG_LED_PIN_3),
        
    #if defined(CONFIG_ENABLE_LED_4) && (CONFIG_LED_PIN_4 < MAX_GPIO_NUM) && (CONFIG_BLUEPAD32_MAX_DEVICES > 3)
        static_cast<gpio_num_t>(CONFIG_LED_PIN_4),

    #endif // CONFIG_ENABLE_LED_4
    #endif // CONFIG_ENABLE_LED_3
    #endif // CONFIG_ENABLE_LED_2
    #endif // CONFIG_ENABLE_LED_1
    };

    static constexpr uint8_t NUM_LEDS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

    void init_board();
    void set_led(uint8_t index, bool state);
    void set_led(bool state);
}

#endif // _BOARD_API_H_