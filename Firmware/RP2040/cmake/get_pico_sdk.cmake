function(get_pico_sdk EXTERNAL_DIR VERSION_TAG)
    if(NOT DEFINED ENV{PICO_SDK_PATH})
        message("PICO_SDK_PATH not set")
        set(PICO_SDK_PATH ${EXTERNAL_DIR}/pico-sdk PARENT_SCOPE)

        if(NOT EXISTS ${PICO_SDK_PATH})
            message("Cloning pico-sdk to ${PICO_SDK_PATH}")
            execute_process(
                COMMAND git clone --recursive https://github.com/raspberrypi/pico-sdk.git
                WORKING_DIRECTORY ${EXTERNAL_DIR}
            )

        endif()

        execute_process(
            COMMAND git fetch --tags
            WORKING_DIRECTORY ENV{PICO_SDK_PATH}
            )
    
        execute_process(
            COMMAND git checkout tags/${VERSION_TAG}
            WORKING_DIRECTORY ENV{PICO_SDK_PATH}
            )
    
        execute_process(
            COMMAND git submodule update --init --recursive 
            WORKING_DIRECTORY ENV{PICO_SDK_PATH}
            )

    else()
        message("Using PICO_SDK_PATH from environment: $ENV{PICO_SDK_PATH}")
        set(PICO_SDK_PATH ENV{PICO_SDK_PATH} PARENT_SCOPE)

    endif()

    set(PICOTOOL_FETCH_FROM_GIT_PATH ${EXTERNAL_DIR}/picotool PARENT_SCOPE)
endfunction()