#include <hal/gpio_ll.h>
#include "sdkconfig.h"
#include "led/led.h"
#include "gamepad/gamepad.h"

static const int LED_PINS[] = {
#if defined(CONFIG_ENABLE_LED_1) && (GAMEPADS_MAX > 0)
    CONFIG_LED_PIN_1,

#if defined(CONFIG_ENABLE_LED_2) && (GAMEPADS_MAX > 1)
    CONFIG_LED_PIN_2,

#if defined(CONFIG_ENABLE_LED_3) && (GAMEPADS_MAX > 2)
    CONFIG_LED_PIN_3,
    
#if defined(CONFIG_ENABLE_LED_4) && (GAMEPADS_MAX > 3)
    CONFIG_LED_PIN_4,

#endif // CONFIG_ENABLE_LED_4
#endif // CONFIG_ENABLE_LED_3
#endif // CONFIG_ENABLE_LED_2
#endif // CONFIG_ENABLE_LED_1
};

#define NUM_LEDS (sizeof(LED_PINS) / sizeof(LED_PINS[0]))

void led_init(void) {
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        gpio_ll_output_enable(&GPIO, LED_PINS[i]);
        gpio_ll_set_level(&GPIO, LED_PINS[i], 0);
    }
}

void led_set(uint8_t index, bool state) {
    if (NUM_LEDS == 0) {
        return;
    }
    if (index >= NUM_LEDS) {
        index = NUM_LEDS - 1;
    }
    gpio_ll_set_level(&GPIO, LED_PINS[index], state ? 1 : 0);
}