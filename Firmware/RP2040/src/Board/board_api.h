#ifndef _BOARD_API_H_
#define _BOARD_API_H_

#include <cstdint>

namespace board_api
{
    void init_gpio();
    void set_led(bool state);
    void reboot();

    bool uart_bridge_mode();
    void reset_esp32();
    void enter_esp32_prog_mode();
}

#endif // _BOARD_API_H_