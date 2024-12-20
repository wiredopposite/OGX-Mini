#ifndef _BLUEPAD32_DRIVER_H_
#define _BLUEPAD32_DRIVER_H_

#include <cstdint>

namespace BP32
{
    void run_task();
    bool any_connected();
    bool connected(uint8_t index);
}

#endif // _BLUEPAD32_DRIVER_H_