idf_component_register( 
    SRCS    
        "main.c" 
        "c_wrapper.cpp"
        "Board/board_api.cpp"
        "Bluepad32/Bluepad32.cpp"
        "I2CDriver/I2CDriver.cpp"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        bluepad32 
        btstack 
        driver 
        nvs_flash
)