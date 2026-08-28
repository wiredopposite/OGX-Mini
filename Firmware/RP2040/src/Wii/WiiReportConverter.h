#ifndef OGXM_WII_REPORT_CONVERTER_H
#define OGXM_WII_REPORT_CONVERTER_H

#include "Board/Config.h"

#if defined(OGXM_BOARD_USES_PICO_W_FIRMWARE) && defined(CONFIG_EN_USB_HOST)

class Gamepad;

namespace Wii {

/** Fill WiimoteReport (C struct from wiimote.h) from Gamepad (for Wii mode BT). */
void gamepad_to_wiimote_report(Gamepad& gamepad, void* out_wiimote_report);

} // namespace Wii

#endif

#endif
