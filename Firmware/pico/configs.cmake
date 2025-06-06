function(set_ogxm_config TARGET BOARD_STR GAMEPADS)
    include(${CMAKE_CURRENT_LIST_DIR}/../shared/firmware_defines.cmake)

    string(TIMESTAMP CURRENT_DATETIME "%Y-%m-%d %H:%M:%S")

    set(BUILD_DATETIME          "'${CURRENT_DATETIME}'")
    set(BUILD_VERSION           "'${FW_VERSION}'")
    set(VERIFY_BUILD_VERSION    0)

    set(LED_ENABLED 0)
    set(LED_PIN     25)

    set(NEOPIXEL_ENABLED        0)
    set(NEOPIXEL_FORMAT_RGB     0)
    set(NEOPIXEL_FORMAT_GRB     1)
    set(NEOPIXEL_FORMAT_WRGB    2)
    set(NEOPIXEL_FORMAT         0)
    set(NEOPIXEL_DATA_PIN       0)
    set(NEOPIXEL_VCC_ENABLED    0)
    set(NEOPIXEL_VCC_ENABLE_PIN 0)

    set(UART_BRIDGE_ENABLED     0)
    set(UART_BRIDGE_UART_NUM    0)
    set(UART_BRIDGE_PIN_TX      0)
    set(UART_BRIDGE_PIN_RX      0)
    set(UART_BRIDGE_PIN_BOOT    0)
    set(UART_BRIDGE_PIN_RESET   0)

    set(USBH_ENABLED            0)
    set(USBH_PIO_ENABLED        1)
    set(USBH_PIO_DP_PIN         0)
    set(USBH_VCC_ENABLED        0)
    set(USBH_VCC_ENABLE_PIN     0)

    set(USBD_PIO_ENABLED        0) 
    set(USBD_PIO_USB_DP_PIN     0) 

    set(TS3USB221_ENABLED       0)
    set(TS3USB221_PIN_MUX_OE    0)
    set(TS3USB221_PIN_MUX_SEL   0)

    set(BLUETOOTH_ENABLED               0)
    set(BLUETOOTH_HARDWARE_PICOW        0)
    set(BLUETOOTH_HARDWARE_ESP32_I2C    1)
    set(BLUETOOTH_HARDWARE_ESP32_SPI    2)
    set(BLUETOOTH_HARDWARE              0)

    set(ESP32_I2C_NUM           0)
    set(ESP32_I2C_PIN_SDA       0)
    set(ESP32_I2C_PIN_SCL       0)

    set(ESP32_SPI_NUM           0) 
    set(ESP32_SPI_PIN_MOSI      0) 
    set(ESP32_SPI_PIN_MISO      0) 
    set(ESP32_SPI_PIN_SCK       0) 
    set(ESP32_SPI_PIN_CS        0) 
    set(ESP32_SPI_PIN_IRQ       0) 

    set(OGXM_BOARD_STANDARD     1                       PARENT_SCOPE)
    set(OGXM_BOARD_BLUETOOTH    2                       PARENT_SCOPE)
    set(OGXM_BOARD_DEVKIT       3                       PARENT_SCOPE)
    set(OGXM_BOARD_4CHANNEL     4                       PARENT_SCOPE)
    set(OGXM_BOARD              OGXM_BOARD_STANDARD     PARENT_SCOPE)

    set(PICO_DEFAULT_UART           1           PARENT_SCOPE)
    set(PICO_DEFAULT_UART_TX_PIN    4           PARENT_SCOPE)
    set(PICO_DEFAULT_UART_RX_PIN    5           PARENT_SCOPE)
    set(PICO_FLASH_SIZE_BYTES       2*1000*1000 PARENT_SCOPE)
    set(PICO_PLATFORM               rp2040      PARENT_SCOPE)
    set(PICO_BOARD                  pico        PARENT_SCOPE)

    if(BOARD_STR STREQUAL "PI_PICO")
        set(LED_ENABLED     1   PARENT_SCOPE)
        set(LED_PIN         25  PARENT_SCOPE)
        set(USBH_ENABLED    1   PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "PI_PICO_2")
        set(PICO_BOARD      pico2   PARENT_SCOPE)
        set(PICO_PLATFORM   rp2350  PARENT_SCOPE)
        set(PICO_FLASH_SIZE_BYTES   4*1000*1000     PARENT_SCOPE)
        set(LED_ENABLED     1       PARENT_SCOPE)
        set(LED_PIN         25      PARENT_SCOPE)
        set(USBH_ENABLED    1       PARENT_SCOPE)
        
    elseif(BOARD_STR STREQUAL "PI_PICO_W")
        set(PICO_BOARD          pico_w  PARENT_SCOPE)
        set(BLUETOOTH_ENABLED   1       PARENT_SCOPE)
        set(BLUETOOTH_HARDWARE  BLUETOOTH_HARDWARE_PICOW    PARENT_SCOPE)
        set(OGXM_BOARD          OGXM_BOARD_BLUETOOTH        PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "PI_PICO_2_W")
        set(PICO_BOARD              pico2_w         PARENT_SCOPE)
        set(PICO_PLATFORM           rp2350          PARENT_SCOPE)
        set(PICO_FLASH_SIZE_BYTES   4*1000*1000     PARENT_SCOPE)
        set(BLUETOOTH_ENABLED       1               PARENT_SCOPE)
        set(BLUETOOTH_HARDWARE      BLUETOOTH_HARDWARE_PICOW    PARENT_SCOPE)
        set(OGXM_BOARD              OGXM_BOARD_BLUETOOTH        PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "ADAFRUIT_FEATHER_RP2040_USBH")
        set(PICO_BOARD              adafruit_feather_rp2040_usb_host PARENT_SCOPE)
        set(NEOPIXEL_ENABLED        1   PARENT_SCOPE)
        set(NEOPIXEL_VCC_ENABLED    1   PARENT_SCOPE)
        set(NEOPIXEL_VCC_ENABLE_PIN 20  PARENT_SCOPE)
        set(NEOPIXEL_DATA_PIN       21  PARENT_SCOPE)
        set(NEOPIXEL_FORMAT         NEOPIXEL_FORMAT_GRB PARENT_SCOPE)
        set(USBH_ENABLED            1   PARENT_SCOPE)
        set(USBH_PIO_DP_PIN         16  PARENT_SCOPE)
        set(USBH_VCC_ENABLED        1   PARENT_SCOPE)
        set(USBH_VCC_ENABLE_PIN     18  PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "WAVESHARE_RP2040_ZERO")
        set(PICO_BOARD          waveshare_rp2040_zero PARENT_SCOPE)
        set(NEOPIXEL_ENABLED    1   PARENT_SCOPE)
        set(NEOPIXEL_DATA_PIN   16  PARENT_SCOPE)
        set(NEOPIXEL_FORMAT     NEOPIXEL_FORMAT_GRB PARENT_SCOPE)
        set(USBH_ENABLED        1   PARENT_SCOPE)
        set(USBH_PIO_DP_PIN     10  PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "WAVESHARE_RP2350_USBH")
        set(PICO_PLATFORM           rp2350      PARENT_SCOPE)
        # set(PICO_FLASH_SIZE_BYTES   4*1000*1000 PARENT_SCOPE)
        set(NEOPIXEL_ENABLED        1           PARENT_SCOPE)
        set(NEOPIXEL_DATA_PIN       16          PARENT_SCOPE)
        set(NEOPIXEL_FORMAT         NEOPIXEL_FORMAT_RGB PARENT_SCOPE)
        set(USBH_ENABLED            1           PARENT_SCOPE)
        set(USBH_PIO_DP_PIN         12          PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "ESP32_I2C")
        set(BLUETOOTH_ENABLED       1   PARENT_SCOPE)
        set(BLUETOOTH_HARDWARE      BLUETOOTH_HARDWARE_ESP32_I2C PARENT_SCOPE)
        set(ESP32_I2C_NUM           1   PARENT_SCOPE)
        set(ESP32_I2C_PIN_SDA       18  PARENT_SCOPE)
        set(ESP32_I2C_PIN_SCL       19  PARENT_SCOPE)
        set(UART_BRIDGE_ENABLED     1   PARENT_SCOPE)
        set(UART_BRIDGE_UART_NUM    0   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_TX      16  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RX      17  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_BOOT    20  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RST     8   PARENT_SCOPE)
        set(OGXM_BOARD              OGXM_BOARD_BLUETOOTH PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "ESP32_SPI")
        set(BLUETOOTH_ENABLED       1   PARENT_SCOPE)
        set(BLUETOOTH_HARDWARE      BLUETOOTH_HARDWARE_ESP32_SPI PARENT_SCOPE)
        set(ESP32_SPI_NUM           1   PARENT_SCOPE)
        set(ESP32_SPI_PIN_SCK       10  PARENT_SCOPE)
        set(ESP32_SPI_PIN_MOSI      11  PARENT_SCOPE)
        set(ESP32_SPI_PIN_MISO      12  PARENT_SCOPE)
        set(ESP32_SPI_PIN_CS        13  PARENT_SCOPE)
        set(ESP32_SPI_PIN_IRQ       7   PARENT_SCOPE)
        set(UART_BRIDGE_ENABLED     1   PARENT_SCOPE)
        set(UART_BRIDGE_UART_NUM    1   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_TX      8   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RX      9   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_BOOT    22  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RST     6   PARENT_SCOPE)
        set(OGXM_BOARD              OGXM_BOARD_BLUETOOTH PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "DEVKIT")
        set(BLUETOOTH_ENABLED           1   PARENT_SCOPE)
        set(BLUETOOTH_HARDWARE          BLUETOOTH_HARDWARE_ESP32_SPI PARENT_SCOPE)
        set(ESP32_SPI_NUM               1   PARENT_SCOPE)
        set(ESP32_SPI_PIN_SCK           10  PARENT_SCOPE)
        set(ESP32_SPI_PIN_MOSI          11  PARENT_SCOPE)
        set(ESP32_SPI_PIN_MISO          12  PARENT_SCOPE)
        set(ESP32_SPI_PIN_CS            13  PARENT_SCOPE)
        set(ESP32_SPI_PIN_IRQ           7   PARENT_SCOPE)
        set(UART_BRIDGE_ENABLED         1   PARENT_SCOPE)
        set(UART_BRIDGE_UART_NUM        1   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_TX          8   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RX          9   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_BOOT        22  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RESET       6   PARENT_SCOPE)
        set(PICO_DEFAULT_UART           0   PARENT_SCOPE)
        set(PICO_DEFAULT_UART_TX_PIN    16  PARENT_SCOPE)
        set(PICO_DEFAULT_UART_RX_PIN    17  PARENT_SCOPE)
        set(TS3USB221_ENABLED           1   PARENT_SCOPE)
        set(TS3USB221_PIN_MUX_OE        20  PARENT_SCOPE)
        set(TS3USB221_PIN_MUX_SEL       21  PARENT_SCOPE)
        set(USBD_PIO_ENABLED            1   PARENT_SCOPE)
        set(USBH_ENABLED                1   PARENT_SCOPE)
        set(OGXM_BOARD                  OGXM_BOARD_DEVKIT PARENT_SCOPE)

    elseif(BOARD_STR STREQUAL "OGXW_HW1")
        set(BLUETOOTH_ENABLED       1   PARENT_SCOPE)
        set(BLUETOOTH_HARDWARE      BLUETOOTH_HARDWARE_ESP32_I2C PARENT_SCOPE)
        set(ESP32_I2C_NUM           1   PARENT_SCOPE)
        set(ESP32_I2C_PIN_SDA       18  PARENT_SCOPE)
        set(ESP32_I2C_PIN_SCL       19  PARENT_SCOPE)
        set(UART_BRIDGE_ENABLED     1   PARENT_SCOPE)
        set(UART_BRIDGE_UART_NUM    0   PARENT_SCOPE)
        set(UART_BRIDGE_PIN_TX      16  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RX      17  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_BOOT    20  PARENT_SCOPE)
        set(UART_BRIDGE_PIN_RST     8   PARENT_SCOPE)
        set(VERIFY_BUILD_VERSION    1   PARENT_SCOPE)
        set(OGXM_BOARD              OGXM_BOARD_BLUETOOTH PARENT_SCOPE)

    # elseif(BOARD_STR STREQUAL "4CHANNEL_EXTERNAL")
    #     target_compile_definitions(${TARGET} PUBLIC
    #         OGXM_BOARD_4CHANNEL=1
    #         GAMEPADS_MAX=4
    #     )
    else()
        message(FATAL_ERROR "Unknown board: ${BOARD_STR}")
    endif()

    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/board_config.h.in
        ${CMAKE_CURRENT_LIST_DIR}/src/board_config.h
        @ONLY
    )
endfunction() 

# function(set_ogxm_config TARGET BOARD_STR GAMEPADS)
#     if(BOARD_STR STREQUAL "PI_PICO")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_STANDARD=1
#             PICO_DEFAULT_LED_PIN=25
#             PICO_DEFAULT_UART=1
#             PICO_DEFAULT_UART_TX_PIN=4
#             PICO_DEFAULT_UART_RX_PIN=5
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "PI_PICO_2")
#         set(PICO_PLATFORM rp2350 PARENT_SCOPE)
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_STANDARD=1
#             PICO_FLASH_SIZE_BYTES=4*1000*1000
#             PICO_DEFAULT_LED_PIN=25
#             PICO_DEFAULT_UART=1
#             PICO_DEFAULT_UART_TX_PIN=4
#             PICO_DEFAULT_UART_RX_PIN=5
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "PI_PICO_W")
#         set(PICO_BOARD pico_w PARENT_SCOPE)
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_BLUETOOTH=1
#             PICO_DEFAULT_UART=1
#             PICO_DEFAULT_UART_TX_PIN=4
#             PICO_DEFAULT_UART_RX_PIN=5
#             BLUETOOTH_HW=BLUETOOTH_HW_PICOW
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "PI_PICO_2_W")
#         set(PICO_BOARD pico2_w PARENT_SCOPE)
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_BLUETOOTH=1
#             PICO_FLASH_SIZE_BYTES=4*1000*1000
#             PICO_DEFAULT_UART=1
#             PICO_DEFAULT_UART_TX_PIN=4
#             PICO_DEFAULT_UART_RX_PIN=5
#             BLUETOOTH_HW=BLUETOOTH_HW_PICOW
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "ADAFRUIT_FEATHER_RP2040_USBH")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_STANDARD=1
#             NEOPIXEL_POWER_PIN=20
#             NEOPIXEL_DATA_PIN=21
#             NEOPIXEL_FORMAT=NEOPIXEL_FORMAT_GRB
#             USBH_PIO_DP_PIN=16
#             USBH_VCC_ENABLE_PIN=18
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "WAVESHARE_RP2040_ZERO")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_STANDARD=1
#             PICO_DEFAULT_LED_PIN=14
#             NEOPIXEL_DATA_PIN=16
#             NEOPIXEL_FORMAT=NEOPIXEL_FORMAT_GRB
#             USBH_PIO_DP_PIN=10
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "WAVESHARE_RP2350_USB_A")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_STANDARD=1
#             NEOPIXEL_DATA_PIN=16
#             NEOPIXEL_FORMAT=NEOPIXEL_FORMAT_RGB
#             USBH_PIO_DP_PIN=12
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "ESP32_I2C")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_BLUETOOTH=1
#             ESP32_I2C=1
#             ESP32_I2C_SDA_PIN=18
#             ESP32_I2C_SCL_PIN=19
#             UART_BRIDGE_UART=0
#             UART_BRIDGE_PIN_TX=16
#             UART_BRIDGE_PIN_RX=17
#             UART_BRIDGE_PIN_BOOT=20
#             UART_BRIDGE_PIN_RST=8
#             BLUETOOTH_HW=BLUETOOTH_HW_ESP32_I2C
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "ESP32_SPI")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_BLUETOOTH=1
#             ESP32_SPI=1
#             ESP32_SPI_PIN_SCK=10
#             ESP32_SPI_PIN_MOSI=11
#             ESP32_SPI_PIN_MISO=12
#             ESP32_SPI_PIN_CS=13
#             ESP32_SPI_PIN_INT=7
#             UART_BRIDGE_UART=1
#             UART_BRIDGE_PIN_TX=8
#             UART_BRIDGE_PIN_RX=9
#             UART_BRIDGE_PIN_BOOT=22
#             UART_BRIDGE_PIN_RST=6
#             # PICO_DEFAULT_UART=0
#             # PICO_DEFAULT_UART_TX_PIN=16
#             # PICO_DEFAULT_UART_RX_PIN=17
#             BLUETOOTH_HW=BLUETOOTH_HW_ESP32_SPI
#             # GAMEPADS_MAX=GAMEPADS
#         )
#     elseif(BOARD_STR STREQUAL "DEVKIT")
#         string(TIMESTAMP CURRENT_DATETIME "%Y-%m-%d %H:%M:%S")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_DEVKIT=1
#             PICO_DEFAULT_LED_PIN=25
#             ESP32_SPI=1
#             ESP32_SPI_PIN_SCK=10
#             ESP32_SPI_PIN_MOSI=11
#             ESP32_SPI_PIN_MISO=12
#             ESP32_SPI_PIN_CS=13
#             ESP32_SPI_PIN_INT=7
#             UART_BRIDGE_UART=1
#             UART_BRIDGE_PIN_TX=8
#             UART_BRIDGE_PIN_RX=9
#             UART_BRIDGE_PIN_BOOT=22
#             UART_BRIDGE_PIN_RESET=6
#             PICO_DEFAULT_UART=0
#             PICO_DEFAULT_UART_TX_PIN=16
#             PICO_DEFAULT_UART_RX_PIN=17
#             BLUETOOTH_HW=BLUETOOTH_HW_ESP32_SPI
#             # GAMEPADS_MAX=GAMEPADS
#             USBD_PIO_ENABLED=1
#             TS3USB221_ENABLED=1
#             TS3USB221_PIN_MUX_OE=20
#             TS3USB221_PIN_MUX_SEL=21
#             BUILD_DATETIME="${CURRENT_DATETIME}"
#         )
#     elseif(BOARD_STR STREQUAL "OGXW_HW1")
#         string(TIMESTAMP CURRENT_DATETIME "%Y-%m-%d %H:%M:%S")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_BLUETOOTH=1
#             ESP32_I2C=1
#             ESP32_I2C_SDA_PIN=18
#             ESP32_I2C_SCL_PIN=19
#             UART_BRIDGE_UART=0
#             UART_BRIDGE_PIN_TX=16
#             UART_BRIDGE_PIN_RX=17
#             UART_BRIDGE_PIN_BOOT=20
#             UART_BRIDGE_PIN_RST=8
#             BLUETOOTH_HW=BLUETOOTH_HW_ESP32_I2C
#             # GAMEPADS_MAX=GAMEPADS
#             VERIFY_DATETIME=1
#             BUILD_DATETIME="${CURRENT_DATETIME}"
#         )
#     elseif(BOARD_STR STREQUAL "4CHANNEL_EXTERNAL")
#         target_compile_definitions(${TARGET} PUBLIC
#             OGXM_BOARD_4CHANNEL=1
#             GAMEPADS_MAX=4
#         )
#     else()
#         message(FATAL_ERROR "Unknown board: ${BOARD_STR}")
#     endif()
# endfunction() 

# function(import_bluepad32 TARGET BLUEPAD32_PATH INCLUDE_DIR MAX_GAMEPADS)
#     execute_process(
#         COMMAND git submodule update --init --recursive
#         WORKING_DIRECTORY ${BLUEPAD32_PATH}
#         RESULT_VARIABLE result
#     )
#     execute_process(
#         COMMAND git apply --ignore-whitespace ${BLUEPAD32_PATH}/../btstack_l2cap.diff
#         WORKING_DIRECTORY ${BLUEPAD32_PATH}/external/btstack
#         RESULT_VARIABLE result
#         ERROR_VARIABLE error
#     )

#     # if(result EQUAL 0)
#     #     message(STATUS "Bluepad32 patches applied successfully.")
#     # else()
#     #     message(WARNING "Failed to apply Bluepad32 patches.")
#     #     if(error)
#     #         message(WARNING "Error: ${error}")
#     #     endif()
#     # endif()

#     add_subdirectory(${BLUEPAD32_PATH}/src/components/bluepad32)

#     include_directories(${BLUEPAD32_PATH}/external/btstack/3rd-party/bluedroid/encoder/include)
#     include_directories(${BLUEPAD32_PATH}/external/btstack/3rd-party/bluedroid/decoder/include)
#     include_directories(${BLUEPAD32_PATH}/src/components/bluepad32/include)
#     include_directories(bluepad32 ${INCLUDE_DIR})
#     target_include_directories(pico_btstack_classic INTERFACE ${INCLUDE_DIR})

#     target_compile_definitions(bluepad32 
#         PUBLIC
#             GAMEPADS_MAX=${MAX_GAMEPADS}
#     )

#     target_link_libraries(${TARGET} 
#         PUBLIC
#             pico_cyw43_arch_none
#             pico_btstack_classic
#             pico_btstack_cyw43
#             bluepad32
#     )
# endfunction()