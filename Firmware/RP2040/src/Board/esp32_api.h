#include "Board/Config.h"
#if defined(CONFIG_EN_ESP32)

#include <cstdint>

namespace esp32_api {
    bool uart_bridge_mode();
    void reset();
    void enter_programming_mode();
    void init();
} // namespace board_api_esp32

#endif // defined(CONFIG_EN_ESP32)