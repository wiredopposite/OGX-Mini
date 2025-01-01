#ifndef _OGXM_BOARD_API_H_
#define _OGXM_BOARD_API_H_

#include <cstdint>
#include <string>

namespace board_api
{
    void init_board();
    void set_led(bool state);
    void reboot();

    bool uart_bridge_mode();
    void reset_esp32();
    void enter_esp32_prog_mode();

    uint32_t ms_since_boot();
}

#endif // _OGXM_BOARD_API_H_