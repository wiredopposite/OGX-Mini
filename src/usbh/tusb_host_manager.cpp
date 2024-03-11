#include "tusb.h"

#include "tusb_xinput/xinput_host_app.h"
#include "tusb_xinput/xinput_host.h"

#include "tusb_hid/hid_host_app.h"
#include "tusb_hid/shared.h"

#include "tusb_host_manager.h"

#include "Gamepad.h"

HostMode host_mode;

bool device_mounted;
uint8_t device_daddr = 0;

typedef struct 
{
    const usb_vid_pid_t* devices;
    size_t num_devices;
    HostMode check_mode;
} DeviceTypeInfo;

const DeviceTypeInfo device_types[] = 
{
    { n64_devices, sizeof(n64_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_N64USB },
    { psc_devices, sizeof(psc_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PSCLASSIC },
    { ps3_devices, sizeof(ps3_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS3 },
    { ps4_devices, sizeof(ps4_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS4 },
    { ps5_devices, sizeof(ps5_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS5 },
    { switch_wired_devices, sizeof(switch_wired_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_SWITCH_WIRED },
    { switch_pro_devices, sizeof(switch_pro_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_SWITCH_PRO }
};

bool check_vid_pid(const usb_vid_pid_t* devices, size_t num_devices, HostMode check_mode, uint16_t vid, uint16_t pid) 
{
    for (size_t i = 0; i < num_devices; i++) 
    {
        if (vid == devices[i].vid && pid == devices[i].pid) 
        {
            return true;
        }
    }

    return false;
}

void tuh_mount_cb(uint8_t daddr)
{
    device_mounted = true;
    device_daddr = daddr;

    uint16_t vid, pid;
    tuh_vid_pid_get(daddr, &vid, &pid);

    host_mode = HOST_MODE_XINPUT;

    // set host mode depending on VID/PID match
    const size_t num_device_types = sizeof(device_types) / sizeof(DeviceTypeInfo);

    for (size_t i = 0; i < num_device_types; i++) 
    {
        if (check_vid_pid(device_types[i].devices, device_types[i].num_devices, device_types[i].check_mode, vid, pid)) 
        {
            host_mode = device_types[i].check_mode;
            return;
        }
    }

    return;
}

void tuh_umount_cb(uint8_t daddr)
{
    if (device_mounted)
    {
        device_mounted = false;
    }
}

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count)
{
    *driver_count = 1;

    switch (host_mode)
    {
        case HOST_MODE_XINPUT:
            return &usbh_xinput_driver;
        default:
            return &usbh_hid_driver;
    }
}

bool send_fb_data_to_gamepad()
{
    if (!device_mounted)
        return true;

    if (host_mode == HOST_MODE_XINPUT)
    {
        return send_fb_data_to_xinput_gamepad();
    }
    else if (send_fb_data_to_hid_gamepad())
    {
        gamepadOut.rumble_hid_reset();
        return true;
    }
    
    return false;
}