#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#define PI_PICO             0
#define RP2040_ZERO         1
#define ADAFRUIT_FEATHER    2
#define PI_PICOW            3
#define ESP32_BLUEPAD32_I2C 4
#define ESP32_BLUERETRO_I2C 5
#define EXTERNAL_4CH_I2C    6
#define INTERNAL_4CH_I2C    7
#define BOARDS_COUNT        8

#define SYSCLOCK_KHZ 240000

#ifndef MAX_GAMEPADS
    #define MAX_GAMEPADS 1
#endif

#if defined(CONFIG_OGXM_BOARD_PI_PICO) || defined(CONFIG_OGXM_BOARD_PI_PICO2)
    #define OGXM_BOARD          PI_PICO
    #define PIO_USB_DP_PIN      0 // DM = 1
    #define LED_INDICATOR_PIN   25

#elif defined(CONFIG_OGXM_BOARD_PI_PICOW) || defined(CONFIG_OGXM_BOARD_PI_PICO2W)
    #define OGXM_BOARD          PI_PICOW

#elif defined(CONFIG_OGXM_BOARD_RP2040_ZERO)
    #define OGXM_BOARD          RP2040_ZERO
    #define RGB_PXL_PIN         16 
    #define PIO_USB_DP_PIN      10 // DM = 11
    #define LED_INDICATOR_PIN   14

#elif defined(CONFIG_OGXM_BOARD_ADAFRUIT_FEATHER)
    #define OGXM_BOARD          ADAFRUIT_FEATHER
    #define RGB_PWR_PIN         20
    #define RGB_PXL_PIN         21 

    #define PIO_USB_DP_PIN      16 // DM = 17
    #define LED_INDICATOR_PIN   13
    #define VCC_EN_PIN          18

#elif defined(CONFIG_OGXM_BOARD_INTERNAL_4CH)
    #define OGXM_BOARD          INTERNAL_4CH_I2C
    #define PIO_USB_DP_PIN      16 // DM = 17 
    #define FOUR_CH_ENABLED     1
    #define I2C_SDA_PIN         10
    #define I2C_SCL_PIN         11
    #define SLAVE_ADDR_PIN_1    20
    #define SLAVE_ADDR_PIN_2    21

#elif defined(CONFIG_OGXM_BOARD_EXTERNAL_4CH)
    #define OGXM_BOARD          EXTERNAL_4CH_I2C
    #define RGB_PXL_PIN         16 
    #define FOUR_CH_ENABLED     1
    #define PIO_USB_DP_PIN      10 // DM = 11
    #define I2C_SDA_PIN         6
    #define I2C_SCL_PIN         7
    #define SLAVE_ADDR_PIN_1    13
    #define SLAVE_ADDR_PIN_2    14

#elif defined(CONFIG_OGXM_BOARD_ESP32_BLUEPAD32_I2C)
    #define OGXM_BOARD          ESP32_BLUEPAD32_I2C
    #define I2C_SDA_PIN         18
    #define I2C_SCL_PIN         19
    #define UART0_TX_PIN        16
    #define UART0_RX_PIN        17
    #define MODE_SEL_PIN        21
    #define ESP_PROG_PIN        20 // ESP32 IO0
    #define ESP_RST_PIN         8  // ESP32 EN

    #if MAX_GAMEPADS > 1
        #undef MAX_GAMEPADS
        #define MAX_GAMEPADS 1
    #endif

#elif defined(CONFIG_OGXM_BOARD_ESP32_BLUERETRO_I2C)
    #define OGXM_BOARD          ESP32_BLUERETRO_I2C
    #define I2C_SDA_PIN         18
    #define I2C_SCL_PIN         19
    #define UART0_TX_PIN        16
    #define UART0_RX_PIN        17
    #define MODE_SEL_PIN        21
    #define ESP_PROG_PIN        20 // ESP32 IO0
    #define ESP_RST_PIN         8  // ESP32 EN

    #if MAX_GAMEPADS > 1
        #undef MAX_GAMEPADS
        #define MAX_GAMEPADS 1
    #endif

#else
    #error "Invalid OGXMini board selected"

#endif

#if defined(CONFIG_OGXM_DEBUG)
    //Pins and port are defined in CMakeLists.txt
    #define DEBUG_UART_PORT __CONCAT(uart,PICO_DEFAULT_UART)
#endif // defined(CONFIG_OGXM_DEBUG)

#if defined(I2C_SDA_PIN)
    #define I2C_BAUDRATE 400 * 1000
    #define I2C_PORT    ((I2C_SDA_PIN == 2 ) || \
                         (I2C_SDA_PIN == 6 ) || \
                         (I2C_SDA_PIN == 10) || \
                         (I2C_SDA_PIN == 14) || \
                         (I2C_SDA_PIN == 18) || \
                         (I2C_SDA_PIN == 26)) ? i2c1 : i2c0
#endif // defined(I2C_SDA_PIN)

#if defined(PIO_USB_DP_PIN)
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
#endif // defined(PIO_USB_DP_PIN)

#endif // _BOARD_CONFIG_H_