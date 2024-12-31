#include "tusb_option.h"
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XID)

#include <cstring>

#include "USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.h"
#include "Descriptors/XboxOG.h"

#if defined(XREMOTE_ROM_AVAILABLE)
    #define XREMOTE_ENABLED 1
    #include "USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid_xremote_rom.h"
#else
    #define XREMOTE_ENABLED 0
#endif

namespace tud_xid {

static constexpr uint16_t ENDPOINT_SIZE = 32;
static constexpr uint8_t INTERFACE_MULT = XREMOTE_ENABLED + 1;
static constexpr uint8_t INTERFACE_CLASS = 0x58;
static constexpr uint8_t INTERFACE_SUBCLASS = 0x42;

enum class RequestType { SET_REPORT, GET_REPORT, GET_DESC, GET_CAPABILITIES_IN, GET_CAPABILITIES_OUT, UNKNOWN };

struct Interface
{
    Type type{Type::GAMEPAD};

    uint8_t itf_num{0xFF};
    
    uint8_t ep_in{0xFF};
    uint8_t ep_out{0xFF};

    uint16_t ep_in_size{ENDPOINT_SIZE};
    uint16_t ep_out_size{ENDPOINT_SIZE};

    std::array<uint8_t, ENDPOINT_SIZE> ep_in_buffer;
    std::array<uint8_t, ENDPOINT_SIZE> ep_out_buffer;

    Interface()
    {
        ep_out_buffer.fill(0);
        ep_in_buffer.fill(0);
    }
};

static std::array<Interface, CFG_TUD_XID * INTERFACE_MULT> interfaces_;
static Type xid_type_{Type::GAMEPAD};

static inline RequestType get_request_type(const tusb_control_request_t *request)
{
    if (request->bmRequestType == XboxOG::GET_REPORT_REQ_TYPE && request->bRequest == XboxOG::GET_REPORT_REQ && request->wValue == XboxOG::GET_REPORT_VALUE)
    {
        return RequestType::GET_REPORT;
    }
    if (request->bmRequestType == XboxOG::SET_REPORT_REQ_TYPE && request->bRequest == XboxOG::SET_REPORT_REQ && request->wValue == XboxOG::SET_REPORT_VALUE && request->wLength == static_cast<uint16_t>(0x06))
    {
        return RequestType::SET_REPORT;
    }
    if (request->bmRequestType == XboxOG::GET_DESC_REQ_TYPE && request->bRequest == XboxOG::GET_DESC_REQ && request->wValue == XboxOG::GET_DESC_VALUE)
    {
        return RequestType::GET_DESC;
    }
    if (request->bmRequestType == XboxOG::GET_CAP_REQ_TYPE && request->bRequest == XboxOG::GET_CAP_REQ)
    {
        if (request->wValue == XboxOG::GET_CAP_VALUE_IN)
        {
            return RequestType::GET_CAPABILITIES_IN;
        }
        else if (request->wValue == XboxOG::GET_CAP_VALUE_OUT)
        {
            return RequestType::GET_CAPABILITIES_OUT;
        }
    }
    return RequestType::UNKNOWN;
}

static inline uint8_t get_idx_by_itf(uint8_t itf)
{
    for (uint8_t i = 0; i < interfaces_.size(); i++)
    {
        if (interfaces_[i].itf_num == itf)
        {
            return i;
        }
        if (interfaces_[i].type == Type::XREMOTE)
        {
            if (itf == interfaces_[i].itf_num + 1)
            {
                return i;
            }
        }
    }
    return 0xFF;
}

static inline uint8_t get_idx_by_edpt(uint8_t edpt)
{
    for (uint8_t i = 0; i < interfaces_.size(); i++)
    {
        if (interfaces_[i].ep_in == edpt || interfaces_[i].ep_out == edpt)
        {
            return i;
        }
    }
    return 0xFF;
}

uint8_t get_index_by_type(uint8_t type_idx, Type type)
{
    uint8_t type_idx_ = 0;
    for (uint8_t i = 0; i < interfaces_.size(); i++)
    {
        if (interfaces_[i].type == type)
        {
            if (type_idx_ == type_idx)
            {
                return i;
            }
            type_idx_++;
        }
    }
    return 0xFF;
}

static inline Interface* find_available_interface()
{
    for (Interface &itf : interfaces_)
    {
        if (itf.itf_num == 0xFF)
        {
            return &itf;
        }
    }
    return nullptr;
}

const uint8_t *xremote_get_rom()
{
#if XREMOTE_ENABLED
    return XRemote::ROM;
#else
    return nullptr;
#endif 
}

//Class driver

static void xid_init()
{
    for (Interface &itf : interfaces_)
    {
        itf = Interface();
        itf.type = xid_type_;
    }
}

static bool xid_deinit()
{
    xid_init();
    return true;
}

static void xid_reset(uint8_t rhport)
{
    xid_init();
}

static uint16_t xid_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{
    TU_VERIFY(itf_desc->bInterfaceClass == INTERFACE_CLASS, 0);
    TU_VERIFY(itf_desc->bInterfaceSubClass == INTERFACE_SUBCLASS, 0);

    Interface *interface = find_available_interface();
    TU_ASSERT(interface != nullptr, 0);

    uint16_t driver_len = 0;

    switch (interface->type)
    {
        case Type::GAMEPAD:
            driver_len = XboxOG::GP::DRIVER_LEN;
            break;
        case Type::STEELBATTALION:
            driver_len = XboxOG::SB::DRIVER_LEN;
            break;
        case Type::XREMOTE:
            driver_len = XboxOG::XR::DRIVER_LEN;
            break;
    }

    TU_ASSERT(max_len >= driver_len, 0);

    const uint8_t *desc_ep = reinterpret_cast<const uint8_t*>(itf_desc);
    int endpoint = 0;
    int pos = 0;

    while (endpoint < itf_desc->bNumEndpoints && pos < max_len)
    {
        if (tu_desc_type(desc_ep) != TUSB_DESC_ENDPOINT)
        {
            pos += tu_desc_len(desc_ep);
            desc_ep = tu_desc_next(desc_ep);
            continue;
        }

        const tusb_desc_endpoint_t *ep_desc = reinterpret_cast<const tusb_desc_endpoint_t*>(desc_ep);
        TU_VERIFY(TUSB_DESC_ENDPOINT == ep_desc->bDescriptorType, 0);
        TU_VERIFY(usbd_edpt_open(rhport, ep_desc), 0);

        if (tu_edpt_dir(ep_desc->bEndpointAddress) == TUSB_DIR_IN)
        {
            interface->ep_in  = ep_desc->bEndpointAddress;
            interface->ep_in_size = ep_desc->wMaxPacketSize;
        }
        else
        {
            interface->ep_out = ep_desc->bEndpointAddress;
            interface->ep_out_size = ep_desc->wMaxPacketSize;
        }

        interface->itf_num = itf_desc->bInterfaceNumber;

        pos += tu_desc_len(desc_ep);
        desc_ep = tu_desc_next(desc_ep);
        endpoint++;
    }

    return driver_len;
}

static bool xid_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    // uint8_t index = get_idx_by_edpt(ep_addr);

    // TU_VERIFY(result == XFER_RESULT_SUCCESS, true);
    // TU_VERIFY(index != 0xFF, true);
    // TU_VERIFY(xferred_bytes < ENDPOINT_SIZE, true);

    return true;
}

bool xremote_control_xfer(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request, Interface *interface)
{
    if (request->bmRequestType == 0xC1 && request->bRequest == 0x01 && request->wIndex == 1 && request->wValue == 0x0000)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending XREMOTE INFO\r\n");
            const uint8_t *rom = xremote_get_rom();
            if (rom == nullptr)
            {
                return false; //STALL
            }
            tud_control_xfer(rhport, request, const_cast<uint8_t*>(&rom[0]), request->wLength);
        }
        return true;
    }
    //ROM DATA (Interface 1)
    else if (request->bmRequestType == 0xC1 && request->bRequest == 0x02 && request->wIndex == 1)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            const uint8_t *rom = xremote_get_rom();
            if (rom == nullptr)
            {
                return false; //STALL
            }
            tud_control_xfer(rhport, request, const_cast<uint8_t*>(&rom[request->wValue * 1024]), request->wLength);
        }
        return true;
    }

    return false;
}

bool steelbattalion_control_xfer(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request, Interface *p_xid);
bool duke_control_xfer(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request, Interface *p_xid);

bool xid_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    TU_VERIFY(request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE);

    uint8_t index = get_idx_by_itf(static_cast<uint8_t>(request->wIndex));
    TU_VERIFY(index != 0xFF, false);

    Interface& interface = interfaces_[index];

    bool ret = false;

     //Get HID Report
    if (request->bmRequestType == 0xA1 && request->bRequest == 0x01 && request->wValue == 0x0100)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending HID report on control pipe for index %02x\n", request->wIndex);
            tud_control_xfer(rhport, request, interface.ep_in_buffer.data(), std::min(request->wLength, interface.ep_in_size));
        }
        return true;
    }

    //Set HID Report
    if (request->bmRequestType == 0x21 && request->bRequest == 0x09 && request->wValue == 0x0200 && request->wLength == 0x06)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            //Host is sending a rumble command to control pipe. Queue receipt.
            tud_control_xfer(rhport, request, interface.ep_out_buffer.data(), std::min(request->wLength, interface.ep_out_size));
        }
        else if (stage == CONTROL_STAGE_ACK)
        {
            //Receipt complete. Copy data to rumble struct
            TU_LOG1("Got HID report from control pipe for index %02x\n", request->wIndex);
            // memcpy(interface.out, interface.ep_out_buff, min(request->wLength, sizeof(interface.out)));
        }
        return true;
    }

    switch (interface.type)
    {
        case Type::GAMEPAD:
            ret = duke_control_xfer(rhport, stage, request, &interface);
            break;
        case Type::STEELBATTALION:
            ret = steelbattalion_control_xfer(rhport, stage, request, &interface);
            break;
        case Type::XREMOTE:
            ret = xremote_control_xfer(rhport, stage, request, &interface);
            break;
        default:
            break;
    }

    if (ret == false)
    {
        TU_LOG1("STALL: wIndex: %02x bmRequestType: %02x, bRequest: %02x, wValue: %04x\n",
                request->wIndex,
                request->bmRequestType,
                request->bRequest,
                request->wValue);
        return false;
    }

    return true;
}

//Public API

//Call first before device stack is initialized with tud_init(), will default to Duke/Gamepad otherwise
void initialize(Type xid_type)
{
    xid_type_ = xid_type;
}

const usbd_class_driver_t* class_driver()
{
    static const usbd_class_driver_t tud_xid_class_driver =
    {
    #if CFG_TUSB_DEBUG >= 2
        .name = "XID",
    #else
        .name = nullptr,
    #endif
        .init = xid_init,
        .deinit = xid_deinit,
        .reset = xid_reset,
        .open = xid_open,
        .control_xfer_cb = xid_control_xfer_cb,
        .xfer_cb = xid_xfer_cb,
        .sof = NULL
    };
    return &tud_xid_class_driver;
}

bool send_report_ready(uint8_t index)
{
    TU_VERIFY(index < interfaces_.size(), false);
    TU_VERIFY(interfaces_[index].ep_in != 0xFF, false);
    return (tud_ready() && !usbd_edpt_busy(BOARD_TUD_RHPORT, interfaces_[index].ep_in));
}

bool send_report(uint8_t index, const uint8_t* report, uint16_t len)
{
    TU_VERIFY(len < ENDPOINT_SIZE, false);
    TU_VERIFY(send_report_ready(index), false);

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }

    uint16_t size = std::min(len, interfaces_[index].ep_in_size);

    std::memcpy(interfaces_[index].ep_in_buffer.data(), report, size);

    return usbd_edpt_xfer(BOARD_TUD_RHPORT, interfaces_[index].ep_in, interfaces_[index].ep_in_buffer.data(), size);
}

bool receive_report(uint8_t index, uint8_t *report, uint16_t len)
{
    TU_VERIFY(index < interfaces_.size());
    TU_VERIFY(interfaces_[index].ep_out != 0xFF);
    TU_VERIFY(len < ENDPOINT_SIZE);

    uint16_t size = std::min(len, interfaces_[index].ep_out_size);

    if (tud_ready() && !usbd_edpt_busy(BOARD_TUD_RHPORT, interfaces_[index].ep_out))
    {
        usbd_edpt_xfer(BOARD_TUD_RHPORT, interfaces_[index].ep_out, interfaces_[index].ep_out_buffer.data(), size);
    }

    std::memcpy(report, interfaces_[index].ep_out_buffer.data(), size);
    return true;
}

bool xremote_rom_available()
{
    return (xremote_get_rom() != nullptr);
}

bool steelbattalion_control_xfer(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request, Interface *p_xid)
{
    if (request->bmRequestType == 0xC1 && request->bRequest == 0x06 && request->wValue == 0x4200)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending STEELBATTALION_DESC_XID\n");
            tud_control_xfer(rhport, request, (void *)XboxOG::SB::XID_DEVICE_DESCRIPTORS, sizeof(XboxOG::SB::XID_DEVICE_DESCRIPTORS));
        }
        return true;
    }
    else if (request->bmRequestType == 0xC1 && request->bRequest == 0x01 && request->wValue == 0x0100)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending STEELBATTALION_CAPABILITIES_IN\n");
            tud_control_xfer(rhport, request, (void *)XboxOG::SB::XID_CAPABILITIES_IN, sizeof(XboxOG::SB::XID_CAPABILITIES_IN));
        }
        return true;
    }
    else if (request->bmRequestType == 0xC1 && request->bRequest == 0x01 && request->wValue == 0x0200)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending STEELBATTALION_CAPABILITIES_OUT\n");
            tud_control_xfer(rhport, request, (void *)XboxOG::SB::XID_CAPABILITIES_OUT, sizeof(XboxOG::SB::XID_CAPABILITIES_OUT));
        }
        return true;
    }
    return false;
}
bool duke_control_xfer(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request, Interface *p_xid)
{
    if (request->bmRequestType == 0xC1 && request->bRequest == 0x06 && request->wValue == 0x4200)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending DUKE_DESC_XID\n");
            tud_control_xfer(rhport, request, (void *)XboxOG::GP::XID_DEVICE_DESCRIPTORS, sizeof(XboxOG::GP::XID_DEVICE_DESCRIPTORS));
        }
        return true;
    }
    else if (request->bmRequestType == 0xC1 && request->bRequest == 0x01 && request->wValue == 0x0100)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending DUKE_CAPABILITIES_IN\n");
            tud_control_xfer(rhport, request, (void *)XboxOG::GP::XID_CAPABILITIES_IN, sizeof(XboxOG::GP::XID_CAPABILITIES_IN));
        }
        return true;
    }
    else if (request->bmRequestType == 0xC1 && request->bRequest == 0x01 && request->wValue == 0x0200)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            TU_LOG1("Sending DUKE_CAPABILITIES_OUT\n");
            tud_control_xfer(rhport, request, (void *)XboxOG::SB::XID_CAPABILITIES_OUT, sizeof(XboxOG::SB::XID_CAPABILITIES_OUT));
        }
        return true;
    }
    return false;
}

} // namespace TUDXID

#endif // TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XID