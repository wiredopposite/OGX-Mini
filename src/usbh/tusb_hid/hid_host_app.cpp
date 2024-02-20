#include <stdint.h>

#include "pico/stdlib.h"

// #include "bsp/board_api.h"
#include "tusb.h"

#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/hid/hid_host.h"

#include "tusb_hid/hid_host_app.h"
#include "tusb_hid/ps4.h"
#include "tusb_hid/ps5.h"

#include "tusb_host_manager.h" // global enum host_mode

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

bool hid_gamepad_mounted()
{
    return gamepad_mounted;
}

// Invoked when device with hid interface is mounted
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

    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if (!tuh_hid_receive_report(dev_addr, instance))
    {
        printf("Error: cannot request to receive report\r\n");
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    if (gamepad_mounted && gamepad_dev_addr == dev_addr && gamepad_instance == instance)
    {
        gamepad_mounted = false;
    }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    switch(host_mode)
    {
        case HOST_MODE_HID_PS4:
            process_dualshock4(report, len);
            break;
        case HOST_MODE_HID_PS5:
            process_dualsense(report, len);
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
    bool rumble_sent = false;

    if (gamepad_mounted)
    {
        switch(host_mode)
        {
            case HOST_MODE_HID_PS4:
                rumble_sent = send_fb_data_to_dualshock4(gamepad_dev_addr, gamepad_instance);
                break;
            case HOST_MODE_HID_PS5:
                rumble_sent = send_fb_data_to_dualsense(gamepad_dev_addr, gamepad_instance);
                break;
        }
    }

    return rumble_sent;
}