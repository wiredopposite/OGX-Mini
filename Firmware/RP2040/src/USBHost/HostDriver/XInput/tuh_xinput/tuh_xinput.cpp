#include "tusb_option.h"

#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_XINPUT)

#include <cstring>

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput_cmd.h"

namespace tuh_xinput {

static constexpr uint8_t MAX_INTERFACES = CFG_TUH_XINPUT * 2;
static constexpr uint8_t MAX_DEVICES = CFG_TUH_DEVICE_MAX;
static constexpr uint32_t KEEPALIVE_MS = 1000;

struct Device
{
    std::array<Interface, MAX_INTERFACES> interfaces;

    Device()
    {
        interfaces.fill(Interface());
    }
};

std::array<Device, MAX_DEVICES> devices_;

TU_ATTR_ALWAYS_INLINE static inline Device* get_device_by_addr(uint8_t dev_addr)
{
    TU_VERIFY((dev_addr <= devices_.size() && dev_addr > 0), nullptr);
    return &devices_[dev_addr - 1];
}

TU_ATTR_ALWAYS_INLINE static inline Interface* get_itf_by_instance(uint8_t dev_addr, uint8_t instance)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, nullptr);
    TU_VERIFY(instance < device->interfaces.size(), nullptr);
    return &device->interfaces[instance];
}

TU_ATTR_ALWAYS_INLINE static inline Interface* get_free_interface(uint8_t dev_addr)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, nullptr);

    for (auto& interface : device->interfaces)
    {
        if (interface.itf_num == 0xFF)
        {
            return &interface;
        }
    }
    return nullptr;
}

TU_ATTR_ALWAYS_INLINE static inline uint8_t get_instance_by_itf_num(uint8_t dev_addr, uint8_t itf_num)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, 0xFF);

    for (uint8_t i = 0; i < device->interfaces.size(); ++i)
    {
        if (device->interfaces[i].itf_num == itf_num)
        {
            return i;
        }
    }
    return 0xFF;
}

TU_ATTR_ALWAYS_INLINE static inline uint8_t get_instance_by_ep(uint8_t dev_addr, uint8_t ep_addr)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, 0xFF);
    for (uint8_t i = 0; i < device->interfaces.size(); ++i)
    {
        if (device->interfaces[i].ep_in == ep_addr || device->interfaces[i].ep_out == ep_addr)
        {
            return i;
        }
    }
    return 0xFF;
}

static void wait_for_tx_complete(uint8_t dev_addr, uint8_t ep_addr)
{
    while (usbh_edpt_busy(dev_addr, ep_addr))
    {
        tuh_task();
    }
}

bool send_ctrl_xfer(uint8_t dev_addr, const tusb_control_request_t* request, uint8_t* buffer, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
    tuh_xfer_s transfer = 
    {
        .daddr = dev_addr,
        .ep_addr = 0x00,
        .setup = request, 
        .buffer = buffer,
        .complete_cb = complete_cb, 
        .user_data = user_data
    };
    return tuh_control_xfer(&transfer);
}

static void xbox360w_chatpad_init(Interface *interface, uint8_t address, uint8_t instance)
{
    send_report(address, instance, Xbox360W::INQUIRE_PRESENT, sizeof(Xbox360W::INQUIRE_PRESENT));
    wait_for_tx_complete(address, interface->ep_out);
    send_report(address, instance, Xbox360W::CONTROLLER_INFO, sizeof(Xbox360W::CONTROLLER_INFO));
    wait_for_tx_complete(address, interface->ep_out);
    send_report(address, instance, Xbox360W::Chatpad::INIT, sizeof(Xbox360W::Chatpad::INIT));
    wait_for_tx_complete(address, interface->ep_out);

    uint8_t led_ctrl[4];
    std::memcpy(led_ctrl, Xbox360W::Chatpad::LED_CTRL, sizeof(Xbox360W::Chatpad::LED_CTRL));
    led_ctrl[2] = Xbox360W::Chatpad::LED_ON[0];

    send_report(address, instance, led_ctrl, sizeof(led_ctrl));
    wait_for_tx_complete(address, interface->ep_out);

    interface->chatpad_inited = true;
    interface->chatpad_stage = ChatpadStage::KEEPALIVE_1;
}

static bool xbox360_chatpad_keepalive_cb(repeating_timer_t* rt)
{    
    Interface* interface = static_cast<Interface*>(rt->user_data);
    uint8_t instance = get_instance_by_ep(interface->dev_addr, interface->ep_in);

    switch (interface->chatpad_stage)
    {
        case ChatpadStage::KEEPALIVE_1:
            switch (interface->dev_type)
            {
                case DevType::XBOX360:
                    send_ctrl_xfer(interface->dev_addr, &Xbox360::Chatpad::KEEPALIVE_1, nullptr, nullptr, 0);
                    break;
                case DevType::XBOX360W:
                    send_report(interface->dev_addr, instance, Xbox360W::Chatpad::KEEPALIVE_1, sizeof(Xbox360W::Chatpad::KEEPALIVE_1));
                    break;
                default:
                    break;
            }
            interface->chatpad_stage = ChatpadStage::KEEPALIVE_2;
            break;
        case ChatpadStage::KEEPALIVE_2:
            switch (interface->dev_type)
            {
                case DevType::XBOX360:
                    send_ctrl_xfer(interface->dev_addr, &Xbox360::Chatpad::KEEPALIVE_2, nullptr, nullptr, 0);
                    break;
                case DevType::XBOX360W:
                    send_report(interface->dev_addr, instance, Xbox360W::Chatpad::KEEPALIVE_2, sizeof(Xbox360W::Chatpad::KEEPALIVE_2));
                    break;
                default:
                    break;
            }
            interface->chatpad_stage = ChatpadStage::KEEPALIVE_1;
            break;
    }
    return true;
}

static void xboxone_init(Interface *interface, uint8_t dev_addr, uint8_t instance)
{
    uint16_t PID, VID;
    tuh_vid_pid_get(dev_addr, &VID, &PID);

    send_report(dev_addr, instance, XboxOne::POWER_ON, sizeof(XboxOne::POWER_ON));
    wait_for_tx_complete(dev_addr, interface->ep_out);
    send_report(dev_addr, instance, XboxOne::S_INIT, sizeof(XboxOne::S_INIT));
    wait_for_tx_complete(dev_addr, interface->ep_out);

    if (VID == 0x045e && (PID == 0x0b00))
    {
        send_report(dev_addr, instance, XboxOne::EXTRA_INPUT_PACKET_INIT, sizeof(XboxOne::EXTRA_INPUT_PACKET_INIT));
        wait_for_tx_complete(dev_addr, interface->ep_out);
    }

    //Required for PDP aftermarket controllers
    if (VID == 0x0e6f)
    {
        send_report(dev_addr, instance, XboxOne::PDP_LED_ON, sizeof(XboxOne::PDP_LED_ON));
        wait_for_tx_complete(dev_addr, interface->ep_out);
        send_report(dev_addr, instance, XboxOne::PDP_AUTH, sizeof(XboxOne::PDP_AUTH));
        wait_for_tx_complete(dev_addr, interface->ep_out);
    }
}

//Class driver

static bool init()
{
    devices_.fill(Device());
    for (auto& device : devices_)
    {
        device.interfaces.fill(Interface());
    }
    return true;
} 

static bool open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(desc_itf->bNumEndpoints > 0);

    DevType dev_type = DevType::UNKNOWN;
    ItfType itf_type = ItfType::UNKNOWN;

    if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x81)
    {
        itf_type = ItfType::XID;
        dev_type = DevType::XBOX360W;
    }
    else if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x01)
    {
        itf_type = ItfType::XID;
        dev_type = DevType::XBOX360;
    }
    else if (desc_itf->bInterfaceSubClass == 0x47 && desc_itf->bInterfaceProtocol == 0xD0)
    {
        itf_type = ItfType::XID;
        dev_type = DevType::XBOXONE;
    }
    else if (desc_itf->bInterfaceClass == 0x58 && desc_itf->bInterfaceSubClass == 0x42) 
    {
        itf_type = ItfType::XID;
        dev_type = DevType::XBOXOG;
    }

    TU_VERIFY(dev_type != DevType::UNKNOWN);

    Interface* interface = get_free_interface(dev_addr);
    TU_VERIFY(interface != nullptr);

    const uint8_t *p_desc = reinterpret_cast<const uint8_t*>(desc_itf);
    int endpoint = 0;
    int pos = 0;

    while (endpoint < desc_itf->bNumEndpoints && pos < max_len)
    {
        if (tu_desc_type(p_desc) != TUSB_DESC_ENDPOINT)
        {
            pos += tu_desc_len(p_desc);
            p_desc = tu_desc_next(p_desc);
            continue;
        }

        const tusb_desc_endpoint_t *desc_ep = reinterpret_cast<const tusb_desc_endpoint_t*>(p_desc);
        TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
        TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));

        interface->itf_num = desc_itf->bInterfaceNumber;
        interface->itf_type = itf_type;
        interface->dev_type = dev_type;
        interface->dev_addr = dev_addr;

        if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT)
        {
            interface->ep_out = desc_ep->bEndpointAddress;
            interface->ep_out_size = tu_edpt_packet_size(desc_ep);
        }
        else
        {
            interface->ep_in = desc_ep->bEndpointAddress;
            interface->ep_in_size = tu_edpt_packet_size(desc_ep);
        }

        endpoint++;
        pos += tu_desc_len(p_desc);
        p_desc = tu_desc_next(p_desc);
    }

    return true;
}

static bool set_config(uint8_t dev_addr, uint8_t itf_num)
{
    uint8_t instance = get_instance_by_itf_num(dev_addr, itf_num);
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    TU_VERIFY(interface != nullptr);

    interface->connected = true;

    switch (interface->dev_type)
    {
        case DevType::XBOX360W:
            interface->connected = false;
            send_report(dev_addr, instance, Xbox360W::INQUIRE_PRESENT, sizeof(Xbox360W::INQUIRE_PRESENT));
            wait_for_tx_complete(dev_addr, interface->ep_out);
            break;
        case DevType::XBOXONE:
            xboxone_init(interface, dev_addr, instance);
            break;
        default:
            break;
    }

    if (mount_cb)
    {
        mount_cb(dev_addr, instance, interface);
    }

    usbh_driver_set_config_complete(dev_addr, interface->itf_num);
    return true;
}

static bool xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    const uint8_t dir = tu_edpt_dir(ep_addr);
    const uint8_t instance = get_instance_by_ep(dev_addr, ep_addr);
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    TU_VERIFY(interface != nullptr);

    if (result != XFER_RESULT_SUCCESS)
    {
        if (dir == TUSB_DIR_IN)
        {
            report_received_cb(dev_addr, instance, interface->ep_in_buffer.data(), static_cast<uint16_t>(interface->ep_in_size));
        }
        else if (report_sent_cb)
        {
            report_sent_cb(dev_addr, instance, interface->ep_out_buffer.data(), static_cast<uint16_t>(interface->ep_out_size));
        }
    }

    if (dir == TUSB_DIR_IN)
    {
        bool new_pad_data = false;
        uint8_t* in_buffer = interface->ep_in_buffer.data();

        switch (interface->dev_type)
        {
            case DevType::XBOX360:
                if (in_buffer[1] == 0x14)
                {
                    new_pad_data = true;
                }
                break;
            case DevType::XBOX360W:
                if (in_buffer[0] & 0x08)
                {
                    if (in_buffer[1] != 0x00 && !interface->connected)
                    {
                        interface->connected = true;
                        xbox360w_chatpad_init(interface, dev_addr, instance);
                        send_report(dev_addr, instance, Xbox360W::RUMBLE_ENABLE, sizeof(Xbox360W::RUMBLE_ENABLE));
                        add_repeating_timer_ms(KEEPALIVE_MS, xbox360_chatpad_keepalive_cb, interface, &interface->keepalive_timer);
                        if (xbox360w_connect_cb)
                        {
                            xbox360w_connect_cb(dev_addr, instance);
                        }
                    }
                    else if (in_buffer[1] == 0x00 && interface->connected)
                    {
                        interface->connected = false;
                        interface->chatpad_inited = false;
                        cancel_repeating_timer(&interface->keepalive_timer);
                        if (xbox360w_disconnect_cb)
                        {
                            xbox360w_disconnect_cb(dev_addr, instance);
                        }
                    }
                }
                if ((in_buffer[1] & 1) && in_buffer[5] == 0x13)
                {
                    new_pad_data = true;
                }
                break;
            case DevType::XBOXONE:
                switch (in_buffer[0])
                {
                    case XboxOne::GIP_CMD_INPUT:
                        new_pad_data = true;
                        break;
                    case XboxOne::GIP_CMD_VIRTUAL_KEY:
                        if (in_buffer[4] == 0x01 && !(in_buffer[4] & (1 << 1)))
                        {
                            in_buffer[4] |= (1 << 1);
                            new_pad_data = true;
                        }
                        else if (in_buffer[4] == 0x00 && (in_buffer[4] & (1 << 1))) 
                        {
                            in_buffer[4] &= ~(1 << 1);
                            new_pad_data = true;
                        }
                        break;
                    case XboxOne::GIP_CMD_ANNOUNCE:
                        xboxone_init(interface, dev_addr, instance);
                        break;
                }
                break;
            case DevType::XBOXOG:
                if (in_buffer[1] == 0x14)
                {
                    new_pad_data = true;
                }
            default:
                break;
        }

        if (new_pad_data)
        {
            report_received_cb(dev_addr, instance, in_buffer, static_cast<uint16_t>(xferred_bytes));
        }
        else
        {
            receive_report(dev_addr, instance);
        }
    }
    else
    {
        if (report_sent_cb)
        {
            report_sent_cb(dev_addr, instance, interface->ep_out_buffer.data(), static_cast<uint16_t>(xferred_bytes));
        }
    }
    return true;
}

void close(uint8_t dev_addr)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, );

    for (uint8_t i = 0; i < device->interfaces.size(); ++i)
    {
        if (device->interfaces[i].itf_num != 0xFF && unmount_cb)
        {
            unmount_cb(dev_addr, i, &device->interfaces[i]);
        }
        cancel_repeating_timer(&device->interfaces[i].keepalive_timer);
    }

    *device = Device();
}

const usbh_class_driver_t class_driver_ =
{
    .init       = init,
    .open       = open,
    .set_config = set_config,
    .xfer_cb    = xfer_cb,
    .close      = close
};

//Public API

const usbh_class_driver_t* class_driver()
{
    return &class_driver_;
}

bool send_report(uint8_t dev_addr, uint8_t instance, const uint8_t *buffer, uint16_t len)
{
    Interface *interface = get_itf_by_instance(dev_addr, instance);
    TU_VERIFY(interface != nullptr);
    TU_ASSERT(len <= interface->ep_out_size);
    TU_VERIFY(usbh_edpt_claim(dev_addr, interface->ep_out));

    std::memcpy(interface->ep_out_buffer.data(), buffer, len);

    if (!usbh_edpt_xfer(dev_addr, interface->ep_out, interface->ep_out_buffer.data(), len))
    {
        usbh_edpt_release(dev_addr, interface->ep_out);
        return false;
    }
    return true;
}

bool receive_report(uint8_t dev_addr, uint8_t instance)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    TU_VERIFY(interface != nullptr);

    if (!usbh_edpt_xfer(dev_addr, interface->ep_in, interface->ep_in_buffer.data(), interface->ep_in_size))
    {
        usbh_edpt_release(dev_addr, interface->ep_in);
        return false;
    }
    return true;
}

bool set_led(uint8_t dev_addr, uint8_t instance, uint8_t quadrant, bool block)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    TU_VERIFY(interface != nullptr);

    uint8_t buffer[32];
    uint16_t len;

    switch (interface->dev_type)
    {
        case DevType::XBOX360W:
            std::memcpy(buffer, Xbox360W::LED, sizeof(Xbox360W::LED));
            buffer[3] = (quadrant == 0) ? 0x40 : (0x40 | (quadrant + 5));
            len = sizeof(Xbox360W::LED);
            break;
        case DevType::XBOX360:
            std::memcpy(buffer, Xbox360::LED, sizeof(Xbox360::LED));
            buffer[2] = (quadrant == 0) ? 0 : (quadrant + 5);
            len = sizeof(Xbox360::LED);
            break;
        default:
            return true;
    }

    bool ret = send_report(dev_addr, instance, buffer, len);
    if (block && ret)
    {
        wait_for_tx_complete(dev_addr, interface->ep_out);
    }
    return ret;
}

bool set_rumble(uint8_t dev_addr, uint8_t instance, uint8_t rumble_l, uint8_t rumble_r, bool block)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    TU_VERIFY(interface != nullptr);

    uint8_t buffer[32];
    uint16_t len;

    switch (interface->dev_type)
    {
        case DevType::XBOX360W:
            std::memcpy(buffer, Xbox360W::RUMBLE, sizeof(Xbox360W::RUMBLE));
            buffer[5] = rumble_l;
            buffer[6] = rumble_r;
            len = sizeof(Xbox360W::RUMBLE);
            break;
        case DevType::XBOX360:
            std::memcpy(buffer, Xbox360::RUMBLE, sizeof(Xbox360::RUMBLE));
            buffer[3] = rumble_l;
            buffer[4] = rumble_r;
            len = sizeof(Xbox360::RUMBLE);
            break;
        case DevType::XBOXONE:
            std::memcpy(buffer, XboxOne::RUMBLE, sizeof(XboxOne::RUMBLE));
            buffer[8] = rumble_l / 2; // 0 - 128
            buffer[9] = rumble_r / 2; // 0 - 128
            len = sizeof(XboxOne::RUMBLE);
            break;
        case DevType::XBOXOG:
            std::memcpy(buffer, XboxOG::RUMBLE, sizeof(XboxOG::RUMBLE));
            buffer[2] = rumble_l;
            buffer[3] = rumble_l;
            buffer[4] = rumble_r;
            buffer[5] = rumble_r;
            len = sizeof(XboxOG::RUMBLE);
            break;
        default:
            return true;
    }

    bool ret = send_report(dev_addr, instance, buffer, len);
    if (block && ret)
    {
        wait_for_tx_complete(dev_addr, interface->ep_out);
    }
    return true;
}

} // namespace tuh_xinput

// #ifndef CFG_TUH_XINPUT_WIRED_CHATPAD_EN
// #define CFG_TUH_XINPUT_WIRED_CHATPAD_EN 0
// #endif

// #include <cstring>

// #include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
// #include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput_cmd.h"

// namespace tuh_xinput {

// static constexpr uint8_t INTERFACE_MULT = (CFG_TUH_XINPUT_WIRED_CHATPAD_EN > 0) ? 2 : 1;
// static constexpr uint8_t INTERFACE_MAX = CFG_TUH_XINPUT * INTERFACE_MULT;

// enum class ChatpadStage
// {
//     INIT_1 = 0,
//     INIT_2,
//     INIT_3,
//     INIT_4,
//     INIT_5,
//     INIT_6,
//     INIT_1E,
//     INIT_1F,
//     LED_REQUEST
// };

// struct Device
// {
//     uint8_t inst_count{0};
//     std::array<Interface, INTERFACE_MAX> interfaces_;
// };

// static std::array<Device, CFG_TUH_DEVICE_MAX> devices_;

// TU_ATTR_ALWAYS_INLINE static inline Device* get_device(uint8_t dev_addr)
// {
//     return &devices_[dev_addr - 1];
// }

// TU_ATTR_ALWAYS_INLINE static inline Interface* get_interface_by_inst(uint8_t dev_addr, uint8_t instance)
// {
//     return &get_device(dev_addr)->interfaces_[instance];
// }

// TU_ATTR_ALWAYS_INLINE static inline uint8_t get_instance_by_epaddr(uint8_t dev_addr, uint8_t ep_addr)
// {
//     for (uint8_t inst = 0; inst < INTERFACE_MAX; inst++)
//     {
//         Interface *interface = get_interface_by_inst(dev_addr, inst);

//         if ((ep_addr == interface->ep_in) || (ep_addr == interface->ep_out))
//         {
//             return inst;
//         }
//     }
//     return 0xff;
// }

// TU_ATTR_ALWAYS_INLINE static inline uint8_t get_instance_by_itf(uint8_t dev_addr, uint8_t itf_num)
// {
//     for (uint8_t inst = 0; inst < INTERFACE_MAX; inst++)
//     {
//         Interface *interface = get_interface_by_inst(dev_addr, inst);

//         if (itf_num == interface->itf_num)
//         {
//             return inst;
//         }
//     }
//     return 0xff;
// }

// static void wait_for_tx_complete(uint8_t dev_addr, uint8_t ep_addr)
// {
//     while (usbh_edpt_busy(dev_addr, ep_addr))
//     {
//         tuh_task();
//     }
// }

// void ctrl_xfer_cb(tuh_xfer_t* xfer) { return; }

// bool send_ctrl_xfer(uint8_t dev_addr, const tusb_control_request_t* setup, uint8_t* buffer, bool block = true)
// {
//     tuh_xfer_t xfer = 
//     {
//         .daddr = dev_addr,
//         .ep_addr = 0,
//         .setup = setup,
//         .buffer = buffer,
//         .complete_cb = block ? nullptr : ctrl_xfer_cb,
//         .user_data = 0
//     };
//     if (tuh_control_xfer(&xfer))
//     {
//         return true;
//     }
//     return false;
// }

// bool run_tuh_task(uint32_t duration_ms)
// {
//     if (!time_in_milliseconds_cb)
//     {
//         return false;
//     }
//     uint32_t end_time = time_in_milliseconds_cb() + duration_ms;
//     while (end_time > time_in_milliseconds_cb())
//     {
//         tuh_task();
//     }
//     return true;
// }

// static void xboxone_init(Interface *xid_itf, uint8_t dev_addr, uint8_t instance)
// {
//     uint16_t pid, vid;
//     tuh_vid_pid_get(dev_addr, &vid, &pid);

//     send_report(dev_addr, instance, XboxOne::POWER_ON, sizeof(XboxOne::POWER_ON));
//     wait_for_tx_complete(dev_addr, xid_itf->ep_out);
//     send_report(dev_addr, instance, XboxOne::S_INIT, sizeof(XboxOne::S_INIT));
//     wait_for_tx_complete(dev_addr, xid_itf->ep_out);

//     if (vid == 0x045e && (pid == 0x0b00))
//     {
//         send_report(dev_addr, instance, XboxOne::EXTRA_INPUT_PACKET_INIT, sizeof(XboxOne::EXTRA_INPUT_PACKET_INIT));
//         wait_for_tx_complete(dev_addr, xid_itf->ep_out);
//     }

//     //Required for PDP aftermarket controllers
//     if (vid == 0x0e6f)
//     {
//         send_report(dev_addr, instance, XboxOne::PDP_LED_ON, sizeof(XboxOne::PDP_LED_ON));
//         wait_for_tx_complete(dev_addr, xid_itf->ep_out);
//         send_report(dev_addr, instance, XboxOne::PDP_AUTH, sizeof(XboxOne::PDP_AUTH));
//         wait_for_tx_complete(dev_addr, xid_itf->ep_out);
//     }
// }

// static void xbox360w_chatpad_init(uint8_t address, uint8_t instance)
// {
//     Interface *xid_itf = get_interface_by_inst(address, instance);

//     send_report(address, instance, Xbox360W::INQUIRE_PRESENT, sizeof(Xbox360W::INQUIRE_PRESENT));
//     wait_for_tx_complete(address, xid_itf->ep_out);
//     send_report(address, instance, Xbox360W::CONTROLLER_INFO, sizeof(Xbox360W::CONTROLLER_INFO));
//     wait_for_tx_complete(address, xid_itf->ep_out);
//     send_report(address, instance, Xbox360W::Chatpad::INIT, sizeof(Xbox360W::Chatpad::INIT));
//     wait_for_tx_complete(address, xid_itf->ep_out);

//     uint8_t led_ctrl[4];
//     std::memcpy(led_ctrl, Xbox360W::Chatpad::LED_CTRL, sizeof(Xbox360W::Chatpad::LED_CTRL));
//     led_ctrl[2] = Xbox360W::Chatpad::LED_ON[0];

//     send_report(address, instance, led_ctrl, sizeof(led_ctrl));
//     wait_for_tx_complete(address, xid_itf->ep_out);

//     xid_itf->chatpad_initialized = true;
// }

// #if (CFG_TUH_XINPUT_WIRED_CHATPAD_EN > 0)
// static void xbox360_chatpad_init(uint8_t address, uint8_t instance)
// {
//     if (!time_in_milliseconds_cb)
//     {
//         return;
//     }

//     Interface *xid_itf = get_interface_by_inst(address, instance);
//     ChatpadStage init_stage = ChatpadStage::INIT_1;

//     tusb_desc_device_t desc_device;
//     tuh_descriptor_get_device(address, &desc_device, 18, nullptr, 0);

//     uint8_t buffer[2] = {0x01, 0x02};

//     // if (desc_device.bcdDevice == 0x0114)
//     // {
//         buffer[0] = 0x09;
//         buffer[1] = 0x00;
//     // }

//     while (!xid_itf->chatpad_initialized)
//     {
//         switch (init_stage)
//         {
//             case ChatpadStage::INIT_1:
//                 send_ctrl_xfer(address, &Xbox360::Chatpad::INIT_1, nullptr);
//                 init_stage = ChatpadStage::INIT_2;
//                 break;
//             case ChatpadStage::INIT_2:
//                 send_ctrl_xfer(address, &Xbox360::Chatpad::INIT_2, nullptr);
//                 init_stage = ChatpadStage::INIT_3;
//                 break;
//             case ChatpadStage::INIT_3:
//                 send_ctrl_xfer(address, &Xbox360::Chatpad::INIT_3, nullptr);
//                 init_stage = ChatpadStage::INIT_4;
//                 break;
//             case ChatpadStage::INIT_4:
//                 if (send_ctrl_xfer(address, &Xbox360::Chatpad::INIT_4, buffer))
//                 {
//                     init_stage = ChatpadStage::INIT_5;
//                 }
//                 break;
//             case ChatpadStage::INIT_5:
//                 if (send_ctrl_xfer(address, &Xbox360::Chatpad::INIT_5, buffer))
//                 {
//                     init_stage = ChatpadStage::INIT_6;
//                 }
//                 break;
//             case ChatpadStage::INIT_6:
//                 if (send_ctrl_xfer(address, &Xbox360::Chatpad::INIT_6, buffer))
//                 {
//                     init_stage = ChatpadStage::INIT_1E;
//                 }
//                 break;
//             case ChatpadStage::INIT_1E:
//                 run_tuh_task(1000);
//                 if (send_ctrl_xfer(address, &Xbox360::Chatpad::KEEPALIVE_2, nullptr))
//                 {
//                     init_stage = ChatpadStage::INIT_1F;
//                 }
//                 break;
//             case ChatpadStage::INIT_1F:
//                 run_tuh_task(1000);
//                 if (send_ctrl_xfer(address, &Xbox360::Chatpad::KEEPALIVE_1, nullptr))
//                 {
//                     init_stage = ChatpadStage::LED_REQUEST;
//                 }
//                 break;
//             case ChatpadStage::LED_REQUEST:
//                 if (send_ctrl_xfer(address, &Xbox360::Chatpad::LEDS_1B, nullptr))
//                 {
//                     xid_itf->chatpad_initialized = true;
//                 }
//                 break;
//         }
//     }
// }
// #endif // CFG_TUH_XINPUT_WIRED_CHATPAD_EN

// //Class driver

// bool xinputh_init()
// {
//     for (auto& device : devices_)
//     {
//         device.inst_count = 0;
//         device.interfaces_.fill(Interface());
//     }
//     return true;
// }

// bool xinputh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
// {
//     TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX);

//     Type type = Type::UNKNOWN;

//     if (desc_itf->bNumEndpoints < 1)
//     {
//         type = Type::UNKNOWN;
//     }
//     else if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x81) 
//     {
//         type = Type::XBOX360_WIRELESS;
//     }
//     else if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x01) 
//     {
//         type = Type::XBOX360_WIRED;
//     }
// #if (CFG_TUH_XINPUT_WIRED_CHATPAD_EN > 0)
//     else if (desc_itf->bInterfaceNumber == 2 && desc_itf->bInterfaceSubClass == 0x5d && desc_itf->bInterfaceProtocol == 0x02)
//     {
//         uint16_t vid, pid;
//         tuh_vid_pid_get(dev_addr, &vid, &pid);
//         if (vid == 0x045e && pid == 0x028e)
//         {
//             type = Type::XBOX360_WIRED_CHATPAD;
//         }
//     }
// #endif // CFG_TUH_XINPUT_WIRED_CHATPAD_EN
//     else if (desc_itf->bInterfaceSubClass == 0x47 && desc_itf->bInterfaceProtocol == 0xD0) 
//     {
//         type = Type::XBOXONE;
//     }
//     else if (desc_itf->bInterfaceClass == 0x58 && desc_itf->bInterfaceSubClass == 0x42) 
//     {
//         type = Type::XBOXOG;
//     }

//     if (type == Type::UNKNOWN)
//     {
//         TU_LOG2("XINPUT: Not a valid interface\n");
//         return false;
//     }

//     TU_LOG2("XINPUT opening Interface %u (addr = %u)\r\n", desc_itf->bInterfaceNumber, dev_addr);

//     Device *xinput_dev = get_device(dev_addr);
//     bool dual_interface = (type == Type::XBOX360_WIRED_CHATPAD || type == Type::XBOX360_WIRED);
//     TU_ASSERT(xinput_dev->inst_count < (dual_interface ? INTERFACE_MAX : CFG_TUH_XINPUT), 0);

//     Interface *xid_itf = get_interface_by_inst(dev_addr, xinput_dev->inst_count);
//     xid_itf->itf_num = desc_itf->bInterfaceNumber;
//     xid_itf->type = type;

//     //Parse descriptor for all endpoints and open them
//     uint8_t const *p_desc = (uint8_t const *)desc_itf;
//     int endpoint = 0;
//     int pos = 0;

//     while (endpoint < desc_itf->bNumEndpoints && pos < max_len)
//     {
//         if (tu_desc_type(p_desc) != TUSB_DESC_ENDPOINT)
//         {
//             pos += tu_desc_len(p_desc);
//             p_desc = tu_desc_next(p_desc);
//             continue;
//         }

//         tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
//         TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
//         TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));

//         if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT)
//         {
//             xid_itf->ep_out = desc_ep->bEndpointAddress;
//             xid_itf->ep_out_size = static_cast<uint8_t>(tu_edpt_packet_size(desc_ep));
//         }
//         else
//         {
//             xid_itf->ep_in = desc_ep->bEndpointAddress;
//             xid_itf->ep_in_size = static_cast<uint8_t>(tu_edpt_packet_size(desc_ep));
//         }

//         endpoint++;
//         pos += tu_desc_len(p_desc);
//         p_desc = tu_desc_next(p_desc);
//     }

//     xinput_dev->inst_count++;
//     return true;
// }

// bool xinputh_set_config(uint8_t dev_addr, uint8_t itf_num)
// {
//     uint8_t instance = get_instance_by_itf(dev_addr, itf_num);
//     Interface *xid_itf = get_interface_by_inst(dev_addr, instance);
//     xid_itf->connected = true;

//     if (xid_itf->type == Type::XBOX360_WIRELESS)
//     {
//         xid_itf->connected = false;
//         send_report(dev_addr, instance, Xbox360W::INQUIRE_PRESENT, sizeof(Xbox360W::INQUIRE_PRESENT));
//         wait_for_tx_complete(dev_addr, xid_itf->ep_out);
//     }
//     else if (xid_itf->type == Type::XBOX360_WIRED)
//     {
//     }
// #if (CFG_TUH_XINPUT_WIRED_CHATPAD_EN > 0)
//     else if (xid_itf->type == Type::XBOX360_WIRED_CHATPAD)
//     {
//         xbox360_chatpad_init(dev_addr, instance);
//     }
// #endif
//     else if (xid_itf->type == Type::XBOXONE)
//     {
//         xboxone_init(xid_itf, dev_addr, instance);
//     }

//     if (mount_cb)
//     {
//         mount_cb(dev_addr, instance, xid_itf);
//     }

//     usbh_driver_set_config_complete(dev_addr, xid_itf->itf_num);
//     return true;
// }

// bool xinputh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
// {
//     uint8_t const dir = tu_edpt_dir(ep_addr);
//     uint8_t const instance = get_instance_by_epaddr(dev_addr, ep_addr);
//     Interface *xid_itf = get_interface_by_inst(dev_addr, instance);
//     uint8_t *rdata = xid_itf->ep_in_buffer.data();
//     bool new_pad_data = false;

//     xid_itf->last_xfer_result = result;
//     xid_itf->last_xfer_size = xferred_bytes;

//     // On transfer error, bail early but notify the application
//     if (result != XFER_RESULT_SUCCESS)
//     {
//         // if (dir == TUSB_DIR_IN)
//         // {
//         //     report_received_cb(dev_addr, instance, xid_itf->ep_in_buffer.data(), xid_itf->ep_in_buffer.size());
//         // }
//         // else if (report_sent_cb)
//         // {
//         //     report_sent_cb(dev_addr, instance, xid_itf->ep_out_buffer.data(), xferred_bytes);
//         // }
//         return false;
//     }

//     if (xid_itf->type == Type::XBOX360_WIRED_CHATPAD)
//     {
//         TU_LOG1("XINPUT: Chatpad data received (%u bytes)\n", xferred_bytes);
//     }

//     if (dir == TUSB_DIR_IN)
//     {
//         TU_LOG2("Get Report callback (%u, %u, %u bytes)\r\n", dev_addr, instance, xferred_bytes);

//         switch (xid_itf->type)
//         {
//             case Type::XBOX360_WIRED:
//                 if (rdata[1] == 0x14)
//                 {
//                     std::memcpy(xid_itf->gp_report_buffer.data(), rdata, xferred_bytes);
//                     xid_itf->gp_report_size = xferred_bytes;
//                     new_pad_data = true;
//                 }    
//                 break;
//             case Type::XBOX360_WIRELESS:
//                 if (rdata[0] & 0x08)
//                 {
//                     if (rdata[1] != 0x00 && xid_itf->connected == false)
//                     {
//                         TU_LOG2("XINPUT: WIRELESS CONTROLLER CONNECTED\n");
//                         xid_itf->connected = true;
//                         xbox360w_chatpad_init(dev_addr, instance);
//                     }
//                     else if (rdata[1] == 0x00 && xid_itf->connected == true)
//                     {
//                         TU_LOG2("XINPUT: WIRELESS CONTROLLER DISCONNECTED\n");
//                         xid_itf->connected = false;
//                     }
//                 }
//                 if ((rdata[1] & 1) && rdata[5] == 0x13)
//                 {
//                     std::memcpy(xid_itf->gp_report_buffer.data(), rdata, xferred_bytes);
//                     xid_itf->gp_report_size = xferred_bytes;
//                     new_pad_data = true;
//                 }
//                 break;
//             case Type::XBOXONE:
//                 if (rdata[0] == XboxOne::GIP_CMD_INPUT)
//                 {
//                     std::memcpy(xid_itf->gp_report_buffer.data(), rdata, xferred_bytes);
//                     xid_itf->gp_report_size = xferred_bytes;
//                     new_pad_data = true;
//                 }
//                 else if (rdata[0] == XboxOne::GIP_CMD_VIRTUAL_KEY)
//                 {
//                     if (rdata[4] == 0x01 && !(xid_itf->gp_report_buffer[4] & (1 << 1))) 
//                     {
//                         xid_itf->gp_report_buffer[4] |= (1 << 1);
//                         new_pad_data = true;
//                     }
//                     else if (rdata[4] == 0x00 && (xid_itf->gp_report_buffer[4] & (1 << 1))) 
//                     {
//                         xid_itf->gp_report_buffer[4] &= ~(1 << 1);
//                         new_pad_data = true;
//                     }
//                 }
//                 else if (rdata[0] == XboxOne::GIP_CMD_ANNOUNCE)
//                 {
//                     xboxone_init(xid_itf, dev_addr, instance);
//                 }
//                 break;
//             case Type::XBOXOG:
//                 if (rdata[1] == 0x14)
//                 {
//                     std::memcpy(xid_itf->gp_report_buffer.data(), rdata, xferred_bytes);
//                     xid_itf->gp_report_size = xferred_bytes;
//                     new_pad_data = true;
//                 }
//                 break;
// #if (CFG_TUH_XINPUT_WIRED_CHATPAD_EN > 0)
//             case Type::XBOX360_WIRED_CHATPAD:
//                 std::memcpy(xid_itf->gp_report_buffer.data(), rdata, xferred_bytes);
//                 xid_itf->gp_report_size = xferred_bytes;
//                 new_pad_data = true;
//                 break;
// #endif // CFG_TUH_XINPUT_WIRED_CHATPAD_EN
//             default:
//                 break;
//         }

//         if (new_pad_data)
//         {
//             report_received_cb(dev_addr, instance, xid_itf->gp_report_buffer.data(), static_cast<uint16_t>(xid_itf->gp_report_size));
//         } 
//         else 
//         {
//             receive_report(dev_addr, instance);
//         }
//     }
//     else
//     {
//         if (report_sent_cb)
//         {
//             report_sent_cb(dev_addr, instance, xid_itf->ep_out_buffer.data(), static_cast<uint16_t>(xferred_bytes));
//         }
//     }

//     return true;
// }

// void xinputh_close(uint8_t dev_addr)
// {
//     TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX, );
//     Device *xinput_dev = get_device(dev_addr);

//     for (uint8_t i = 0; i < xinput_dev->interfaces_.size(); i++)
//     {
//         if (umount_cb)
//         {
//             umount_cb(dev_addr, i, &xinput_dev->interfaces_[i]);
//         }
//     }

//     *xinput_dev = Device();
//     xinput_dev->interfaces_.fill(Interface());
// }

// #ifndef DRIVER_NAME
// #if CFG_TUSB_DEBUG >= CFG_TUH_LOG_LEVEL
//   #define DRIVER_NAME(_name)    .name = _name,
// #else
//   #define DRIVER_NAME(_name)
// #endif
// #endif

// const usbh_class_driver_t usbh_xinput_class_driver =
// {
//     DRIVER_NAME("XINPUT")
//     .init       = xinputh_init,
//     .open       = xinputh_open,
//     .set_config = xinputh_set_config,
//     .xfer_cb    = xinputh_xfer_cb,
//     .close      = xinputh_close
// };

// //Public API

// const usbh_class_driver_t* class_driver()
// {
//     return &usbh_xinput_class_driver;
// }

// const Interface& get_interface(uint8_t address, uint8_t instance)
// {
//     return *get_interface_by_inst(address, instance);
// }

// bool receive_report(uint8_t dev_addr, uint8_t instance)
// {
//     Interface *xid_itf = get_interface_by_inst(dev_addr, instance);
//     TU_VERIFY(usbh_edpt_claim(dev_addr, xid_itf->ep_in));

//     if (!usbh_edpt_xfer(dev_addr, xid_itf->ep_in, xid_itf->ep_in_buffer.data(), xid_itf->ep_in_size))
//     {
//         usbh_edpt_release(dev_addr, xid_itf->ep_in);
//         return false;
//     }
//     return true;
// }

// bool send_report(uint8_t dev_addr, uint8_t instance, const uint8_t *report, uint16_t len)
// {
//     Interface *xid_itf = get_interface_by_inst(dev_addr, instance);

//     TU_ASSERT(len <= xid_itf->ep_out_size);
//     TU_VERIFY(usbh_edpt_claim(dev_addr, xid_itf->ep_out));

//     std::memcpy(xid_itf->ep_out_buffer.data(), report, len);

//     if (!usbh_edpt_xfer(dev_addr, xid_itf->ep_out, xid_itf->ep_out_buffer.data(), len))
//     {
//         usbh_edpt_release(dev_addr, xid_itf->ep_out);
//         return false;
//     }
//     return true;
// }

// bool set_led(uint8_t dev_addr, uint8_t instance, uint8_t led_number, bool block)
// {
//     Interface *xid_itf = get_interface_by_inst(dev_addr, instance);
//     uint8_t buffer[32];
//     uint16_t len;

//     switch (xid_itf->type)
//     {
//         case Type::XBOX360_WIRELESS:
//             std::memcpy(buffer, Xbox360W::LED, sizeof(Xbox360W::LED));
//             buffer[3] = (led_number == 0) ? 0x40 : (0x40 | (led_number + 5));
//             len = sizeof(Xbox360W::LED);
//             break;
//         case Type::XBOX360_WIRED:
//             std::memcpy(buffer, Xbox360::LED, sizeof(Xbox360::LED));
//             buffer[2] = (led_number == 0) ? 0 : (led_number + 5);
//             len = sizeof(Xbox360::LED);
//             break;
//         default:
//             return true;
//     }
//     bool ret = send_report(dev_addr, instance, buffer, len);
//     if (block && ret)
//     {
//         wait_for_tx_complete(dev_addr, xid_itf->ep_out);
//     }
//     return ret;
// }

// bool set_rumble(uint8_t dev_addr, uint8_t instance, uint8_t rumble_l, uint8_t rumble_r, bool block)
// {
//     Interface *xid_itf = get_interface_by_inst(dev_addr, instance);
//     uint8_t buffer[32];
//     uint16_t len;

//     switch (xid_itf->type)
//     {
//         case Type::XBOX360_WIRELESS:
//             std::memcpy(buffer, Xbox360W::RUMBLE, sizeof(Xbox360W::RUMBLE));
//             buffer[5] = rumble_l;
//             buffer[6] = rumble_r;
//             len = sizeof(Xbox360W::RUMBLE);
//             break;
//         case Type::XBOX360_WIRED:
//             std::memcpy(buffer, Xbox360::RUMBLE, sizeof(Xbox360::RUMBLE));
//             buffer[3] = rumble_l;
//             buffer[4] = rumble_r;
//             len = sizeof(Xbox360::RUMBLE);
//             break;
//         case Type::XBOXONE:
//             std::memcpy(buffer, XboxOne::RUMBLE, sizeof(XboxOne::RUMBLE));
//             buffer[8] = rumble_l / 2; // 0 - 128
//             buffer[9] = rumble_r / 2; // 0 - 128
//             len = sizeof(XboxOne::RUMBLE);
//             break;
//         case Type::XBOXOG:
//             std::memcpy(buffer, XboxOG::RUMBLE, sizeof(XboxOG::RUMBLE));
//             buffer[2] = rumble_l;
//             buffer[3] = rumble_l;
//             buffer[4] = rumble_r;
//             buffer[5] = rumble_r;
//             len = sizeof(XboxOG::RUMBLE);
//             break;
//         default:
//             return true;
//     }
//     bool ret = send_report(dev_addr, instance, buffer, len);
//     if (block && ret)
//     {
//         wait_for_tx_complete(dev_addr, xid_itf->ep_out);
//     }
//     return true;
// }

// //Call every TUHXInput::CHATPAD_KEEPALIVE_MS if using chatpad
// void chatpad_keepalive(uint8_t address, uint8_t instance)
// {
//     Interface *xid_itf = get_interface_by_inst(address, instance);
//     if ((xid_itf->type != Type::XBOX360_WIRED_CHATPAD && xid_itf->type != Type::XBOX360_WIRELESS) ||
//         !xid_itf->chatpad_initialized)
//     {
//         return;
//     }
//     switch (xid_itf->chatpad_keepalive)
//     {
//         case KeepaliveStage::KEEPALIVE_1:
//             switch (xid_itf->type)
//             {
//                 case Type::XBOX360_WIRED_CHATPAD:
//                     send_ctrl_xfer(address, &Xbox360::Chatpad::KEEPALIVE_1, nullptr, false);
//                     break;
//                 case Type::XBOX360_WIRELESS:
//                     send_report(address, instance, Xbox360W::Chatpad::KEEPALIVE_1, sizeof(Xbox360W::Chatpad::KEEPALIVE_1));
//                     break;
//                 default:
//                     break;
//             }
//             xid_itf->chatpad_keepalive = KeepaliveStage::KEEPALIVE_2;
//             break;
//         case KeepaliveStage::KEEPALIVE_2:
//             switch (xid_itf->type)
//             {
//                 case Type::XBOX360_WIRED_CHATPAD:
//                     send_ctrl_xfer(address, &Xbox360::Chatpad::KEEPALIVE_2, nullptr, false);
//                     break;
//                 case Type::XBOX360_WIRELESS:
//                     send_report(address, instance, Xbox360W::Chatpad::KEEPALIVE_2, sizeof(Xbox360W::Chatpad::KEEPALIVE_2));
//                     break;
//                 default:
//                     break;
//             }
//             xid_itf->chatpad_keepalive = KeepaliveStage::KEEPALIVE_1;
//             break;
//     }
// }

// }; // namespace TUHXInput

#endif // (TUSB_OPT_HOST_ENABLED && CFG_TUH_XINPUT)