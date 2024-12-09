#include <cstdint>
#include <atomic>
#include <pico/stdlib.h>

#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/tuh_uni/tuh_uni.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostManager.h"
#include "OGXMini/OGXMini.h"
#include "Board/board_api.h"

#if defined(CONFIG_EN_4CH)
#include "I2CDriver/I2CDriver.h"
#endif //defined(CONFIG_EN_4CH)

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count)
{
    *driver_count = 1;
    // return tuh_xinput::class_driver();
    return tuh_uni::class_driver();
}

//HID

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    HostManager& host_manager = HostManager::get_instance();
    if (host_manager.setup_driver(HostManager::get_type({ vid, pid }), dev_addr, instance, desc_report, desc_len))
    {
#if defined(CONFIG_EN_4CH)
        i2c_driver::notify_tuh_mounted();
#endif //defined(CONFIG_EN_4CH)
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    board_api::set_led(false);
    HostManager& host_manager = HostManager::get_instance();
    host_manager.deinit_driver(HostManager::DriverClass::HID, dev_addr, instance);

#if defined(CONFIG_EN_4CH)
    i2c_driver::notify_tuh_unmounted();
#endif //defined(CONFIG_EN_4CH)
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    HostManager::get_instance().process_report(HostManager::DriverClass::HID, dev_addr, instance, report, len);
}

//XINPUT

void tuh_xinput::mount_cb(uint8_t dev_addr, uint8_t instance, const tuh_xinput::Interface* interface)
{
    HostManager& host_manager = HostManager::get_instance();
    HostDriver::Type host_type = HostManager::get_type(interface->dev_type);

    if (host_manager.setup_driver(host_type, dev_addr, instance))
    {
#if defined(CONFIG_EN_4CH)
        i2c_driver::notify_tuh_mounted(host_type);
#endif //defined(CONFIG_EN_4CH)
    }
}

void tuh_xinput::unmount_cb(uint8_t dev_addr, uint8_t instance, const tuh_xinput::Interface* interface)
{
    HostManager& host_manager = HostManager::get_instance();
    host_manager.deinit_driver(HostManager::DriverClass::XINPUT, dev_addr, instance);

#if defined(CONFIG_EN_4CH)
    i2c_driver::notify_tuh_unmounted(host_manager.get_type(interface->dev_type));
#endif //defined(CONFIG_EN_4CH)
}

void tuh_xinput::report_received_cb(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len)
{
    HostManager::get_instance().process_report(HostManager::DriverClass::XINPUT, dev_addr, instance, report, len);
}

#if defined(CONFIG_EN_4CH)
void tuh_xinput::xbox360w_connect_cb(uint8_t dev_addr, uint8_t instance)
{
    uint8_t idx = HostManager::get_instance().get_gamepad_idx(HostManager::DriverClass::XINPUT, dev_addr, instance);
    i2c_driver::notify_xbox360w_connected(idx);
}

void tuh_xinput::xbox360w_disconnect_cb(uint8_t dev_addr, uint8_t instance)
{
    uint8_t idx = HostManager::get_instance().get_gamepad_idx(HostManager::DriverClass::XINPUT, dev_addr, instance);
    i2c_driver::notify_xbox360w_disconnected(idx);
}
#endif //defined(CONFIG_EN_4CH)
