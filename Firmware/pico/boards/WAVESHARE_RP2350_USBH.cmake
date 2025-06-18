set(NEOPIXEL_ENABLED            1)
set(NEOPIXEL_FORMAT             NEOPIXEL_FORMAT_RGB)
set(NEOPIXEL_DATA_PIN           16)

set(USBH_ENABLED                1)
set(USBH_PIO_ENABLED            1)
set(USBH_PIO_DP_PIN             12)

set(OGXM_BOARD                  OGXM_BOARD_STANDARD)

# set(PICO_DEFAULT_UART           1)
# set(PICO_DEFAULT_UART_TX_PIN    4)
# set(PICO_DEFAULT_UART_RX_PIN    5)
set(PICO_DEFAULT_UART           0)
set(PICO_DEFAULT_UART_TX_PIN    0)
set(PICO_DEFAULT_UART_RX_PIN    1)
set(PICO_FLASH_SIZE_BYTES       2*1024*1024)
set(PICO_PLATFORM               rp2350)
set(PICO_BOARD                  none)
# set(PICO_BOARD                  pico2)

add_compile_definitions(PICO_BOOT_STAGE2_CHOOSE_W25Q016=1)
add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128)