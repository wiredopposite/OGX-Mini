#include <array>

#include "tusb_option.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/tuh_uni/tuh_uni.h"
#include "USBHost/HostDriver/Xinput/tuh_xinput/tuh_xinput.h"

namespace tuh_uni {

enum class HostType { UNKNOWN = 0, HID, XINPUT };

static std::array<HostType, CFG_TUH_DEVICE_MAX> host_type_;
// const usbh_class_driver_t* xinput_class_driver_ = nullptr;

bool init()
{
    host_type_.fill(HostType::UNKNOWN);
    hidh_init();
    tuh_xinput::class_driver()->init();
    return true;
}

bool deinit()
{
    hidh_deinit();
    tuh_xinput::class_driver()->deinit();
    host_type_.fill(HostType::UNKNOWN);
    return true;
}

bool open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const* desc_itf, uint16_t max_len)
{
    if (tuh_xinput::class_driver()->open(rhport, dev_addr, desc_itf, max_len))
    {
        host_type_[dev_addr - 1] = HostType::XINPUT;
        return true;
    }

    host_type_[dev_addr - 1] = HostType::HID;
    return hidh_open(rhport, dev_addr, desc_itf, max_len);;
}

bool set_config(uint8_t dev_addr, uint8_t itf_num)
{
    switch (host_type_[dev_addr - 1])
    {
        case HostType::HID:
            return hidh_set_config(dev_addr, itf_num);
        case HostType::XINPUT:
            return tuh_xinput::class_driver()->set_config(dev_addr, itf_num);
        default:
            return false;
    }
}

bool xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    switch (host_type_[dev_addr - 1])
    {
        case HostType::HID:
            return hidh_xfer_cb(dev_addr, ep_addr, result, xferred_bytes);
        case HostType::XINPUT:
            return tuh_xinput::class_driver()->xfer_cb(dev_addr, ep_addr, result, xferred_bytes);
        default:
            return false;
    }
}

void close(uint8_t dev_addr)
{
    switch (host_type_[dev_addr - 1])
    {
        case HostType::HID:
            hidh_close(dev_addr);
            break;
        case HostType::XINPUT:
            tuh_xinput::class_driver()->close(dev_addr);
            break;
        default:
            break;
    }
}

const usbh_class_driver_t* class_driver()
{
    static const usbh_class_driver_t class_driver = {
        .init = init,
        .deinit = deinit,
        .open = open,
        .set_config = set_config,
        .xfer_cb = xfer_cb,
        .close = close
    };
    return &class_driver;
}

} // namespace tuh_uni