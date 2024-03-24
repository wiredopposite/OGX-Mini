#include "hardware/gpio.h"
#include "tusb.h"
#include "host/usbh.h"

#include "usbh/xinput/XInput.h"
#include "usbh/n64usb/N64USB.h"
#include "usbh/psclassic/PSClassic.h"
#include "usbh/ps3/Dualshock3.h"
#include "usbh/ps4/Dualshock4.h"
#include "usbh/ps5/Dualsense.h"
#include "usbh/switch/SwitchPro.h"
#include "usbh/switch/SwitchWired.h"

#include "usbh/shared/hid_class_driver.h"
#include "usbh/tusb_host_manager.h"

#include "board_config.h"
#include "Gamepad.h"

struct USBDevice
{
    bool mounted = {false};
    uint8_t address;
    bool hid_class = {false};
    bool class_mounted = {false}; 
    uint8_t class_address;
    uint8_t class_instance;
    const usbh_class_driver_t* class_driver = {&usbh_xinput_driver};
    GPHostDriver* gamepad_driver = {nullptr};
};

USBDevice usb_device = {};

typedef struct 
{
    const usb_vid_pid_t* devices;
    size_t num_devices;
    HostMode check_mode;
} DeviceType;

const DeviceType device_types[] = 
{
    { n64_devices, sizeof(n64_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_N64USB },
    { psc_devices, sizeof(psc_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PSCLASSIC },
    // { ps3_devices, sizeof(ps3_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS3 },
    { ps4_devices, sizeof(ps4_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS4 },
    { ps5_devices, sizeof(ps5_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS5 },
    { switch_wired_devices, sizeof(switch_wired_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_SWITCH_WIRED },
    { switch_pro_devices, sizeof(switch_pro_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_SWITCH_PRO }
};

void led_mounted_indicator(bool mounted) 
{
    gpio_put(LED_INDICATOR_PIN, mounted ? 1 : 0);
}

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

void create_gamepad_driver(HostMode host_mode)
{
    switch(host_mode)
    {
        case HOST_MODE_XINPUT:
            usb_device.gamepad_driver = new XInputHost();
            break;
        case HOST_MODE_HID_PSCLASSIC:
            usb_device.gamepad_driver = new PSClassic();
            break;
        case HOST_MODE_HID_PS3:
            usb_device.gamepad_driver = new Dualshock3();
            break;
        case HOST_MODE_HID_PS4:
            usb_device.gamepad_driver = new Dualshock4();
            break;
        case HOST_MODE_HID_PS5:
            usb_device.gamepad_driver = new Dualsense();
            break;
        case HOST_MODE_HID_SWITCH_PRO:
            usb_device.gamepad_driver = new SwitchPro();
            break;
        case HOST_MODE_HID_SWITCH_WIRED:
            usb_device.gamepad_driver = new SwitchWired();
            break;
        case HOST_MODE_HID_N64USB:
            usb_device.gamepad_driver = new N64USB();
            break;
    }

    if (usb_device.gamepad_driver) 
    {
        usb_device.gamepad_driver->init(usb_device.class_address, usb_device.class_instance);
    }
}

/* ----------- TUSB ----------- */

void tuh_mount_cb(uint8_t daddr)
{
    led_mounted_indicator(true);
    usb_device.mounted = true;
    usb_device.address = daddr;
}

void tuh_umount_cb(uint8_t daddr)
{
    led_mounted_indicator(false); 
    usb_device.mounted = false;
    gamepad.reset_state();
}

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count)
{
    *driver_count = 1;

    return usb_device.class_driver;
}

/* ----------- HID ----------- */

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    (void)desc_report;
    (void)desc_len;

    usb_device.class_address = dev_addr;
    usb_device.class_instance = instance;
    usb_device.class_mounted = true;

    HostMode host_mode;
    uint16_t vid, pid;

    tuh_vid_pid_get(dev_addr, &vid, &pid);

    const size_t num_device_types = sizeof(device_types) / sizeof(DeviceType); // set host mode depending on VID/PID match

    for (size_t i = 0; i < num_device_types; i++) 
    {
        if (check_vid_pid(device_types[i].devices, device_types[i].num_devices, device_types[i].check_mode, vid, pid)) 
        {
            usb_device.hid_class = true;
            usb_device.class_driver = &usbh_hid_driver; // I don't think we have to do this
            host_mode = device_types[i].check_mode;
            create_gamepad_driver(host_mode);
            break;
        }
    }

    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    if (usb_device.class_mounted && usb_device.class_address == dev_addr && usb_device.class_instance == instance)
    {
        usb_device.class_mounted = false;
    }

    if (usb_device.gamepad_driver)
    {
        delete usb_device.gamepad_driver;
        usb_device.gamepad_driver = nullptr;
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    if (usb_device.gamepad_driver)
    {
        usb_device.gamepad_driver->process_hid_report(gamepad, dev_addr, instance, report, len);
    }

    tuh_hid_receive_report(dev_addr, instance);
}

/* ----------- XINPUT ----------- */

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
    usb_device.class_mounted = true;
    usb_device.class_address = dev_addr;
    usb_device.class_instance = instance;

    create_gamepad_driver(HOST_MODE_XINPUT);

    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    if (usb_device.class_mounted && usb_device.class_address == dev_addr && usb_device.class_instance == instance)
    {
        usb_device.class_mounted = false;
    }

    if (usb_device.gamepad_driver)
    {
        delete usb_device.gamepad_driver;
        usb_device.gamepad_driver = nullptr;
    }
}

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len)
{
    if (usb_device.gamepad_driver)
    {
        usb_device.gamepad_driver->process_xinput_report(gamepad, dev_addr, instance, report, len);
    }
     
    tuh_xinput_receive_report(dev_addr, instance);
}

/* ----------- SEND FEEDBACK ----------- */

void send_fb_data_to_gamepad()
{
    if (!usb_device.class_mounted || !usb_device.gamepad_driver)
        return;

    static const uint8_t fb_interval_ms = 100;
    static unsigned long fb_sent_time = to_ms_since_boot(get_absolute_time());
    unsigned long current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - fb_sent_time >= fb_interval_ms) 
    {
        if (usb_device.gamepad_driver->send_fb_data(gamepadOut, usb_device.class_address, usb_device.class_instance))
        {
            fb_sent_time = current_time;

            if (usb_device.hid_class)
            {
                gamepadOut.rumble_hid_reset(); // reset rumble so it doesn't get stuck on
            }
        }
    }
}