function(init_git_submodules EXTERNAL_DIR)
    set(REPO_DIR "${EXTERNAL_DIR}/../../")
    message(STATUS "Initializing submodules in ${REPO_DIR}")

    execute_process(
        COMMAND git submodule update --init --recursive
        WORKING_DIRECTORY ${REPO_DIR}
        RESULT_VARIABLE INIT_SUBMODULES_RESULT
        OUTPUT_VARIABLE INIT_SUBMODULES_OUTPUT
        ERROR_VARIABLE INIT_SUBMODULES_ERROR
    )

    if(INIT_SUBMODULES_RESULT EQUAL 0)
        message(STATUS "Subnmodules initialized successfully.")
    else()
        message(FATAL_ERROR "Failed to initialize submodules: ${INIT_SUBMODULES_ERROR}")
    endif()
endfunction()