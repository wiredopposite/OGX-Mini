set(LED_ENABLED                 1)
set(LED_PIN                     25)

set(UART_BRIDGE_ENABLED         1)
set(UART_BRIDGE_UART_NUM        1)
set(UART_BRIDGE_PIN_TX          8)
set(UART_BRIDGE_PIN_RX          9)
set(UART_BRIDGE_PIN_BOOT        22)
set(UART_BRIDGE_PIN_RESET       6)

# set(USBH_ENABLED                1)
# set(USBH_PIO_ENABLED            1)
# set(USBH_PIO_DP_PIN             18)

set(USBD_PIO_ENABLED            0) 
set(USBD_PIO_USB_DP_PIN         0) 

set(TS3USB221_ENABLED           1)
set(TS3USB221_PIN_MUX_OE        20)
set(TS3USB221_PIN_MUX_SEL       21)

set(BLUETOOTH_ENABLED           1)
# set(BLUETOOTH_HARDWARE          BLUETOOTH_HARDWARE_ESP32_SPI)
set(BLUETOOTH_HARDWARE          BLUETOOTH_HARDWARE_ESP32_I2C)

set(ESP32_SPI_NUM               1) 
set(ESP32_SPI_PIN_SCK           10) 
set(ESP32_SPI_PIN_MISO          11) 
set(ESP32_SPI_PIN_MOSI          12) 
set(ESP32_SPI_PIN_CS            13) 
set(ESP32_SPI_PIN_IRQ           7) 

set(ESP32_I2C_NUM               1)
set(ESP32_I2C_PIN_SDA           26)
set(ESP32_I2C_PIN_SCL           27)
set(ESP32_I2C_PIN_IRQ           UART_BRIDGE_PIN_TX)

set(OGXM_BOARD                  OGXM_BOARD_DEVKIT_ESP32)

set(PICO_DEFAULT_UART           0)
set(PICO_DEFAULT_UART_TX_PIN    16)
set(PICO_DEFAULT_UART_RX_PIN    17)
set(PICO_BOARD                  pico)