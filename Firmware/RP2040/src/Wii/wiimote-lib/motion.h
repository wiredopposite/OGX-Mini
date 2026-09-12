#ifndef MOTION_H
#define MOTION_H

#include "wiimote.h"

void set_motion_state(struct wiimote_state * state, float pointer_x, float pointer_y);

#endif