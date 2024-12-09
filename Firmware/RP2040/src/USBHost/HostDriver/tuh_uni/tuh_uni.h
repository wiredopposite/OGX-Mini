#ifndef _TUH_UNI_CLASS_DRIVER_H_
#define _TUH_UNI_CLASS_DRIVER_H_

#include <cstdint>

#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"

/*  There seems to be a conflict with tinyusb using multiple host class drivers */
namespace tuh_uni 
{
    const usbh_class_driver_t* class_driver();
} // namespace tuh_uni

#endif // _TUH_UNI_CLASS_DRIVER_H_