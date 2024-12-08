#include "c_wrapper.h"
#include "I2CDriver/I2CDriver.h"
#include "Bluepad32/Bluepad32.h"

void run_bluepad32()
{
    bluepad32::run_task();
}

void run_i2c()
{
    I2CDriver i2c_driver;
    i2c_driver.run_task();
}