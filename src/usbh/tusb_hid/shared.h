#pragma once

#ifndef _SHARED_H_
#define _SHARED_H_

typedef struct 
{
    uint16_t vid;
    uint16_t pid;
} usb_vid_pid_t;

int16_t scale_uint8_to_int16(uint8_t value, bool invert);

#endif // _SHARED_H_