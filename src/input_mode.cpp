#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "input_mode.h"

enum InputMode load_input_mode()
{
    #ifdef HOST_ORIGINAL_XBOX
        return INPUT_MODE_XBOXORIGINAL;
    #elif HOST_NINTENDO_SWITCH
        return INPUT_MODE_SWITCH;
    #else
        return INPUT_MODE_XINPUT;
    #endif
}