#include "Board/Config.h"
#if defined(CONFIG_EN_RGB)

#include <hardware/gpio.h>

#include "Board/Pico_WS2812/WS2812.hpp"
#include "Board/board_api_private/board_api_private.h"

namespace board_api_rgb {

WS2812& get_ws2812() {
    static WS2812 ws2812 = WS2812(RGB_PXL_PIN, 1, pio1, 0, WS2812::FORMAT_GRB);
    return ws2812;
}

void init() {
#if defined(RGB_PWR_PIN)
    gpio_init(RGB_PWR_PIN);
    gpio_set_dir(RGB_PWR_PIN, GPIO_OUT);
    gpio_put(RGB_PWR_PIN, 1);
#endif

    set_led(0xFF, 0, 0);
}

void set_led(uint8_t r, uint8_t g, uint8_t b) {
    get_ws2812().setPixelColor(0, WS2812::RGB(r, g, b));
    get_ws2812().show();
}

} // namespace board_api_rgb

#endif // defined(CONFIG_EN_RGB)