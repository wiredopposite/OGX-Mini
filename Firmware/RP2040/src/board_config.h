#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

/*  OGXM_BOARD and MAX_GAMEPADS are both defined as Cmake configure args,
    add args -DOGXM_BOARD=PI_PICO and -DMAX_GAMEPADS=1 (or = whatever option you want).
    If you're setting MAX_GAMEPADS > 1 only D-Input, Switch, and WebApp device drivers will work. */

#define ADA_FEATHER 1
#define PI_PICO 2
#define RP_ZERO 3
#define PI_PICOW 4
#define INTERNAL_4CH 5
#define EXTERNAL_4CH 6
#define W_ESP32 7

#ifndef MAX_GAMEPADS
    #define MAX_GAMEPADS 1
#endif

#ifndef OGXM_BOARD
    #define OGXM_BOARD PI_PICO
#endif

#if OGXM_BOARD == ADA_FEATHER
    #define RGB_PWR_PIN 20
    #define RGB_PXL_PIN 21 

    #define PIO_USB_DP_PIN    16 // DM = 17
    #define LED_INDICATOR_PIN 13
    #define VCC_EN_PIN        18

#elif OGXM_BOARD == PI_PICO
    #define PIO_USB_DP_PIN      0 // DM = 1
    #define LED_INDICATOR_PIN   25

#elif OGXM_BOARD == RP_ZERO
    #define RGB_PXL_PIN 16 

    #define PIO_USB_DP_PIN    10 // DM = 11
    #define LED_INDICATOR_PIN 14

#elif OGXM_BOARD == PI_PICOW

#elif OGXM_BOARD == INTERNAL_4CH
    #define PIO_USB_DP_PIN    16 // DM = 17 
    #define FOUR_CH_ENABLED   1
    #define I2C_SDA_PIN       10 // SCL = 11
    #define SLAVE_ADDR_PIN_1  20
    #define SLAVE_ADDR_PIN_2  21

#elif OGXM_BOARD == EXTERNAL_4CH
    #define RGB_PXL_PIN       16 
    #define FOUR_CH_ENABLED   1
    #define PIO_USB_DP_PIN    10 // DM = 11
    
    #define I2C_SDA_PIN       6  // SCL = 7
    #define SLAVE_ADDR_PIN_1  13
    #define SLAVE_ADDR_PIN_2  14

#elif OGXM_BOARD == W_ESP32
    #define I2C_SDA_PIN     18 // SCL = 19
    #define UART0_TX_PIN    16 // RX = 17
    #define MODE_SEL_PIN    21
    #define ESP_PROG_PIN    20 // ESP32 IO0
    #define ESP_RST_PIN     8  // ESP32 EN

    #if MAX_GAMEPADS > 1
        #undef MAX_GAMEPADS
        #define MAX_GAMEPADS 1
    #endif

#endif // OGXM_BOARD

#if defined(UART0_TX_PIN)
    #define UART0_RX_PIN (UART0_TX_PIN + 1)
#endif // defined(UART0_TX_PIN)

#if defined(I2C_SDA_PIN)
    #define I2C_BAUDRATE 400 * 1000
    #define I2C_SCL_PIN (I2C_SDA_PIN + 1)
    #define I2C_PORT    ((I2C_SDA_PIN == 2 ) || \
                         (I2C_SDA_PIN == 6 ) || \
                         (I2C_SDA_PIN == 10) || \
                         (I2C_SDA_PIN == 14) || \
                         (I2C_SDA_PIN == 18) || \
                         (I2C_SDA_PIN == 26)) ? i2c1 : i2c0
#endif // defined(I2C_SDA_PIN)

#if defined(CONFIG_EN_4CH)
    #if MAX_GAMEPADS < 4
        #undef MAX_GAMEPADS
        #define MAX_GAMEPADS 4
    #endif
#endif // FOUR_CH_ENABLED

#if defined(CONFIG_EN_USB_HOST)
    #define PIO_USB_CONFIG { \
                PIO_USB_DP_PIN, \
                PIO_USB_TX_DEFAULT, \
                PIO_SM_USB_TX_DEFAULT, \
                PIO_USB_DMA_TX_DEFAULT, \
                PIO_USB_RX_DEFAULT, \
                PIO_SM_USB_RX_DEFAULT, \
                PIO_SM_USB_EOP_DEFAULT, \
                NULL, \
                PIO_USB_DEBUG_PIN_NONE, \
                PIO_USB_DEBUG_PIN_NONE, \
                false, \
                PIO_USB_PINOUT_DPDM \
            }
#endif // PIO_USB_DP_PIN

#endif // _BOARD_CONFIG_H_