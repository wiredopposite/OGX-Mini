cmake_minimum_required(VERSION 3.13)

set(FW_NAME "OGX-Mini")
set(FW_VERSION "1.0.0a4")

# set(CMAKE_C_COMPILER arm-none-eabi-gcc)
# set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(SRC ${CMAKE_CURRENT_LIST_DIR}/src)
set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/external)

# set(OGXMINI_BOARD "PI_PICO_W")
# set(MAX_GAMEPADS 1)

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/boards/${OGXMINI_BOARD}.cmake)
    message(FATAL_ERROR "Board configuration file for '${OGXMINI_BOARD}' not found.")
endif()

string(TIMESTAMP CURRENT_DATETIME "%Y-%m-%d %H:%M:%S")
set(BUILD_DATETIME  "\"${CURRENT_DATETIME}\"")
set(BUILD_VERSION   "\"${FW_VERSION}\"")

include(${CMAKE_CURRENT_LIST_DIR}/boards/init.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/boards/${OGXMINI_BOARD}.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

set(PICO_PIO_USB_PATH ${EXTERNAL_DIR}/Pico-PIO-USB)
set(PICO_TINYUSB_PATH ${EXTERNAL_DIR}/tinyusb)
set(BLUEPAD32_PATH ${EXTERNAL_DIR}/bluepad32)
set(PICO_BTSTACK_PATH ${EXTERNAL_DIR}/bluepad32/external/btstack)

project(${FW_NAME} C CXX ASM)

pico_sdk_init()

add_subdirectory(${EXTERNAL_DIR}/pico_usb)
add_subdirectory(${EXTERNAL_DIR}/libxsm3)
add_subdirectory(${EXTERNAL_DIR}/g726)
add_subdirectory(${EXTERNAL_DIR}/libfixmath libfixmath)

add_executable(${FW_NAME}
    ${SRC}/main_standard.c
    ${SRC}/main_bluetooth.c
    ${SRC}/main_devkit.c
    
    ${SRC}/log/log.c

    ${SRC}/settings/nvs.c
    ${SRC}/settings/settings.cpp
    ${SRC}/settings/button_combo.c

    ${SRC}/sdcard/sdcard.c

    ${SRC}/led/led.cpp
    ${SRC}/led/neopixel/WS2812.cpp

    ${SRC}/usb/device/device.c
    ${SRC}/usb/device/drivers/ps3.c
    ${SRC}/usb/device/drivers/xboxog_hub.c
    ${SRC}/usb/device/drivers/xboxog_gp.c
    ${SRC}/usb/device/drivers/xboxog_sb.c
    ${SRC}/usb/device/drivers/xboxog_xblc.c
    ${SRC}/usb/device/drivers/msc.c
    ${SRC}/usb/device/drivers/xinput.c
    ${SRC}/usb/device/drivers/generic_hub.c
    ${SRC}/usb/device/drivers/dinput.c
    ${SRC}/usb/device/drivers/switch.c
    ${SRC}/usb/device/drivers/psclassic.c
    ${SRC}/usb/device/drivers/uart_bridge.c
    ${SRC}/usb/device/drivers/webapp.c

    ${SRC}/usb/host/host.cpp
    ${SRC}/usb/host/drivers/xboxog_gp.c
    ${SRC}/usb/host/drivers/xinput_wl.c
    ${SRC}/usb/host/drivers/xinput.c
    ${SRC}/usb/host/drivers/xgip_gp.c
    ${SRC}/usb/host/drivers/ps3.c
    ${SRC}/usb/host/drivers/ps4.c
    ${SRC}/usb/host/drivers/ps5.c
    ${SRC}/usb/host/drivers/switch_pro.c
    ${SRC}/usb/host/drivers/hid/hid.c
    ${SRC}/usb/host/drivers/hid/dinput.c
    ${SRC}/usb/host/drivers/hid/psclassic.c
    ${SRC}/usb/host/drivers/hid/switch.c
    ${SRC}/usb/host/drivers/hid/n64.c
    ${SRC}/usb/host/drivers/hid/generic.c
    ${SRC}/usb/host/tusb_host/tuh_hxx.c
    ${SRC}/usb/host/tusb_host/tuh_xaudio.c

    ${SRC}/bluetooth/picow/picow.c
    ${SRC}/bluetooth/picow/ble_server.c
    ${SRC}/bluetooth/esp32/esp32_i2c.c
    ${SRC}/bluetooth/esp32/esp32_spi.c
)

target_include_directories(${FW_NAME} 
    PRIVATE
        ${SRC}
)

target_link_libraries(${FW_NAME} 
    PUBLIC
        pico_stdlib
        pico_multicore
        pico_rand
        pico_i2c_slave
        hardware_pio
        hardware_sync
        hardware_gpio
        hardware_i2c
        hardware_spi
        hardware_flash
        
        pico_usb
        libxsm3
        g726
        libfixmath
        tinyusb_board
        tinyusb_host
        tinyusb_pico_pio_usb
)

target_compile_definitions(${FW_NAME} 
    PUBLIC
        PICO_DEFAULT_UART=${PICO_DEFAULT_UART}
        PICO_DEFAULT_UART_TX_PIN=${PICO_DEFAULT_UART_TX_PIN}
        PICO_DEFAULT_UART_RX_PIN=${PICO_DEFAULT_UART_RX_PIN}
        PICO_FLASH_SIZE_BYTES=${PICO_FLASH_SIZE_BYTES}
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/board_config.h.in
    ${SRC}/board_config.h
    @ONLY
)

if(PICO_BOARD STREQUAL pico_w OR PICO_BOARD STREQUAL pico2_w)
    execute_process(
        COMMAND git apply --ignore-whitespace ${BLUEPAD32_PATH}/../btstack_l2cap.diff
        WORKING_DIRECTORY ${PICO_BTSTACK_PATH}
        RESULT_VARIABLE result
        ERROR_VARIABLE error
    )

    include_directories(${PICO_BTSTACK_PATH}/3rd-party/bluedroid/encoder/include)
    include_directories(${PICO_BTSTACK_PATH}/3rd-party/bluedroid/decoder/include)
    include_directories(${BLUEPAD32_PATH}/src/components/bluepad32/include)
    include_directories(bluepad32 ${SRC}/bluetooth/picow)
    target_include_directories(pico_btstack_classic INTERFACE ${SRC}/bluetooth/picow)

    target_link_libraries(${FW_NAME} 
        PUBLIC
            pico_cyw43_arch_none
            pico_btstack_classic
            pico_btstack_cyw43
            bluepad32
    )

    add_subdirectory(${BLUEPAD32_PATH}/src/components/bluepad32 libbluepad32)

    target_compile_definitions(bluepad32 
        PUBLIC
            GAMEPADS_MAX=${GAMEPADS_MAX}
    )

    target_include_directories(bluepad32 
        PUBLIC
            ${SRC}/bluetooth/picow
    )
# else()
#     add_compile_definitions(PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H=1)
#     add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)
endif()

if(GAMEPADS_MAX LESS 1 OR GAMEPADS_MAX GREATER 4)
    message(FATAL_ERROR "GAMEPADS_MAX must be 1 to 4")
elseif(GAMEPADS_MAX EQUAL 1)
    target_compile_definitions(pico_usb PUBLIC
        USBD_DEVICES_MAX=1
    )
else()
    target_compile_definitions(pico_usb PUBLIC
        USBD_DEVICES_MAX=${GAMEPADS_MAX}+1
    )
endif()

target_compile_definitions(libfixmath PRIVATE
    FIXMATH_FAST_SIN
    FIXMATH_NO_64BIT
    FIXMATH_NO_CACHE
    FIXMATH_NO_HARD_DIVISION
    FIXMATH_NO_OVERFLOW
    # FIXMATH_NO_ROUNDING
    # FIXMATH_OPTIMIZE_8BIT
)

target_include_directories(tinyusb_host
    INTERFACE
        ${SRC}/usb/host/tusb_host
)

target_compile_definitions(tinyusb_host
    INTERFACE
        # $<$<CONFIG:Release>:CFG_TUSB_DEBUG=0>
        # $<$<CONFIG:Debug>:CFG_TUSB_DEBUG=2>
        # $<$<CONFIG:Debug>:CFG_TUH_LOG_LEVEL=2>
)

pico_generate_pio_header(${FW_NAME} ${SRC}/led/neopixel/WS2812.pio)

pico_add_extra_outputs(${FW_NAME})