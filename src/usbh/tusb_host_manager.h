#ifndef _TUSB_HOST_MANAGER_H_
#define _TUSB_HOST_MANAGER_H_

typedef enum
{
    HOST_MODE_XINPUT,
    HOST_MODE_HID_PS3,
    HOST_MODE_HID_PS4,
    HOST_MODE_HID_PS5
} HostMode;

extern HostMode host_mode;

// bool tuh_device_mounted();
void send_fb_data_to_gamepad();

#endif // _TUSB_HOST_MANAGER_H_