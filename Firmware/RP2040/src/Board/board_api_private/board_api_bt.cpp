#include "Board/Config.h"
#if defined(CONFIG_EN_BLUETOOTH)

#include <atomic>
#include <pico/cyw43_arch.h>

#include "Board/board_api_private/board_api_private.h"
#include "Board/ogxm_log.h"

#if defined(CYW43_WL_GPIO_LED_PIN) && defined(LED_INDICATOR_PIN)
static_assert(CYW43_WL_GPIO_LED_PIN != LED_INDICATOR_PIN, "CYW43_WL_GPIO_LED_PIN cannot be the same as LED_INDICATOR_PIN");
#endif

namespace board_api_bt {

std::atomic<bool> inited{false};

void init() {
    if (cyw43_arch_init() != 0) {
        OGXM_LOG("board_api_bt: CYW43 init failed\n");
        panic("CYW43 init failed");
    } else {
        inited.store(true);
        OGXM_LOG("board_api_bt: CYW43 init OK\n");
    }
}

void set_led(bool state) {
    if (!inited.load()) {
        OGXM_LOG("board_api_bt: set_led skipped (not inited)\n");
        return;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state ? 1 : 0);
}

} // namespace board_api_bt

#endif // defined(CONFIG_EN_BLUETOOTH)