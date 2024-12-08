#include <pico/stdlib.h>

#include "OGXMini/OGXMini.h"
#include "board_config.h"

int main()
{
    set_sys_clock_khz(250000, true);
    OGXMini::run_program();
}