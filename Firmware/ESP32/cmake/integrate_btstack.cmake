function(integrate_btstack EXTERNAL_DIR)
    if(NOT EXISTS ${EXTERNAL_DIR})
        message(FATAL_ERROR "External directory not found: ${EXTERNAL_DIR}")
    else()
        message(STATUS "External directory found: ${EXTERNAL_DIR}")
    endif()

    set(PROJECT_COMPONENTS ${CMAKE_CURRENT_LIST_DIR}/components)
    set(BTSTACK_DIR ${EXTERNAL_DIR}/bluepad32/external/btstack)
    set(PROJECT_BTSTACK ${PROJECT_COMPONENTS}/btstack)
    
    if (NOT EXISTS ${PROJECT_COMPONENTS})
        message(STATUS "Creating components folder at ${PROJECT_COMPONENTS}")
        file(MAKE_DIRECTORY ${PROJECT_COMPONENTS})
    endif()
    
    if (NOT EXISTS ${BTSTACK_DIR})
        message(FATAL_ERROR "Error: No BTstack folder at ${BTSTACK_DIR}")
    endif()
    
    if (NOT EXISTS ${PROJECT_BTSTACK})
        find_package(Python3 COMPONENTS Interpreter REQUIRED)

        if(Python3_Interpreter_FOUND)
            message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
            message(STATUS "Python3 version: ${Python3_VERSION}")
        else()
            message(FATAL_ERROR "Python3 not found! Python is required to integrate BTStack with ESP-IDF")
        endif()

        set(BTSTACK_SCRIPT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/components/integrate_btstack.py)

        message(STATUS "Integrating BTStack with project")
        execute_process(
            COMMAND ${Python3_EXECUTABLE} ${BTSTACK_SCRIPT_PATH} ${BTSTACK_DIR} ${PROJECT_COMPONENTS}
            WORKING_DIRECTORY ${BTSTACK_SCRIPT_PATH}/..
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
    
    message(STATUS "BTstack setup complete")
endfunction()