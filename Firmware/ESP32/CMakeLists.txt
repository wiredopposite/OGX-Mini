cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_CURRENT_SOURCE_DIR}/FWDefines.cmake)
set(COMPONENTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/components)
set(BLUEPAD32_ROOT ${COMPONENTS_PATH}/bluepad32/src/components/bluepad32)
set(BTSTACK_REPO_PATH ${COMPONENTS_PATH}/bluepad32/external/btstack)
set(BTSTACK_SCRIPT_PATH ${COMPONENTS_PATH}/integrate_btstack.py)

execute_process(
    COMMAND git apply --ignore-whitespace ${COMPONENTS_PATH}/btstack_l2cap.diff
    WORKING_DIRECTORY ${BTSTACK_REPO_PATH}
    # RESULT_VARIABLE result
    # ERROR_VARIABLE error
)

if(NOT EXISTS ${COMPONENTS_PATH}/btstack)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    if(Python3_Interpreter_FOUND)
        message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
        message(STATUS "Python3 version: ${Python3_VERSION}")
    else()
        message(FATAL_ERROR "Python3 not found! Python is required to integrate BTStack with ESP-IDF")
    endif()

    message(STATUS "Integrating BTStack with project")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${BTSTACK_SCRIPT_PATH} ${BTSTACK_REPO_PATH} ${COMPONENTS_PATH}
        WORKING_DIRECTORY ${COMPONENTS_PATH}
        RESULT_VARIABLE PYTHON_RESULT
        OUTPUT_VARIABLE PYTHON_OUTPUT
        ERROR_VARIABLE PYTHON_ERROR
    )

    if(PYTHON_RESULT EQUAL 0)
        message(STATUS "BTStack integration successful:\n${PYTHON_OUTPUT}")
    else()
        message(FATAL_ERROR "BTStack integration failed:\n${PYTHON_ERROR}")
    endif()
endif()

set(EXTRA_COMPONENT_DIRS 
    ${BLUEPAD32_ROOT}/..
    ${COMPONENTS_PATH}
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project("${FW_NAME}-${FW_VERSION}-ESP32")

