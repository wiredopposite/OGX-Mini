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
set(TUSB_XINPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/tusb_xinput")
add_subdirectory(${TUSB_XINPUT_PATH} xinput_host)

pico_sdk_init()

add_subdirectory(lib/Pico-PIO-USB)
add_subdirectory(lib)

file(GLOB_RECURSE SOURCES 
"src/main.cpp"
"src/input_mode.cpp"
"src/Gamepad.cpp"

"src/utilities/log.cpp"

"src/usbh/tusb_host_manager.cpp"
"src/usbh/tusb_host.cpp"
"src/usbh/n64usb/N64USB.cpp"
"src/usbh/ps3/Dualshock3.cpp"
"src/usbh/ps3/DInput.cpp"
"src/usbh/ps4/Dualshock4.cpp"
"src/usbh/ps5/Dualsense.cpp"
"src/usbh/psclassic/PSClassic.cpp"
"src/usbh/switch/SwitchPro.cpp"
"src/usbh/switch/SwitchWired.cpp"
"src/usbh/xinput/XInput.cpp"
"src/usbh/shared/hid_class_driver.c"
"src/usbh/shared/scaling.cpp" 

"src/usbd/usbdriver.cpp"
"src/usbd/drivermanager.cpp"
"src/usbd/drivers/shared/driverhelper.h"
"src/usbd/drivers/shared/scaling.cpp"
"src/usbd/drivers/dinput/DInputDriver.cpp"
# "src/usbd/drivers/hid/HIDDriver.cpp"
# "src/usbd/drivers/ps3/PS3Driver.cpp"
"src/usbd/drivers/psclassic/PSClassicDriver.cpp"
"src/usbd/drivers/switch/SwitchDriver.cpp"
"src/usbd/drivers/usbserial/USBSerialDriver.cpp"
"src/usbd/drivers/xinput/XInputDriver.cpp"
"src/usbd/drivers/xboxog/XboxOriginalDriver.cpp"
"src/usbd/drivers/xboxog/xid/xid.c"
"src/usbd/drivers/xboxog/xid/xid_driver.c"
"src/usbd/drivers/xboxog/xid/xid_gamepad.c"
"src/usbd/drivers/xboxog/xid/xid_remote.c"
"src/usbd/drivers/xboxog/xid/xid_steelbattalion.c"
)

# Firmware
add_executable(${NAME}
${SOURCES}
    )

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR}/lib)

include_directories(
lib/)

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
)

pico_add_extra_outputs(${NAME})
