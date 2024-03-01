#ifndef _HID_HOST_APP_H_
#define _HID_HOST_APP_H_

#include "usbh/tusb_hid/n64usb.h"
#include "usbh/tusb_hid/psclassic.h"
#include "usbh/tusb_hid/ps3.h"
#include "usbh/tusb_hid/ps4.h"
#include "usbh/tusb_hid/ps5.h"
#include "usbh/tusb_hid/switch_pro.h"
#include "usbh/tusb_hid/switch_wired.h"

#ifdef __cplusplus
extern "C" {
#endif

extern usbh_class_driver_t const usbh_hid_driver;

#ifdef __cplusplus
}
#endif

bool send_fb_data_to_hid_gamepad();

#endif // _HID_HOST_APP_H_
