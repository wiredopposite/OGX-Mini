#ifndef _BLUEPAD32_DRIVER_H_
#define _BLUEPAD32_DRIVER_H_

#include <cstdint>

#include "sdkconfig.h"
#include "I2CDriver/I2CDriver.h"
#include "Reports.h"

namespace bluepad32 
{
    void run_task();
    bool connected(uint8_t index); 
    bool any_connected();
    bool new_report_in(uint8_t index);
    ReportIn get_report_in(uint8_t index);
    void set_report_out(const ReportOut& report);
}

#endif // _BLUEPAD32_DRIVER_H_