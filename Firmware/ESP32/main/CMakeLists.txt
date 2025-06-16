include(${CMAKE_CURRENT_SOURCE_DIR}/../FWDefines.cmake)

idf_component_register( 
    SRCS    
        "main.c" 
        "log/log.c"
        "wired/wired_i2c.c"
        "wired/wired_spi.c"
        "led/led.c"
        "settings/nvs.c"
        "settings/settings.cpp"
        "settings/button_combo.c"
        "bluetooth/bluetooth.c"
        "periph/i2c_master.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        bluepad32 
        btstack 
        nvs_flash 
        libfixmath
)

target_compile_definitions(${COMPONENT_LIB} 
    PRIVATE
        FIRMWARE_NAME=\"${FW_NAME}\"
        FIRMWARE_VERSION=\"${FW_VERSION}\"
)