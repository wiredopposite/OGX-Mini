#ifndef USBD_CONFIG_H_
#define USBD_CONFIG_H_

#include "usbd/usbd_boards.h"

// Boards
// OGXM_PI_PICO
// OGXM_ADA_FEATHER_USBH 
// OGXM_RPZERO_INTERPOSER

#define USBD_BOARD        OGXM_RPZERO_INTERPOSER
#define USBD_MAX_GAMEPADS 1 // This is set by idf.py menuconfig for wireless boards, number here is ignored in that case.
#define CDC_DEBUG         0 // Set to 1 for CDC device, helpful for debugging USB host. Include utilities/log.h and use log() as you would printf()

#endif // USBD_CONFIG_H_