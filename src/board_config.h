// Determines pinout for different boards
// Set one of these to 1, the other 0

#define BOARD_PI_PICO 0
#define BOARD_ADA_FEATHER_USBH 1
#define BOARD_RPZERO_INTERPOSER 0

// other options
#define CDC_DEBUG 0 // CDC device, include utilities/log.h and use log() as you would printf()

// don't edit below this unless you know what you're doing

#if (BOARD_ADA_FEATHER_USBH >= 1)
    #define PIO_USB_DP_PIN    16 // DM = 17
    #define LED_INDICATOR_PIN 13
#elif (BOARD_PI_PICO >= 1)
    #define PIO_USB_DP_PIN    0 // DM = 1
    #define LED_INDICATOR_PIN 25
#elif (BOARD_RPZERO_INTERPOSER >= 1)
    #define PIO_USB_DP_PIN    10 // DM = 11
    #define LED_INDICATOR_PIN 13
#endif