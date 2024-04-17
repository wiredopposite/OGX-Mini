Gamepad device drivers for TinyUSB, this is being used in several of my projects

This needs a board_config.h file in the project defining the board you're using and the number of gamepads, here's how mine looks
```
#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

// Boards // Wired
#define OGXM_PI_PICO            1
#define OGXM_ADA_FEATHER_USBH   2
#define OGXM_RPZERO_INTERPOSER  3
// Boards // Wireless
#define OGXW_RETAIL_1CH         11
#define OGXW_RPZERO_1CH         12
#define OGXW_RPZERO_2CH         13
#define OGXW_LITE               16

// ----- Options ---- //
#define OGX_BOARD        OGXM_ADA_FEATHER_USBH
#define OGX_MAX_GAMEPADS 1
#define CDC_DEBUG        0 // Set to 1 for CDC device, helpful for debugging usb host. Include utilities/log.h and use log() as you would printf()
// ------------------ //

// Don't edit below this unless you're changing pinouts for new hardware

// Type
#define WIRED    1
#define WIRELESS 2
// MCU
#define MCU_RP2040  1
#define MCU_ESP32S3 2

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
    #define MAX_GAMEPADS OGX_MAX_GAMEPADS

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
    #ifdef CONFIG_BLUEPAD32_MAX_DEVICES
        #define MAX_GAMEPADS CONFIG_BLUEPAD32_MAX_DEVICES
    #else
        #define MAX_GAMEPADS OGX_MAX_GAMEPADS
    #endif 

    #define PLAYER_ID1_PIN 2
    #define PLAYER_ID2_PIN 3

    #if (OGX_BOARD == OGXW_RETAIL_1CH)
        #define I2C1_SLAVE_SDA_PIN 18
        #define I2C1_SLAVE_SCL_PIN 19

        #define MODE_SEL_PIN 21

        #define ESP_PROG_PIN 20 // ESP32 IO0
        #define ESP_RST_PIN 8   // ESP32 EN

        #define UART0_TX_PIN 16
        #define UART0_RX_PIN 17

    #elif (OGX_BOARD == OGXW_RPZERO_1CH)
        #define I2C1_SLAVE_SDA_PIN 10
        #define I2C1_SLAVE_SCL_PIN 11

        #define MODE_SEL_PIN 9

        #define ESP_PROG_PIN 15 // ESP32 IO0
        #define ESP_RST_PIN 14  // ESP32 EN

        #define UART0_TX_PIN 12
        #define UART0_RX_PIN 13

    #elif (OGX_BOARD == OGXW_RPZERO_2CH)
        #define I2C1_SLAVE_SDA_PIN 10
        #define I2C1_SLAVE_SCL_PIN 11

        #define MODE_SEL_PIN 8

        #define ESP_PROG_PIN 9 // ESP32 IO0
        #define ESP_RST_PIN  7  // ESP32 EN

        #define UART0_TX_PIN 12
        #define UART0_RX_PIN 13

    #endif
#endif

#endif // BOARD_CONFIG_H_
```