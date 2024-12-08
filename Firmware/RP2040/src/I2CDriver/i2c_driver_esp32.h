#ifndef _I2CDRIVER_ESP_H_
#define _I2CDRIVER_ESP_H_

#include <cstdint>

#include "Gamepad.h"

namespace i2c_driver_esp32
{
    void initialize(std::array<Gamepad, MAX_GAMEPADS>& gamepad);
    bool pad_connected();
}

#endif // _I2CDRIVER_ESP_H_