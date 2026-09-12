function(apply_lib_patches EXTERNAL_DIR)
    set(BTSTACK_PATCH "${EXTERNAL_DIR}/patches/btstack_l2cap.diff")
    set(BTSTACK_PATH "${EXTERNAL_DIR}/bluepad32/external/btstack")

    message(STATUS "Applying BTStack patch: ${BTSTACK_PATCH}")

    execute_process(
        COMMAND git apply --ignore-whitespace ${BTSTACK_PATCH}
        WORKING_DIRECTORY ${BTSTACK_PATH}
        RESULT_VARIABLE BTSTACK_PATCH_RESULT
        OUTPUT_VARIABLE BTSTACK_PATCH_OUTPUT
        ERROR_VARIABLE BTSTACK_PATCH_ERROR
    )

    if (BTSTACK_PATCH_RESULT EQUAL 0)
        message(STATUS "BTStack patch applied successfully.")
    elseif (BTSTACK_PATCH_ERROR MATCHES "patch does not apply")
        message(STATUS "BTStack patch already applied.")
    else ()
        message(FATAL_ERROR "Failed to apply BTStack patch: ${BTSTACK_PATCH_ERROR}")
    endif ()

    set(BTSTACK_HIDS_REPORTS_PATCH "${EXTERNAL_DIR}/patches/btstack_hids_num_reports.diff")
    message(STATUS "Applying BTStack HIDS reports patch: ${BTSTACK_HIDS_REPORTS_PATCH}")
    execute_process(
        COMMAND git apply --ignore-whitespace ${BTSTACK_HIDS_REPORTS_PATCH}
        WORKING_DIRECTORY ${BTSTACK_PATH}
        RESULT_VARIABLE BTSTACK_HIDS_REPORTS_RESULT
        OUTPUT_VARIABLE BTSTACK_HIDS_REPORTS_OUTPUT
        ERROR_VARIABLE BTSTACK_HIDS_REPORTS_ERROR
    )
    if (BTSTACK_HIDS_REPORTS_RESULT EQUAL 0)
        message(STATUS "BTStack HIDS reports patch applied successfully.")
    elseif (BTSTACK_HIDS_REPORTS_ERROR MATCHES "patch does not apply")
        message(STATUS "BTStack HIDS reports patch already applied.")
    else ()
        message(FATAL_ERROR "Failed to apply BTStack HIDS reports patch: ${BTSTACK_HIDS_REPORTS_ERROR}")
    endif ()

    set(BLUEPAD32_PATCH "${EXTERNAL_DIR}/patches/bluepad32_uni.diff")
    set(BLUEPAD32_PATH "${EXTERNAL_DIR}/bluepad32")

    message(STATUS "Applying Bluepad32 patch: ${BLUEPAD32_PATCH}")

    execute_process(
        COMMAND git apply --ignore-whitespace ${BLUEPAD32_PATCH}
        WORKING_DIRECTORY ${BLUEPAD32_PATH}
        RESULT_VARIABLE BLUEPAD32_PATCH_RESULT
        OUTPUT_VARIABLE BLUEPAD32_PATCH_OUTPUT
        ERROR_VARIABLE BLUEPAD32_PATCH_ERROR
    )

    if (BLUEPAD32_PATCH_RESULT EQUAL 0)
        message(STATUS "Bluepad32 patch applied successfully.")
    elseif (BLUEPAD32_PATCH_ERROR MATCHES "patch does not apply")
        message(STATUS "Bluepad32 patch already applied.")
    else ()
        message(FATAL_ERROR "Failed to apply Bluepad32 patch: ${BLUEPAD32_PATCH_ERROR}")
    endif ()

    set(BLUEPAD32_8BITDO_PATCH "${EXTERNAL_DIR}/patches/bluepad32_8bitdo_pids.diff")
    message(STATUS "Applying Bluepad32 8BitDo PID patch: ${BLUEPAD32_8BITDO_PATCH}")
    execute_process(
        COMMAND git apply --ignore-whitespace ${BLUEPAD32_8BITDO_PATCH}
        WORKING_DIRECTORY ${BLUEPAD32_PATH}
        RESULT_VARIABLE BLUEPAD32_8BITDO_RESULT
        OUTPUT_VARIABLE BLUEPAD32_8BITDO_OUTPUT
        ERROR_VARIABLE BLUEPAD32_8BITDO_ERROR
    )
    if (BLUEPAD32_8BITDO_RESULT EQUAL 0)
        message(STATUS "Bluepad32 8BitDo PID patch applied successfully.")
    elseif (BLUEPAD32_8BITDO_ERROR MATCHES "patch does not apply")
        message(STATUS "Bluepad32 8BitDo PID patch already applied.")
    else ()
        message(FATAL_ERROR "Failed to apply Bluepad32 8BitDo PID patch: ${BLUEPAD32_8BITDO_ERROR}")
    endif ()

    # Pico SDK 2.1.x still lists BTstack's old hids_client.c; Bluepad32's BTstack
    # v1.8 renamed it to hids_host.c. Patch the SDK cmake when using that tree.
    set(PICO_SDK_HIDS_PATCH "${EXTERNAL_DIR}/patches/pico_sdk_hids_host.diff")
    set(PICO_SDK_PATH_LOCAL "${EXTERNAL_DIR}/pico-sdk")
    if (EXISTS "${PICO_SDK_PATH_LOCAL}/src/rp2_common/pico_btstack/CMakeLists.txt")
        message(STATUS "Applying Pico SDK HIDS host patch: ${PICO_SDK_HIDS_PATCH}")
        execute_process(
            COMMAND git apply --ignore-whitespace ${PICO_SDK_HIDS_PATCH}
            WORKING_DIRECTORY ${PICO_SDK_PATH_LOCAL}
            RESULT_VARIABLE PICO_SDK_HIDS_PATCH_RESULT
            OUTPUT_VARIABLE PICO_SDK_HIDS_PATCH_OUTPUT
            ERROR_VARIABLE PICO_SDK_HIDS_PATCH_ERROR
        )
        if (PICO_SDK_HIDS_PATCH_RESULT EQUAL 0)
            message(STATUS "Pico SDK HIDS host patch applied successfully.")
        elseif (PICO_SDK_HIDS_PATCH_ERROR MATCHES "patch does not apply")
            message(STATUS "Pico SDK HIDS host patch already applied.")
        else ()
            message(FATAL_ERROR "Failed to apply Pico SDK HIDS host patch: ${PICO_SDK_HIDS_PATCH_ERROR}")
        endif ()
    endif ()

endfunction()
