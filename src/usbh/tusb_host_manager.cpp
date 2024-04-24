#include "hardware/gpio.h"
#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"
#include "tusb_gamepad.h"

#include "usbh/xinput/XInput.h"
#include "usbh/n64usb/N64USB.h"
#include "usbh/psclassic/PSClassic.h"
#include "usbh/ps3/DInput.h"
#include "usbh/ps3/Dualshock3.h"
#include "usbh/ps4/Dualshock4.h"
#include "usbh/ps5/Dualsense.h"
#include "usbh/switch/SwitchPro.h"
#include "usbh/switch/SwitchWired.h"

#include "usbh/shared/hid_class_driver.h"
#include "usbh/tusb_host_manager.h"

#include "ogxm_config.h"

struct HostManager
{
    bool hid = {false};
    bool mounted = {false}; 
    uint8_t address;
    uint8_t instance;
    const usbh_class_driver_t* class_driver = {&usbh_xinput_driver};
    GPHostDriver* gp_host_driver = {nullptr};
};

HostManager host_manager[MAX_GAMEPADS] = {};

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
    { dinput_devices, sizeof(dinput_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_DINPUT},
    { ps3_devices, sizeof(ps3_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS3 },
    { ps4_devices, sizeof(ps4_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS4 },
    { ps5_devices, sizeof(ps5_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_PS5 },
    { switch_wired_devices, sizeof(switch_wired_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_SWITCH_WIRED },
    { switch_pro_devices, sizeof(switch_pro_devices) / sizeof(usb_vid_pid_t), HOST_MODE_HID_SWITCH_PRO }
};

void led_mounted_indicator(bool mounted) 
{
    gpio_put(LED_INDICATOR_PIN, mounted ? 1 : 0);
}

bool check_vid_pid(const usb_vid_pid_t* devices, size_t num_devices, uint16_t vid, uint16_t pid) 
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

void init_gp_host_driver(HostMode host_mode, int idx)
{
    switch(host_mode)
    {
        case HOST_MODE_XINPUT:
            host_manager[idx].gp_host_driver = new XInputHost();
            break;
        case HOST_MODE_HID_PSCLASSIC:
            host_manager[idx].gp_host_driver = new PSClassic();
            break;
        case HOST_MODE_HID_DINPUT:
            host_manager[idx].gp_host_driver = new DInput();
            break;
        case HOST_MODE_HID_PS3:
            host_manager[idx].gp_host_driver = new Dualshock3();
            break;
        case HOST_MODE_HID_PS4:
            host_manager[idx].gp_host_driver = new Dualshock4();
            break;
        case HOST_MODE_HID_PS5:
            host_manager[idx].gp_host_driver = new Dualsense();
            break;
        case HOST_MODE_HID_SWITCH_PRO:
            host_manager[idx].gp_host_driver = new SwitchPro();
            break;
        case HOST_MODE_HID_SWITCH_WIRED:
            host_manager[idx].gp_host_driver = new SwitchWired();
            break;
        case HOST_MODE_HID_N64USB:
            host_manager[idx].gp_host_driver = new N64USB();
            break;
        default:
            break;
    }

    if (host_manager[idx].gp_host_driver) 
    {
        host_manager[idx].gp_host_driver->init((uint8_t)idx + 1, host_manager[idx].address, host_manager[idx].instance);
    }
}

int find_free_slot()
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (!host_manager[i].mounted)
        {
            host_manager[i].mounted = true;
            return i;
        }
    }

    return -1;
}

void unmount_gamepad(uint8_t dev_addr, uint8_t instance)
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (host_manager[i].mounted && 
            host_manager[i].address == dev_addr && 
            host_manager[i].instance == instance)
        {
            host_manager[i].mounted = false;

            gamepad(i)->reset_pad(gamepad(i));
            gamepad(i)->enable_analog_buttons = false;
            
            if (host_manager[i].gp_host_driver)
            {
                delete host_manager[i].gp_host_driver;
                host_manager[i].gp_host_driver = nullptr;
            }
        }
    }

    bool all_free = true;

    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (host_manager[i].gp_host_driver != nullptr)
        {
            all_free = false;
            break;
        }
    }

    if (all_free) 
    {
        led_mounted_indicator(false); 
    }
}

/* ----------- TUSB ----------- */

void tuh_mount_cb(uint8_t daddr)
{
    (void)daddr;

    led_mounted_indicator(true);
}

void tuh_umount_cb(uint8_t daddr) 
{
    (void)daddr;
}

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count)
{
    *driver_count = 1;

    return host_manager[0].class_driver;
}

/* ----------- HID ----------- */

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    (void)desc_report;
    (void)desc_len;

    int slot = find_free_slot();

    if (slot < 0) return; // no available slots, shouldn't happen

    host_manager[slot].address = dev_addr;
    host_manager[slot].instance = instance;

    uint16_t vid, pid;

    tuh_vid_pid_get(dev_addr, &vid, &pid);

    const size_t num_device_types = sizeof(device_types) / sizeof(DeviceType); // set host mode depending on VID/PID match

    for (size_t i = 0; i < num_device_types; i++) 
    {
        if (check_vid_pid(device_types[i].devices, device_types[i].num_devices, vid, pid)) 
        {
            host_manager[slot].hid = true;

            init_gp_host_driver(device_types[i].check_mode, slot);

            if (device_types[i].check_mode == HOST_MODE_HID_DINPUT || 
                device_types[i].check_mode == HOST_MODE_HID_PS3)
            {
                gamepad(slot)->enable_analog_buttons = true;
            }

            break;
        }
    }

    // tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    unmount_gamepad(dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (host_manager[i].mounted && 
            host_manager[i].address == dev_addr && 
            host_manager[i].instance == instance)
        {
            if (host_manager[i].gp_host_driver)
            {
                host_manager[i].gp_host_driver->process_hid_report(gamepad(i), dev_addr, instance, report, len);
            }
        }
    }
}

// Current version of TinyUSB may be bugged so omitting this for now

// void tuh_hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
// {
//     for (int i = 0; i < MAX_GAMEPADS; i++)
//     {
//         if (host_manager[i].mounted && 
//             host_manager[i].address == dev_addr && 
//             host_manager[i].instance == instance)
//         {
//             host_manager[i].gp_host_driver->hid_get_report_complete_cb(dev_addr, instance, report_id, report_type, len);
//         }
//     }
// }

/* ----------- XINPUT ----------- */

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
    (void)xinput_itf;

    int slot = find_free_slot();

    if (slot < 0) return; // no available slots

    host_manager[slot].address = dev_addr;
    host_manager[slot].instance = instance;

    init_gp_host_driver(HOST_MODE_XINPUT, slot);

    if (xinput_itf->type == XBOXOG)
    {
        gamepad(slot)->enable_analog_buttons = true;
    }

    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    unmount_gamepad(dev_addr, instance);
}

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len)
{
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (host_manager[i].mounted && host_manager[i].address == dev_addr && host_manager[i].instance == instance)
        {
            if (host_manager[i].gp_host_driver)
            {
                host_manager[i].gp_host_driver->process_xinput_report(gamepad(i), dev_addr, instance, report, len);
            }
        }
    }
     
    tuh_xinput_receive_report(dev_addr, instance);
}

/* ----------- SEND FEEDBACK ----------- */

void reset_hid_rumble(Gamepad* gp)
{
    if (gp->rumble.l != UINT8_MAX)
    {
        gp->rumble.l = 0;
    }
    
    if (gp->rumble.r != UINT8_MAX)
    {
        gp->rumble.r = 0;
    }
}

void send_fb_data_to_gamepad()
{
    static const uint8_t fb_interval_ms = 100;
    static unsigned long fb_sent_time[MAX_GAMEPADS] = {to_ms_since_boot(get_absolute_time())};
    
    for (int i = 0; i < MAX_GAMEPADS; i++)
    {
        if (!host_manager[i].mounted || !host_manager[i].gp_host_driver) break;

        unsigned long current_time = to_ms_since_boot(get_absolute_time());

        if (current_time - fb_sent_time[i] >= fb_interval_ms) 
        {
            if (host_manager[i].gp_host_driver->send_fb_data(gamepad(i), host_manager[i].address, host_manager[i].instance))
            {
                fb_sent_time[i] = current_time;

                if (host_manager[i].hid)
                {
                    reset_hid_rumble(gamepad(i));
                }
            }
        }
    }
}