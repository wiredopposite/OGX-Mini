#include "tusb_option.h"

#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_XINPUT)

#include <cstring>
#include <chrono>

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput_cmd.h"
#include "USBHost/HostDriver/XInput/XboxArcadeStick.h"

#include "Board/Config.h"
#include "Board/board_api.h"
#if defined(CONFIG_EN_USB_HOST)
#include "pio_usb.h"
#endif

namespace tuh_xinput {

static constexpr uint8_t MAX_INTERFACES = CFG_TUH_XINPUT * 2;
static constexpr uint8_t MAX_DEVICES = CFG_TUH_DEVICE_MAX;
static constexpr uint8_t INVALID_IDX = 0xFF;

struct Device
{
    std::array<Interface, MAX_INTERFACES> interfaces{Interface()};
};

std::array<Device, MAX_DEVICES> devices_;

TU_ATTR_ALWAYS_INLINE static inline Device* get_device_by_addr(uint8_t dev_addr)
{
    TU_VERIFY((dev_addr <= devices_.size() && dev_addr > 0), nullptr);
    return &devices_[dev_addr - 1];
}

TU_ATTR_ALWAYS_INLINE static inline Interface* get_itf_by_itf_num(uint8_t dev_addr, uint8_t itf_num)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, nullptr);

    for (auto& interface : device->interfaces)
    {
        if (interface.itf_num == itf_num)
        {
            return &interface;
        }
    }
    return nullptr;
}

TU_ATTR_ALWAYS_INLINE static inline Interface* get_itf_by_ep(uint8_t dev_addr, uint8_t ep_addr)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, nullptr);

    for (auto& interface : device->interfaces)
    {
        if (interface.ep_in == ep_addr || interface.ep_out == ep_addr)
        {
            return &interface;
        }
    }
    return nullptr;
}

TU_ATTR_ALWAYS_INLINE static inline Interface* get_free_itf(uint8_t dev_addr)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, nullptr);

    for (auto& interface : device->interfaces)
    {
        if (interface.itf_num == INVALID_IDX)
        {
            return &interface;
        }
    }
    return nullptr;
}

TU_ATTR_ALWAYS_INLINE static inline uint8_t get_instance_by_itf_num(uint8_t dev_addr, uint8_t itf_num)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, INVALID_IDX);

    for (uint8_t i = 0; i < device->interfaces.size(); ++i)
    {
        if (device->interfaces[i].itf_num == itf_num)
        {
            return i;
        }
    }
    return INVALID_IDX;
}

TU_ATTR_ALWAYS_INLINE static inline Interface* get_itf_by_instance(uint8_t dev_addr, uint8_t instance)
{
    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, nullptr);
    TU_VERIFY(instance < device->interfaces.size(), nullptr);
    return &device->interfaces[instance];
}

static void std_sleep_ms(uint32_t ms)
{
    auto start = std::chrono::high_resolution_clock::now();
    while ( std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start).count() < ms) 
    {
#if defined(CONFIG_EN_USB_HOST)
        pio_usb_host_frame();
#endif
        tuh_task();
    }
}

static void wait_for_tx_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t timeout_ms = 200)
{
    const auto start = std::chrono::high_resolution_clock::now();
    while (usbh_edpt_busy(dev_addr, ep_addr))
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
        if (static_cast<uint32_t>(elapsed) >= timeout_ms)
        {
            return;
        }
#if defined(CONFIG_EN_USB_HOST)
        pio_usb_host_frame();
#endif
        tuh_task();
    }
}

static void prime_port_for_pairing(uint8_t dev_addr, uint8_t instance);

#if defined(CONFIG_EN_USB_HOST)
static void service_usb_host_frames(uint8_t frames = 4)
{
    for (uint8_t i = 0; i < frames; ++i)
    {
        pio_usb_host_frame();
        tuh_task();
    }
}
#else
static void service_usb_host_frames(uint8_t frames = 4)
{
    (void)frames;
    tuh_task();
}
#endif

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

static bool send_gip_packet(Interface* interface, uint8_t dev_addr, uint8_t instance,
    const uint8_t* packet, uint16_t len, bool assign_seq = true)
{
    uint8_t buf[ENDPOINT_SIZE];
    if (len > sizeof(buf))
    {
        return false;
    }
    std::memcpy(buf, packet, len);
    if (assign_seq && len >= 3)
    {
        buf[2] = interface->gip_out_seq++;
    }
    return send_report(dev_addr, instance, buf, len);
}

static void xboxone_ack_virtual_key(Interface* interface, uint8_t dev_addr, uint8_t instance, uint8_t seq)
{
    uint8_t ack[sizeof(XboxOne::VIRTUAL_KEY_ACK)];
    std::memcpy(ack, XboxOne::VIRTUAL_KEY_ACK, sizeof(ack));
    ack[2] = seq;
    send_gip_packet(interface, dev_addr, instance, ack, sizeof(ack), false);
}

static void xboxone_send_power_on(Interface* interface, uint8_t dev_addr, uint8_t instance)
{
    if (interface->gip_power_sent)
    {
        return;
    }
    send_gip_packet(interface, dev_addr, instance, XboxOne::POWER_ON, sizeof(XboxOne::POWER_ON));
    interface->gip_power_sent = true;
}

static void xboxone_init(Interface *interface, uint8_t dev_addr, uint8_t instance)
{
    uint16_t PID, VID;
    tuh_vid_pid_get(dev_addr, &VID, &PID);

    const bool arcade = interface->gip_arcade_stick || XboxArcadeStick::is_xbox_one_gip(VID, PID);
    interface->gip_out_seq = 0;

    xboxone_send_power_on(interface, dev_addr, instance);
    if (arcade)
    {
        return;
    }
    wait_for_tx_complete(dev_addr, interface->ep_out);

    send_gip_packet(interface, dev_addr, instance, XboxOne::S_INIT, sizeof(XboxOne::S_INIT));
    wait_for_tx_complete(dev_addr, interface->ep_out);

    if (VID == 0x045e && (PID == 0x0b00))
    {
        send_gip_packet(interface, dev_addr, instance, XboxOne::EXTRA_INPUT_PACKET_INIT,
            sizeof(XboxOne::EXTRA_INPUT_PACKET_INIT));
        wait_for_tx_complete(dev_addr, interface->ep_out);
    }

    //Required for PDP aftermarket controllers
    if (VID == 0x0e6f)
    {
        send_gip_packet(interface, dev_addr, instance, XboxOne::PDP_LED_ON, sizeof(XboxOne::PDP_LED_ON));
        wait_for_tx_complete(dev_addr, interface->ep_out);
        send_gip_packet(interface, dev_addr, instance, XboxOne::PDP_AUTH, sizeof(XboxOne::PDP_AUTH));
        wait_for_tx_complete(dev_addr, interface->ep_out);
    }
}

//Class driver

static bool init()
{
    TU_LOG1("XInput Init\r\n");
    devices_.fill(Device());
    return true;
} 

static bool open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_LOG1("XInput Open\r\n");
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
        if (desc_itf->bInterfaceNumber == 0)
        {
            itf_type = ItfType::XID;
            dev_type = DevType::XBOXONE;
        }
    }
    else if (desc_itf->bInterfaceClass == 0x58 && desc_itf->bInterfaceSubClass == 0x42) 
    {
        itf_type = ItfType::XID;
        dev_type = DevType::XBOXOG;
    }
    else if (desc_itf->bInterfaceClass == TUSB_CLASS_VENDOR_SPECIFIC && desc_itf->bNumEndpoints >= 2)
    {
        uint16_t vid = 0;
        uint16_t pid = 0;
        tuh_vid_pid_get(dev_addr, &vid, &pid);
        if (desc_itf->bInterfaceNumber == 0 &&
            XboxArcadeStick::is_xbox_one_gip(vid, pid))
        {
            itf_type = ItfType::XID;
            dev_type = DevType::XBOXONE;
        }
    }

    TU_VERIFY(dev_type != DevType::UNKNOWN && itf_type != ItfType::UNKNOWN);

    Interface* interface = get_free_itf(dev_addr);
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
        if (dev_type == DevType::XBOXONE)
        {
            uint16_t vid = 0;
            uint16_t pid = 0;
            tuh_vid_pid_get(dev_addr, &vid, &pid);
            interface->gip_arcade_stick = XboxArcadeStick::is_xbox_one_gip(vid, pid);
        }

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
    TU_LOG1("XInput Set Config\r\n");
    
    Interface* interface = get_itf_by_itf_num(dev_addr, itf_num);
    uint8_t instance = get_instance_by_itf_num(dev_addr, itf_num);
    TU_VERIFY(instance != INVALID_IDX && interface != nullptr);

    interface->connected = true;

    switch (interface->dev_type)
    {
        case DevType::XBOX360W:
            interface->connected = false;
            send_report(dev_addr, instance, Xbox360W::INQUIRE_PRESENT, sizeof(Xbox360W::INQUIRE_PRESENT));
            wait_for_tx_complete(dev_addr, interface->ep_out);
            break;
        default:
            break;
    }

    if (mount_cb)
    {
        mount_cb(dev_addr, instance, interface);
    }

    if (interface->dev_type == DevType::XBOX360W)
    {
        receive_report(dev_addr, instance);
        prime_port_for_pairing(dev_addr, instance);
    }

    usbh_driver_set_config_complete(dev_addr, interface->itf_num);
    return true;
}

static bool xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    Interface* interface = get_itf_by_ep(dev_addr, ep_addr);
    TU_VERIFY(interface != nullptr);
    uint8_t instance = get_instance_by_itf_num(dev_addr, interface->itf_num);
    TU_VERIFY(instance != INVALID_IDX);
    
    const uint8_t dir = tu_edpt_dir(ep_addr);

    if (result != XFER_RESULT_SUCCESS)
    {
        if (dir == TUSB_DIR_IN)
        {
            receive_report(dev_addr, instance);
        }
        else if (report_sent_cb)
        {
            report_sent_cb(dev_addr, instance, interface->ep_out_buffer.data(), interface->ep_out_size);
        }
        return true;
    }

    if (dir == TUSB_DIR_IN)
    {
        if (host_activity_cb)
        {
            host_activity_cb(dev_addr, instance);
        }

        if (interface->dev_type == DevType::XBOXONE)
        {
            interface->gip_last_in_ok_ms = board_api::ms_since_boot();
        }

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
                    /* Byte1 bit7 = controller present (0x80). Do not treat headset-only 0x40 as paired. */
                    if ((in_buffer[1] & 0x80) && !interface->connected)
                    {
                        interface->connected = true;

                        TU_LOG1("Xbox 360 wireless controller connected\n");

                        send_report(dev_addr, instance, Xbox360W::RUMBLE_ENABLE, sizeof(Xbox360W::RUMBLE_ENABLE));
                        wait_for_tx_complete(dev_addr, interface->ep_out);
                        set_led(dev_addr, instance, wireless_led_quadrant(dev_addr, instance), true);

                        if (xbox360w_connect_cb)
                        {
                            xbox360w_connect_cb(dev_addr, instance);
                        }
                    }
                    else if (in_buffer[1] == 0x00 && interface->connected)
                    {
                        interface->connected = false;
                        interface->chatpad_inited = false;

                        TU_LOG1("Xbox 360 wireless controller disconnected\n");

                        if (xbox360w_disconnect_cb)
                        {
                            xbox360w_disconnect_cb(dev_addr, instance);
                        }
                    }
                }
                if (((in_buffer[1] & 1) && xferred_bytes >= 18 &&
                     (in_buffer[5] == 0x13 || in_buffer[5] == 0x14)) ||
                    (in_buffer[1] & 2))
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
                        if (in_buffer[1] == (XboxOne::GIP_OPT_ACK | XboxOne::GIP_OPT_INTERNAL))
                        {
                            xboxone_ack_virtual_key(interface, dev_addr, instance, in_buffer[2]);
                        }
                        if (xferred_bytes >= 5)
                        {
                            new_pad_data = true;
                        }
                        break;
                    case XboxOne::GIP_CMD_ANNOUNCE:
                        /* Arcade: POWER_ON once. Standard GIP (Series / PowerA): full
                         * xboxone_init (POWER_ON + S_INIT) if bring-up was missed (#27 / #87). */
                        if (!interface->gip_power_sent &&
                            !usbh_edpt_busy(dev_addr, interface->ep_out))
                        {
                            if (interface->gip_arcade_stick)
                                xboxone_send_power_on(interface, dev_addr, instance);
                            else
                                xboxone_init(interface, dev_addr, instance);
                        }
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

bool deinit()
{
    TU_LOG1("XInput deinit\r\n");
    return true;
}

void close(uint8_t dev_addr)
{
    TU_LOG1("XInput close\r\n");

    Device* device = get_device_by_addr(dev_addr);
    TU_VERIFY(device != nullptr, );

    for (uint8_t i = 0; i < device->interfaces.size(); ++i)
    {
        if (device->interfaces[i].itf_num != 0xFF && unmount_cb)
        {
            TU_LOG1("XInput unmounting\r\n");
            unmount_cb(dev_addr, i, &device->interfaces[i]);
            TU_LOG1("XInput unmount\r\n");
            device->interfaces[i].itf_num = 0xFF;
            device->interfaces[i].connected = false;
            device->interfaces[i].gip_power_sent = false;
            device->interfaces[i].gip_last_in_ok_ms = 0;
            device->interfaces[i].gip_last_in_arm_ms = 0;
        }
    }
}

//Public API

const usbh_class_driver_t* class_driver()
{
    static const usbh_class_driver_t class_driver =
    {
    #if CFG_TUSB_DEBUG >= 2
        .name       = "XInput",
    #else
        .name       = nullptr,
    #endif
        .init       = init,
        .deinit     = deinit,
        .open       = open,
        .set_config = set_config,
        .xfer_cb    = xfer_cb,
        .close      = close
    };
    return &class_driver;
}

bool send_report(uint8_t dev_addr, uint8_t instance, const uint8_t *buffer, uint16_t len)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    TU_VERIFY(interface != nullptr);
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

    uint16_t in_size = interface->ep_in_size;
    if (interface->dev_type == DevType::XBOXONE)
    {
        if (interface->gip_arcade_stick)
        {
            in_size = XboxArcadeStick::GIP_IN_XFER_SIZE;
        }
        else
        {
            in_size = ENDPOINT_SIZE;
        }
    }

    TU_VERIFY(usbh_edpt_claim(dev_addr, interface->ep_in));

    if (!usbh_edpt_xfer(dev_addr, interface->ep_in, interface->ep_in_buffer.data(), in_size))
    {
        usbh_edpt_release(dev_addr, interface->ep_in);
        return false;
    }

    return true;
}

bool is_connected(uint8_t dev_addr, uint8_t instance)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    return interface != nullptr && interface->connected;
}

uint8_t wireless_led_quadrant(uint8_t dev_addr, uint8_t instance)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    if (interface == nullptr || interface->dev_type != DevType::XBOX360W)
    {
        return static_cast<uint8_t>(instance + 1);
    }
    /* Receiver USB layout: controller ports on interface numbers 0,2,4,6. */
    return static_cast<uint8_t>((interface->itf_num / 2) + 1);
}

void service_wireless_ports(uint8_t dev_addr)
{
    Device* device = get_device_by_addr(dev_addr);
    if (device == nullptr)
    {
        return;
    }
    for (uint8_t i = 0; i < device->interfaces.size(); ++i)
    {
        if (device->interfaces[i].dev_type == DevType::XBOX360W &&
            device->interfaces[i].itf_num != INVALID_IDX)
        {
            prime_port_for_pairing(dev_addr, i);
        }
    }
}

void start_xboxone(uint8_t dev_addr, uint8_t instance)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    if (interface == nullptr || interface->dev_type != DevType::XBOXONE)
    {
        return;
    }

    uint16_t vid = 0;
    uint16_t pid = 0;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    const bool arcade = interface->gip_arcade_stick || XboxArcadeStick::is_xbox_one_gip(vid, pid);

    interface->gip_power_sent = false;
    interface->gip_last_in_ok_ms = 0;
    interface->gip_last_in_arm_ms = 0;
    interface->gip_out_seq = 0;

    if (arcade)
    {
        /* XBOFS: write init, wait for OUT, then 30-byte read loop. */
        xboxone_send_power_on(interface, dev_addr, instance);
        wait_for_tx_complete(dev_addr, interface->ep_out, 500);
        service_usb_host_frames(8);
        if (receive_report(dev_addr, instance))
        {
            interface->gip_last_in_arm_ms = board_api::ms_since_boot();
        }
        service_usb_host_frames(8);
        return;
    }

    if (receive_report(dev_addr, instance))
    {
        interface->gip_last_in_arm_ms = board_api::ms_since_boot();
    }
    service_usb_host_frames();
    xboxone_init(interface, dev_addr, instance);
    service_usb_host_frames();
}

void service_gip(uint8_t dev_addr, uint8_t instance)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    if (interface == nullptr || interface->dev_type != DevType::XBOXONE ||
        interface->itf_num == INVALID_IDX)
    {
        return;
    }

    if (usbh_edpt_busy(dev_addr, interface->ep_in))
    {
        return;
    }

    const uint32_t now_ms = board_api::ms_since_boot();

    if (interface->gip_arcade_stick)
    {
        static constexpr uint32_t ARCADE_REARM_MS = 250;
        if (interface->gip_last_in_arm_ms != 0 &&
            (now_ms - interface->gip_last_in_arm_ms) < ARCADE_REARM_MS)
        {
            return;
        }
        if (receive_report(dev_addr, instance))
        {
            interface->gip_last_in_arm_ms = now_ms;
        }
        return;
    }

    static constexpr uint32_t IN_STALL_MS = 2000;
    static constexpr uint32_t IN_REARM_MIN_MS = 500;

    if (interface->gip_last_in_ok_ms != 0 &&
        (now_ms - interface->gip_last_in_ok_ms) < IN_STALL_MS)
    {
        return;
    }

    if (interface->gip_last_in_arm_ms != 0 &&
        (now_ms - interface->gip_last_in_arm_ms) < IN_REARM_MIN_MS)
    {
        return;
    }

    if (receive_report(dev_addr, instance))
    {
        interface->gip_last_in_arm_ms = now_ms;
    }
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

/** RUMBLE_ENABLE + player LED on an idle wireless port so sync can complete on any RF slot. */
static void prime_port_for_pairing(uint8_t dev_addr, uint8_t instance)
{
    Interface* interface = get_itf_by_instance(dev_addr, instance);
    if (interface == nullptr || interface->dev_type != DevType::XBOX360W ||
        interface->connected || interface->itf_num == INVALID_IDX)
    {
        return;
    }
    if (usbh_edpt_busy(dev_addr, interface->ep_out))
    {
        return;
    }
    send_report(dev_addr, instance, Xbox360W::RUMBLE_ENABLE, sizeof(Xbox360W::RUMBLE_ENABLE));
    set_led(dev_addr, instance, wireless_led_quadrant(dev_addr, instance), false);
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
            if (!interface->connected)
            {
                return false;
            }
            std::memcpy(buffer, Xbox360W::RUMBLE, sizeof(Xbox360W::RUMBLE));
            buffer[5] = rumble_l;
            buffer[6] = rumble_r;
            len = sizeof(Xbox360W::RUMBLE);
            break;
        case DevType::XBOX360:
            send_report(dev_addr, instance, Xbox360::RUMBLE, sizeof(Xbox360::RUMBLE));
            wait_for_tx_complete(dev_addr, interface->ep_out);

            std::memcpy(buffer, Xbox360::RUMBLE, sizeof(Xbox360::RUMBLE));
            buffer[3] = rumble_l;
            buffer[4] = rumble_r;
            len = sizeof(Xbox360::RUMBLE);
            break;
        case DevType::XBOXONE:
            std::memcpy(buffer, XboxOne::RUMBLE, sizeof(XboxOne::RUMBLE));
            buffer[2] = interface->gip_out_seq++;
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

void xbox360_chatpad_init(uint8_t address, uint8_t instance)
{
    TU_LOG1("XInput Chatpad Init\r\n");

    Interface* interface = get_itf_by_instance(address, instance);
    TU_VERIFY(interface != nullptr && interface->connected, );
    TU_VERIFY(interface->dev_type == DevType::XBOX360W, ); //Only supported on Xbox 360 Wireless atm, wired is more complicated

    send_report(address, instance, Xbox360W::CONTROLLER_INFO, sizeof(Xbox360W::CONTROLLER_INFO));
    wait_for_tx_complete(address, interface->ep_out);
    send_report(address, instance, Xbox360W::Chatpad::INIT, sizeof(Xbox360W::Chatpad::INIT));
    wait_for_tx_complete(address, interface->ep_out);
    send_report(address, instance, Xbox360W::RUMBLE_ENABLE, sizeof(Xbox360W::RUMBLE_ENABLE));
    wait_for_tx_complete(address, interface->ep_out);

    uint8_t led_ctrl[4];
    std::memcpy(led_ctrl, Xbox360W::Chatpad::LED_CTRL, sizeof(Xbox360W::Chatpad::LED_CTRL));
    led_ctrl[2] = Xbox360W::Chatpad::LED_ON[0];

    send_report(address, instance, led_ctrl, sizeof(led_ctrl));
    wait_for_tx_complete(address, interface->ep_out);

    interface->chatpad_inited = true;
    interface->chatpad_stage = ChatpadStage::KEEPALIVE_1;
}

bool xbox360_chatpad_keepalive(uint8_t address, uint8_t instance)
{   
    Interface* interface = get_itf_by_instance(address, instance);
    TU_VERIFY(interface != nullptr, false);
    TU_VERIFY(interface->connected && interface->chatpad_inited, false);

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
        default:
            break;
    }
    return true;
}

} // namespace tuh_xinput

#endif // (TUSB_OPT_HOST_ENABLED && CFG_TUH_XINPUT)