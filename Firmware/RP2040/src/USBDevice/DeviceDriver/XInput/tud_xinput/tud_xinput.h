#ifndef _TUD_XINPUT_H_
#define _TUD_XINPUT_H_

#include <cstdint>

#include "tusb.h"
#include "device/usbd_pvt.h"

namespace tud_xinput 
{
    bool send_report_ready();
    bool send_report(const uint8_t *report, uint16_t len);
    bool receive_report(uint8_t *report, uint16_t len);
    const usbd_class_driver_t* class_driver();
    
} // namespace tud_xinput

#endif // _TUD_XINPUT_H_