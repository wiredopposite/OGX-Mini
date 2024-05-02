cmake_minimum_required(VERSION 3.12)

set(CMAKE_BUILD_TYPE Release)
message("Build type: \"${CMAKE_BUILD_TYPE}\"")

set(NAME OGX-Mini)

set(PICO_BOARD none)

# Fixes that allow some MCH2022 badges with a slowly starting oscillator to boot properly
# add_compile_definitions(PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H=1 PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

# add_compile_options(-Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-function)
add_compile_options(-Wno-missing-field-initializers)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(ROOT ${CMAKE_CURRENT_LIST_DIR})

set(PICO_PIO_USB_PATH ${ROOT}/lib/Pico-PIO-USB)
set(PICO_TINYUSB_PATH ${ROOT}/lib/tinyusb)
set(TUSB_GAMEPAD_PATH ${ROOT}/lib/tusb_gamepad)
set(XINPUT_HOST_PATH ${ROOT}/lib/tusb_xinput)

pico_sdk_init()

# add_subdirectory(${ROOT}/lib)
add_subdirectory(${ROOT}/lib/CRC32 CRC32)
add_subdirectory(${XINPUT_HOST_PATH} xinput_host)
add_subdirectory(${PICO_PIO_USB_PATH})
add_subdirectory(${TUSB_GAMEPAD_PATH})
target_include_directories(tusb_gamepad PRIVATE ${ROOT}/src) # so tusb_gamepad can see tusb_config.h

set(SRC_DIR ${ROOT}/src)
file(GLOB_RECURSE SOURCES 
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/input_mode.cpp

    ${SRC_DIR}/usbh/tusb_host_manager.cpp
    ${SRC_DIR}/usbh/tusb_host.cpp
    ${SRC_DIR}/usbh/n64usb/N64USB.cpp
    ${SRC_DIR}/usbh/ps3/Dualshock3.cpp
    ${SRC_DIR}/usbh/ps3/DInput.cpp
    ${SRC_DIR}/usbh/ps4/Dualshock4.cpp
    ${SRC_DIR}/usbh/ps5/Dualsense.cpp
    ${SRC_DIR}/usbh/psclassic/PSClassic.cpp
    ${SRC_DIR}/usbh/switch/SwitchPro.cpp
    ${SRC_DIR}/usbh/switch/SwitchWired.cpp
    ${SRC_DIR}/usbh/xinput/XInput.cpp
    ${SRC_DIR}/usbh/shared/hid_class_driver.c
    ${SRC_DIR}/usbh/shared/scaling.cpp
)

# Firmware
add_executable(${NAME} ${SOURCES})

target_include_directories(${NAME} PUBLIC
        ${ROOT}/src 
        ${ROOT}/lib)

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
    cmsis_core
    xinput_host
    tusb_gamepad
)

pico_add_extra_outputs(${NAME})