#pragma once

#include <cstdint>
#include <array>

#include "Gamepad/Gamepad.h"
#include "Board/Config.h"

/*  NOTE: Everything bluepad32/uni needs to be wrapped
    and kept away from tinyusb due to naming conflicts */

namespace bluepad32 {
    void run_task(Gamepad(&gamepads)[MAX_GAMEPADS]);
} 