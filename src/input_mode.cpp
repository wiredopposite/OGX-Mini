#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "input_mode.h"

enum InputMode get_input_mode()
{
    #ifdef HOST_ORIGINAL_XBOX
        return INPUT_MODE_XBOXORIGINAL;
    #elif HOST_NINTENDO_SWITCH
        return INPUT_MODE_SWITCH;
    #elif HOST_PLAYSTATION_CLASSIC
        return INPUT_MODE_PSCLASSIC;
    #elif HOST_PLAYSTATION_3
        return INPUT_MODE_HID;
    #elif HOST_DEBUG
        return INPUT_MODE_USBSERIAL;
    #else
        return INPUT_MODE_XINPUT;
    #endif
}