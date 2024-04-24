#ifndef _OGXM_CONFIG_H_
#define _OGXM_CONFIG_H_

#define ADAFRUIT_FEATHER_USBH 1
#define PI_PICO 2
#define RP2040_ZERO_INTERPOSER 3

// Options //
#define OGXM_BOARD ADAFRUIT_FEATHER_USBH
#define CDC_DEBUG 0
// ------- //

#if OGXM_BOARD == ADAFRUIT_FEATHER_USBH
    #define PIO_USB_DP_PIN    16 // DM = 17
    #define LED_INDICATOR_PIN 13
    #define VCC_EN_PIN        18
    // #define NEOPIXEL_PWR_PIN    20
    // #define NEOPIXEL_CTRL_PIN   21 

#elif OGXM_BOARD == PI_PICO
    #define PIO_USB_DP_PIN    0 // DM = 1
    #define LED_INDICATOR_PIN 25

#elif OGXM_BOARD == RP2040_ZERO_INTERPOSER
    #define PIO_USB_DP_PIN    10 // DM = 11
    #define LED_INDICATOR_PIN 13
    
#endif

#ifndef OGXM_BOARD
    #error OGXM_BOARD must be defined in ogxm_config.h
#endif

#ifndef CDC_DEBUG
    #define CDC_DEBUG 0
#endif


#endif // _OGXM_CONFIG_H_