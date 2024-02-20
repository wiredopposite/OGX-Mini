#ifndef _HID_HOST_APP_H_
#define _HID_HOST_APP_H_

extern usbh_class_driver_t const usbh_hid_driver;

bool hid_gamepad_mounted();
bool send_fb_data_to_hid_gamepad();

#endif // _HID_HOST_APP_H_
