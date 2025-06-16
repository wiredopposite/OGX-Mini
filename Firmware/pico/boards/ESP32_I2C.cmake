set(UART_BRIDGE_ENABLED             1)
set(UART_BRIDGE_UART_NUM            0)
set(UART_BRIDGE_PIN_TX              16)
set(UART_BRIDGE_PIN_RX              17)
set(UART_BRIDGE_PIN_BOOT            20)
set(UART_BRIDGE_PIN_RESET           8)

set(BLUETOOTH_ENABLED               1)
set(BLUETOOTH_HARDWARE              BLUETOOTH_HARDWARE_ESP32_I2C)

set(ESP32_I2C_NUM                   1)
set(ESP32_I2C_PIN_SDA               18)
set(ESP32_I2C_PIN_SCL               19)
set(ESP32_I2C_PIN_IRQ               UART_BRIDGE_PIN_TX)

set(OGXM_BOARD                      OGXM_BOARD_BLUETOOTH)

set(PICO_DEFAULT_UART               1)
set(PICO_DEFAULT_UART_TX_PIN        4)
set(PICO_DEFAULT_UART_RX_PIN        5)