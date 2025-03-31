#pragma once

#include <cstdint>

#include "Gamepad/Gamepad.h"

namespace BLEServer {
    void init_server(Gamepad(&gamepads)[MAX_GAMEPADS]);
}