#ifndef _I2CDRIVER_ESP_H_
#define _I2CDRIVER_ESP_H_

#include <cstdint>

#include "Gamepad.h"

namespace I2CDriver
{
    void initialize(Gamepad (&gamepads)[MAX_GAMEPADS]);
}

#endif // _I2CDRIVER_ESP_H_