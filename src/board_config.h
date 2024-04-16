#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

// Type
#define WIRED    1
#define WIRELESS 2

// Boards // Wired
#define OGXM_PI_PICO            1
#define OGXM_ADA_FEATHER_USBH   2
#define OGXM_RPZERO_INTERPOSER  3
// Boards // Wireless
#define OGXW_RETAIL_1CH         11
#define OGXW_RPZERO_1CH         12
#define OGXW_LITE               13

// MCU
#define MCU_RP2040  1
#define MCU_ESP32S3 2

// Options
#define OGX_BOARD    OGXM_ADA_FEATHER_USBH
#define MAX_GAMEPADS 1
#define CDC_DEBUG    0 // CDC device, include utilities/log.h and use log() as you would printf()

// don't edit below this unless you're changing pinouts

#if OGX_BOARD != OGXW_LITE
    #define OGX_MCU MCU_RP2040
#else
    #define OGX_MCU MCU_ESP32S3
#endif

#if (OGX_BOARD == OGXM_ADA_FEATHER_USBH) || (OGX_BOARD == OGXM_PI_PICO) || (OGX_BOARD == OGXM_RPZERO_INTERPOSER)
    #define OGX_TYPE WIRED
#else
    #define OGX_TYPE WIRELESS
#endif

#if OGX_TYPE == WIRED
    #if OGX_BOARD == OGXM_ADA_FEATHER_USBH
        #define PIO_USB_DP_PIN    16 // DM = 17
        #define LED_INDICATOR_PIN 13
        #define VCC_EN_PIN        18
        // #define NEOPIXEL_PWR_PIN    20
        // #define NEOPIXEL_CTRL_PIN   21 

    #elif OGX_BOARD == OGXM_PI_PICO
        #define PIO_USB_DP_PIN    0 // DM = 1
        #define LED_INDICATOR_PIN 25

    #elif OGX_BOARD == OGXM_RPZERO_INTERPOSER
        #define PIO_USB_DP_PIN    10 // DM = 11
        #define LED_INDICATOR_PIN 13
        
    #endif

#elif OGX_TYPE == WIRELESS
    #if (OGX_BOARD == OGXW_RETAIL_1CH)
        #define ESP_PROG_PIN 10
        #define ESP_RST_PIN 10
        #define MODE_SEL_PIN 10

    #elif (OGX_BOARD == OGXW_RPZERO_1CH)
        #define ESP_PROG_PIN 10
        #define ESP_RST_PIN 10
        #define MODE_SEL_PIN 10

    #endif
#endif

#endif // BOARD_CONFIG_H_