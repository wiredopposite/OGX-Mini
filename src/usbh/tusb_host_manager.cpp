#include "tusb.h"

#include "tusb_xinput/xinput_host_app.h"
#include "tusb_xinput/xinput_host.h"

#include "tusb_hid/hid_host_app.h"
#include "tusb_hid/shared.h"

#include "tusb_host_manager.h"

#include "Gamepad.h"

HostMode host_mode;

bool device_mounted = false;
uint8_t device_daddr = 0;
tusb_desc_interface_t config_descriptor;

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
    // { ps3_devices, sizeof(ps3_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS3 },
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

void config_descriptor_complete_cb(tuh_xfer_t* xfer)
{
    if (xfer->result == XFER_RESULT_SUCCESS)
    {
        if ((config_descriptor.bInterfaceSubClass == 0x5D && config_descriptor.bInterfaceProtocol == 0x81) ||
            (config_descriptor.bInterfaceSubClass == 0x5D && config_descriptor.bInterfaceProtocol == 0x01) ||
            (config_descriptor.bInterfaceSubClass == 0x47 && config_descriptor.bInterfaceProtocol == 0xD0) ||
            (config_descriptor.bInterfaceSubClass == 0x42 && config_descriptor.bInterfaceClass == 0x58))
        {
            host_mode = HOST_MODE_XINPUT;
        }
    }
}

void tuh_mount_cb(uint8_t daddr)
{
    if (!device_mounted)
    {
        device_mounted = true;
        device_daddr = daddr;
    }

    uint16_t vid, pid;
    tuh_vid_pid_get(daddr, &vid, &pid);

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

    // if (!tuh_descriptor_get_configuration(daddr, 0, &config_descriptor, sizeof(config_descriptor), config_descriptor_complete_cb, 0))
    // {
        host_mode = HOST_MODE_HID_MOUSE; // idk
    // }
}

void tuh_umount_cb(uint8_t daddr)
{
    if (device_mounted && device_daddr == daddr)
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

void send_fb_data_to_gamepad()
{
    if (device_mounted)
    {
        if (host_mode == HOST_MODE_XINPUT)
        {
            send_fb_data_to_xinput_gamepad();
        }
        else
        {
            if (send_fb_data_to_hid_gamepad())
                gamepadOut.rumble_hid_reset(); // needed so rumble doesn't get stuck on
        }
    }
}