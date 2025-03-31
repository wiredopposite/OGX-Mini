#include "Board/Config.h"
#if defined(CONFIG_EN_BLUETOOTH)

#include <atomic>
#include <pico/cyw43_arch.h>

#include "Board/board_api_private/board_api_private.h"

#if defined(CYW43_WL_GPIO_LED_PIN) && defined(LED_INDICATOR_PIN)
static_assert(CYW43_WL_GPIO_LED_PIN != LED_INDICATOR_PIN, "CYW43_WL_GPIO_LED_PIN cannot be the same as LED_INDICATOR_PIN");
#endif

namespace board_api_bt {

std::atomic<bool> inited{false};

void init() {
    if (cyw43_arch_init() != 0) {  
        panic("CYW43 init failed");
    } else {
        inited.store(true);
    }
}

void set_led(bool state) {
    if (!inited.load()) {
        return;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state ? 1 : 0);
}

} // namespace board_api_bt

#endif // defined(CONFIG_EN_BLUETOOTH)