cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external)
set(EXTERNAL_CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
set(BTSTACK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/components/btstack)

include(${CMAKE_CURRENT_SOURCE_DIR}/../FWDefines.cmake)
include(${EXTERNAL_CMAKE_DIR}/init_submodules.cmake)
include(${EXTERNAL_CMAKE_DIR}/patch_libs.cmake)
include(${EXTERNAL_CMAKE_DIR}/generate_gatt_header.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/integrate_btstack.cmake)

init_git_submodules(${EXTERNAL_DIR}
    ${EXTERNAL_DIR}/bluepad32
    ${EXTERNAL_DIR}/libfixmath
)
apply_lib_patches(${EXTERNAL_DIR})
integrate_btstack(${EXTERNAL_DIR})
generate_gatt_header(
    ${BTSTACK_ROOT} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../RP2040/src/BLEServer/att_delayed_response.gatt
    ${CMAKE_CURRENT_SOURCE_DIR}/main/BLEServer/att_delayed_response.h
    )

set(BLUEPAD32_ROOT ${EXTERNAL_DIR}/bluepad32/src/components/bluepad32)

if(NOT EXISTS ${BLUEPAD32_ROOT})
    message(FATAL_ERROR "External directory not found: ${BLUEPAD32_ROOT}")
endif()

set(EXTRA_COMPONENT_DIRS 
    ${BLUEPAD32_ROOT}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/components
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project("${FW_NAME}-${FW_VERSION}-ESP32")