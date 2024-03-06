#include <stdint.h>
#include "pico/stdlib.h"

#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/hid/hid_host.h"

#include "usbh/tusb_hid/shared.h"
#include "usbh/tusb_hid/hid_host_app.h"

#include "usbh/tusb_host_manager.h" // global enum host_mode

// TODO: make a host driver class for all this

N64USB* n64usb = nullptr;
PSClassic* psclassic = nullptr;
Dualshock3* dualshock3 = nullptr;
Dualshock4* dualshock4 = nullptr;
Dualsense* dualsense = nullptr;
SwitchPro* switch_pro = nullptr;
SwitchWired* switch_wired = nullptr;

static bool gamepad_mounted = false;
static uint8_t gamepad_dev_addr = 0;
static uint8_t gamepad_instance = 0;

usbh_class_driver_t const usbh_hid_driver =
{
    .init       = hidh_init,
    .open       = hidh_open,
    .set_config = hidh_set_config,
    .xfer_cb    = hidh_xfer_cb,
    .close      = hidh_close
};

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    (void)desc_report;
    (void)desc_len;

    if (!gamepad_mounted)
    {
        gamepad_dev_addr = dev_addr;
        gamepad_instance = instance;
        gamepad_mounted = true;
    }

    if (host_mode == HOST_MODE_HID_SWITCH_PRO && !switch_pro)
    {
        switch_pro = new SwitchPro();
        switch_pro->init(dev_addr, instance);
    }
    else if (host_mode == HOST_MODE_HID_SWITCH_WIRED && !switch_wired)
    {
        switch_wired = new SwitchWired();
        switch_wired->init(dev_addr, instance);
    }
    else if (host_mode == HOST_MODE_HID_PS3 && !dualshock3)
    {
        dualshock3 = new Dualshock3();
        dualshock3->init(dev_addr, instance);
    }
    else if (host_mode == HOST_MODE_HID_PS4 && !dualshock4)
    {
        dualshock4 = new Dualshock4();
        dualshock4->init(dev_addr, instance);
    }
    else if (host_mode == HOST_MODE_HID_PS5 && !dualsense)
    {
        dualsense = new Dualsense();
        dualsense->init(dev_addr, instance);
    }
    else if (host_mode == HOST_MODE_HID_PSCLASSIC && !psclassic)
    {
        psclassic = new PSClassic();
        psclassic->init(dev_addr, instance);
    }
    else if (host_mode == HOST_MODE_HID_N64USB && !n64usb)
    {
        n64usb = new N64USB();
        n64usb->init(dev_addr, instance);
    }
    
    if (!tuh_hid_receive_report(dev_addr, instance))
    {
        printf("Error: cannot request to receive report\r\n");
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    if (gamepad_mounted && gamepad_dev_addr == dev_addr && gamepad_instance == instance)
    {
        gamepad_mounted = false;

        if (switch_pro) 
        {
            delete switch_pro;
            switch_pro = nullptr;
        }
        if (switch_wired)
        {
            delete switch_wired;
            switch_wired = nullptr;
        }
        if (n64usb)
        {
            delete n64usb;
            n64usb = nullptr;
        }
        if (psclassic)
        {
            delete psclassic;
            psclassic = nullptr;
        }
        if (dualshock3)
        {
            delete dualshock3;
            dualshock3 = nullptr;
        }
        if (dualshock4)
        {
            delete dualshock4;
            dualshock4 = nullptr;
        }
        if (dualsense)
        {
            delete dualsense;
            dualsense = nullptr;
        }
    }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    switch(host_mode)
    {
        case HOST_MODE_HID_PSCLASSIC:
            psclassic->process_report(report, len);
            break;
        case HOST_MODE_HID_PS3:
            dualshock3->process_report(report, len);
            break;
        case HOST_MODE_HID_PS4:
            dualshock4->process_report(report, len);
            break;
        case HOST_MODE_HID_PS5:
            dualsense->process_report(report, len);
            break;
        case HOST_MODE_HID_SWITCH_PRO:
            switch_pro->process_report(report, len);
            break;
        case HOST_MODE_HID_SWITCH_WIRED:
            switch_wired->process_report(report, len);
            break;
        case HOST_MODE_HID_N64USB:
            n64usb->process_report(report, len);
            break;
    }

    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        printf("Error: cannot request to receive report\r\n");
    }
}

// send rumble data
bool send_fb_data_to_hid_gamepad()
{
    if (!gamepad_mounted)
    {
        return false;
    }

    bool rumble_sent = false;

    if (tuh_hid_send_ready)
    {
        switch(host_mode)
        {
            case HOST_MODE_HID_PSCLASSIC:
                rumble_sent = psclassic->send_fb_data();
                break;
            case HOST_MODE_HID_PS3:
                rumble_sent = dualshock3->send_fb_data();
                break;
            case HOST_MODE_HID_PS4:
                rumble_sent = dualshock4->send_fb_data();
                break;
            case HOST_MODE_HID_PS5:
                rumble_sent = dualsense->send_fb_data();
                break;
            case HOST_MODE_HID_SWITCH_PRO:
                rumble_sent = switch_pro->send_fb_data();
                break;
            case HOST_MODE_HID_SWITCH_WIRED:
                rumble_sent = switch_wired->send_fb_data();
                break;
            case HOST_MODE_HID_N64USB:
                rumble_sent = n64usb->send_fb_data();
                break;
        }
    }

    return rumble_sent;
}