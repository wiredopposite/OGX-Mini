#include "Board/Config.h"
#if defined(LED_INDICATOR_PIN)

#include <hardware/gpio.h>

namespace board_api_led {
    
void init() { 
    gpio_init(LED_INDICATOR_PIN);
    gpio_set_dir(LED_INDICATOR_PIN, GPIO_OUT);
    gpio_put(LED_INDICATOR_PIN, 0);
}

void set_led(bool state) {
    gpio_put(LED_INDICATOR_PIN, state ? 1 : 0);
}

} // namespace board_api

#endif // LED_INDICATOR_PIN