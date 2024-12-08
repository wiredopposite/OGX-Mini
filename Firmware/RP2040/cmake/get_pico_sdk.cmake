function(get_pico_sdk EXTERNAL_DIR)
    if(NOT DEFINED ENV{PICO_SDK_PATH})
        message("PICO_SDK_PATH not set, downloading Pico SDK...")
        set(PICO_SDK_PATH ${EXTERNAL_DIR}/pico-sdk PARENT_SCOPE)
        if(NOT EXISTS ${PICO_SDK_PATH})
            execute_process(
                COMMAND git clone --recursive https://github.com/raspberrypi/pico-sdk.git
                WORKING_DIRECTORY ${EXTERNAL_DIR}
            )
        endif()
    else()
        message("Using PICO_SDK_PATH from environment: $ENV{PICO_SDK_PATH}")
        set(PICO_SDK_PATH $ENV{PICO_SDK_PATH} PARENT_SCOPE)
    endif()

    set(PICOTOOL_FETCH_FROM_GIT_PATH ${EXTERNAL_DIR}/picotool PARENT_SCOPE)
endfunction()