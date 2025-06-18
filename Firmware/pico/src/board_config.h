#pragma once

#define BUILD_DATETIME                  "2025-06-18 08:44:57"
#define BUILD_VERSION                   "1.0.0a4"
#define VERIFY_BUILD_VERSION            0
#define OGXM_LOG_ENABLED                1
#define OGXM_LOG_LEVEL                  3

#define GAMEPADS_MAX                    1

#define LED_ENABLED                     0
#define LED_PIN                         

#define NEOPIXEL_ENABLED                0
#define NEOPIXEL_FORMAT_RGB             0
#define NEOPIXEL_FORMAT_GRB             1
#define NEOPIXEL_FORMAT_WRGB            2
#define NEOPIXEL_FORMAT                 
#define NEOPIXEL_DATA_PIN               
#define NEOPIXEL_VCC_ENABLED            0
#define NEOPIXEL_VCC_ENABLE_PIN         

#define UART_BRIDGE_ENABLED             0
#define UART_BRIDGE_UART_NUM            
#define UART_BRIDGE_PIN_TX              
#define UART_BRIDGE_PIN_RX              
#define UART_BRIDGE_PIN_BOOT            
#define UART_BRIDGE_PIN_RESET           

#define USBH_ENABLED                    0
#define USBH_PIO_ENABLED                0
#define USBH_PIO_DP_PIN                 
#define USBH_VCC_ENABLED                0
#define USBH_VCC_ENABLE_PIN             

#define USBD_PIO_ENABLED                0
#define USBD_PIO_USB_DP_PIN             

#define TS3USB221_ENABLED               0
#define TS3USB221_PIN_MUX_OE            
#define TS3USB221_PIN_MUX_SEL           

#define BLUETOOTH_ENABLED               1
#define BLUETOOTH_HARDWARE_PICOW        0
#define BLUETOOTH_HARDWARE_ESP32_I2C    1
#define BLUETOOTH_HARDWARE_ESP32_SPI    2
#define BLUETOOTH_HARDWARE              BLUETOOTH_HARDWARE_PICOW

#define ESP32_I2C_NUM                   
#define ESP32_I2C_PIN_SDA               
#define ESP32_I2C_PIN_SCL               
#define ESP32_I2C_PIN_IRQ               

#define ESP32_SPI_NUM                   
#define ESP32_SPI_PIN_MOSI              
#define ESP32_SPI_PIN_MISO              
#define ESP32_SPI_PIN_SCK               
#define ESP32_SPI_PIN_CS                
#define ESP32_SPI_PIN_IRQ               

#define ESP32_RESET_PIN                 UART_BRIDGE_PIN_RESET

#define FOUR_CHANNEL_I2C_NUM            
#define FOUR_CHANNEL_I2C_PIN_SDA        
#define FOUR_CHANNEL_I2C_PIN_SCL        
#define FOUR_CHANNEL_I2C_PIN_IRQ_1      
#define FOUR_CHANNEL_I2C_PIN_IRQ_2      
#define FOUR_CHANNEL_I2C_PIN_IRQ_3      
#define FOUR_CHANNEL_PIN_SLAVE_1        
#define FOUR_CHANNEL_PIN_SLAVE_2        

#define SD_CARD_ENABLED                 0
#define SD_CARD_SPI_NUM                 0
#define SD_CARD_SPI_PIN_SCK             0
#define SD_CARD_SPI_PIN_MOSI            0
#define SD_CARD_SPI_PIN_MISO            0
#define SD_CARD_SPI_PIN_CS              0

#define OGXM_BOARD_STANDARD             1
#define OGXM_BOARD_BLUETOOTH            2
#define OGXM_BOARD_DEVKIT               3
#define OGXM_BOARD_DEVKIT_ESP32         4
#define OGXM_BOARD_4CHANNEL             5
#define OGXM_BOARD_BLUETOOTH_USBH       6
#define OGXM_BOARD                      OGXM_BOARD_BLUETOOTH

#if (((OGXM_BOARD != OGXM_BOARD_DEVKIT) && (OGXM_BOARD != OGXM_BOARD_DEVKIT_ESP32)) \
    && USBH_PIO_ENABLED && USBD_PIO_ENABLED)
#error "USB Host and Device PIO cannot be enabled at the same time"
#endif
