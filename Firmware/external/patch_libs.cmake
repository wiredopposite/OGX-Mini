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

    # set(TUSB_PATCH "${EXTERNAL_DIR}/patches/tinyusb_disable_hidh.diff")
    # set(TUSB_PATH "${EXTERNAL_DIR}/tinyusb")

    # message(STATUS "Applying tinyusb patch: ${TUSB_PATCH}")

    # execute_process(
    #     COMMAND git apply --ignore-whitespace ${TUSB_PATCH}
    #     WORKING_DIRECTORY ${TUSB_PATH}
    #     RESULT_VARIABLE TUSB_PATCH_RESULT
    #     OUTPUT_VARIABLE TUSB_PATCH_OUTPUT
    #     ERROR_VARIABLE TUSB_PATCH_ERROR
    # )

    # if (TUSB_PATCH_RESULT EQUAL 0)
    #     message(STATUS "tinyusb patch applied successfully.")
    # elseif (TUSB_PATCH_ERROR MATCHES "patch does not apply")
    #     message(STATUS "tinyusb patch already applied.")
    # else ()
    #     message(FATAL_ERROR "Failed to apply tinyusb patch: ${TUSB_PATCH_ERROR}")
    # endif ()
endfunction()