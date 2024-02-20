#include "tusb.h"

#include "tusb_xinput/xinput_host_app.h"
#include "tusb_xinput/xinput_host.h"

#include "tusb_hid/hid_host_app.h"
#include "tusb_hid/ps4.h"
#include "tusb_hid/ps5.h"

#include "tusb_host_manager.h"

#include "Gamepad.h"

HostMode host_mode;

bool device_mounted = false;
uint8_t device_daddr = 0;

// HostMode get_host_mode()
// {
//     return host_mode;
// }

// not sure why this and tuh_mounted are always true
// unused atm, come back to it
// bool tuh_device_mounted()
// {
//     return device_mounted;
// }

void tuh_mount_cb(uint8_t daddr)
{
    if (!device_mounted)
    {
        device_mounted = true;
        device_daddr = daddr;
    }

    uint16_t vid, pid;
    tuh_vid_pid_get(daddr, &vid, &pid);

    // lists of device VIP/PIDs
    const size_t num_ps4_devices = sizeof(ps4_devices) / sizeof(usb_vid_pid_t);
    const size_t num_ps5_devices = sizeof(ps5_devices) / sizeof(usb_vid_pid_t);

    host_mode = HOST_MODE_XINPUT;

    // check if VID/PID match any in playstation devices
    for (size_t i = 0; i < num_ps4_devices || i < num_ps5_devices; i++)
    {
        if (i < num_ps4_devices && vid == ps4_devices[i].vid && pid == ps4_devices[i].pid)
        {
            host_mode = HOST_MODE_HID_PS4;
            break;
        }
        if (i < num_ps5_devices && vid == ps5_devices[i].vid && pid == ps5_devices[i].pid)
        {
            host_mode = HOST_MODE_HID_PS5;
            break;
        }
    }
}

// void tuh_umount_cb(uint8_t daddr)
// {
//     if (device_mounted && device_daddr == daddr)
//     {
//         device_mounted = false;
//     }
// }

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
    bool rumble_sent = false;

    if (host_mode == HOST_MODE_XINPUT)
    {
        rumble_sent = send_fb_data_to_xinput_gamepad();
    }
    else
    {
        rumble_sent = send_fb_data_to_hid_gamepad();

        // needed so rumble doesn't get stuck on
        // breaks xinput rumble
        if (rumble_sent && (gamepadOut.out_state.lrumble != UINT8_MAX || gamepadOut.out_state.rrumble != UINT8_MAX))
            {
                gamepadOut.out_state.lrumble = 0;
                gamepadOut.out_state.rrumble = 0;
            }
    }
}