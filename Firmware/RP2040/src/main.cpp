#include <cstdint>
#include <hardware/clocks.h>

#include "OGXMini/OGXMini.h"
#include "board_config.h"

int main()
{
    if (set_sys_clock_hz(SYSCLOCK_HZ*1000, true))
    {
        OGXMini::run_program();
    }
}