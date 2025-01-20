#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <cstdint>

#include "Gamepad/Gamepad.h"

namespace BLEServer 
{
    void init_server(Gamepad(&gamepads)[MAX_GAMEPADS]);
}

#endif // BLE_SERVER_H