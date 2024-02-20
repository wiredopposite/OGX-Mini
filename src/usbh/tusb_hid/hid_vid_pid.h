#pragma once

#ifndef _HID_VID_PID_H_
#define _HID_VID_PID_H_

#include <stdint.h>

// put this somewhere else?
typedef struct 
{
    uint16_t vid;
    uint16_t pid;
} usb_vid_pid_t;

#endif // _HID_VID_PID_H_