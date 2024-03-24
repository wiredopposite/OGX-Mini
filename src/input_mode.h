#ifndef INPUT_MODE_H_
#define INPUT_MODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Gamepad.h"

enum InputMode
{
    INPUT_MODE_XINPUT = 0x01,
    INPUT_MODE_SWITCH = 0x02,
    INPUT_MODE_HID = 0x03,
    INPUT_MODE_PSCLASSIC = 0x04,
    INPUT_MODE_XBOXORIGINAL = 0x05,
    INPUT_MODE_USBSERIAL = 0x06,
};

enum InputMode get_input_mode();
bool change_input_mode(Gamepad previous_gamepad_state);

#ifdef __cplusplus
}
#endif

#endif // INPUT_MODE_H_