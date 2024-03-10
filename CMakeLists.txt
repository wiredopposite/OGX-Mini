cmake_minimum_required(VERSION 3.12)

message("Build type: \"${CMAKE_BUILD_TYPE}\"")

# Project name
set(NAME OGX-Mini)

# Board type
set(PICO_BOARD none)

# Fixes that allow some MCH2022 badges with a slowly starting oscillator to boot properly
add_compile_definitions(PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H=1 PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

# SDK
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_PIO_USB_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/Pico-PIO-USB")
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb")

pico_sdk_init()

add_subdirectory(lib/Pico-PIO-USB)
add_subdirectory(lib)

file(GLOB_RECURSE SOURCES 
"src/usbh/*"
"src/usbh/tusb_xinput/*"
"src/usbh/tusb_hid/*"
"src/*" 
"src/utilities/*"
"src/usbd/*"
"src/usbd/shared/*"
"src/usbd/switch/*"
"src/usbd/xboxog/*" 
"src/usbd/dinput/*" 
"src/usbd/xinput/*")

# Firmware
add_executable(${NAME}
${SOURCES}
    )

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR}/lib)

include_directories(
lib/)

#------- Comment out the following line if compiling for the normal Pi Pico -------#

add_compile_definitions(FEATHER_RP2040)

#------- USB host data +/- will be GPIO 0/1 ---------------------------------------#

#------- CDC MODE --------#
# add_compile_definitions(HOST_DEBUG) # makes RP2040 a CDC device, include "utilities/log.h" and use log() as you would printf()


target_link_libraries(${NAME}
    pico_stdlib
    pico_unique_id
    pico_multicore
    hardware_watchdog
    hardware_flash
    hardware_sync
    hardware_uart
    hardware_pio
    hardware_pwm
    hardware_adc
    hardware_i2c
    tinyusb_device
    tinyusb_board
    tinyusb_host
    tinyusb_pico_pio_usb
    CRC32
    hid_parser
    cmsis_core
)

pico_add_extra_outputs(${NAME})
