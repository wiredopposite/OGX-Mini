#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/hid/hid.h"

#include "log/log.h"
#include "board_config.h"
#include "usb/descriptors/xid.h"
#include "usb/descriptors/xinput.h"
#include "usb/descriptors/xgip.h"
#include "usb/host/tusb_host/hardware_ids.h"
#include "usb/host/tusb_host/tuh_hxx.h"

#define INVALID_ITF_NUM         ((uint8_t)0xFF)
#define USB_EP_IN               ((uint8_t)0x80)
#define HXX_INTERFACES_MAX      8U
#define HXX_EPSIZE_MAX          ((uint16_t)64)

typedef void (*ep_complete_cb_t)(uint8_t daddr, uint8_t itf_num, xfer_result_t result, uint32_t len);

typedef enum {
    EP_IN,
    EP_OUT,
    EP_COUNT
} usbh_ep_type_t;

typedef enum {
    HID_INIT_SET_IDLE,
    HID_INIT_SET_PROTOCOL,
    HID_INIT_GET_REPORT_DESC,
    HID_INIT_COMPLETE
} hid_init_state_t;

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
} desc_header_t;

typedef struct {
    uint8_t                 epaddr;
    uint16_t                size;
    uint8_t                 buf[HXX_EPSIZE_MAX] __attribute__((aligned(4)));
    ep_complete_cb_t        int_complete_cb;
    hxx_send_complete_cb_t  ext_send_cb;
    void*                   ext_send_ctx;
} ep_buf_t;

class HxxInterface {
public:
    HxxInterface() = default;
    ~HxxInterface() = default;

    bool            alloc;
    usbh_type_t     dev_type{USBH_TYPE_NONE};
    uint8_t         itf_num{0};
    ep_buf_t        eps[EP_COUNT];

    struct {
        uint8_t     idle_rate{0};
        uint8_t     protocol{HID_ITF_PROTOCOL_NONE};
        uint16_t    status{0};
        uint8_t     desc_report_type{0};
        uint16_t    desc_report_len{0};
    } hid;

    void reset(void) {
        alloc = false;
        dev_type = USBH_TYPE_NONE;
        itf_num = INVALID_ITF_NUM;
        memset(eps, 0, sizeof(eps));
        hid.idle_rate = 0;
        hid.protocol = HID_ITF_PROTOCOL_NONE;
        hid.status = 0;
        hid.desc_report_type = 0;
        hid.desc_report_len = 0;
    }
};

class HxxDevice {
public:
    uint16_t vid{0};
    uint16_t pid{0};

    HxxInterface* get_itf_by_ep(uint8_t epaddr) {
        for (auto& itf : interfaces) {
            if (!itf.alloc) {
                continue;
            }
            for (auto& ep : itf.eps) {
                if (ep.epaddr == epaddr) {
                    return &itf;
                }
            }
        }
        return nullptr;
    }

    HxxInterface* get_itf_by_itf_num(uint8_t itf_num) {
        if (itf_num >= HXX_INTERFACES_MAX) {
            return nullptr;
        }
        return itf_lookup[itf_num];
    }

    bool allocate_itf(uint8_t itf_num, usbh_type_t dev_type) {
        if (itf_num >= HXX_INTERFACES_MAX) {
            return false;
        }
        if (itf_lookup[itf_num] != nullptr) {
            return false;
        }
        for (auto& itf : interfaces) {
            if (!itf.alloc) {
                itf_lookup[itf_num] = &itf;
                break;
            }
        }
        if (itf_lookup[itf_num] == nullptr) {
            return false;
        }
        itf_lookup[itf_num]->itf_num = itf_num;
        itf_lookup[itf_num]->dev_type = dev_type;
        itf_lookup[itf_num]->alloc = true;
        return true;
    }

    void deallocate_itf(uint8_t itf_num) {
        if ((itf_num >= HXX_INTERFACES_MAX) || 
            (itf_lookup[itf_num] == nullptr)) {
            return;
        }
        itf_lookup[itf_num]->reset();
        itf_lookup[itf_num] = nullptr;
    }

    void set_ctrl_complete_cb(hxx_ctrl_complete_cb_t cb, void* ctx) {
        ctrl_complete_cb = cb;
        ctrl_complete_ctx = ctx;
    }

    void call_ctrl_complete_cb(uint8_t daddr, const uint8_t* data, uint16_t len, bool success) {
        hxx_ctrl_complete_cb_t callback = ctrl_complete_cb;
        void* ctx = ctrl_complete_ctx;
        ctrl_complete_cb = nullptr;
        ctrl_complete_ctx = nullptr;
        if (callback != nullptr) {
            callback(daddr, data, len, success, ctx);
        }
    }

    void reset_all(void) {
        vid = 0;
        pid = 0;
        ctrl_complete_cb = nullptr;
        ctrl_complete_ctx = nullptr;
        for (auto& itf : interfaces) {
            itf.reset();
        }
    }
private:
    #define TRUE_ITF_MAX (GAMEPADS_MAX * 2)

    HxxInterface                    interfaces[TRUE_ITF_MAX];
    HxxInterface*                   itf_lookup[HXX_INTERFACES_MAX]{nullptr};
    hxx_ctrl_complete_cb_t          ctrl_complete_cb{nullptr};
    void*                           ctrl_complete_ctx{nullptr};
};

static HxxDevice hxx_devices[CFG_TUH_DEVICE_MAX];

/* ---- Helpers ---- */

static inline usbh_ep_type_t get_ep_type(HxxInterface* itf, uint8_t epaddr) {
    for (uint8_t i = 0; i < EP_COUNT; i++) {
        if (itf->eps[i].epaddr == epaddr) {
            return (usbh_ep_type_t)i;
        }
    }
    return EP_COUNT;
}

static usbh_type_t get_itf_type(uint16_t vid, uint16_t pid, const tusb_desc_interface_t* desc_itf) {
    usbh_type_t type = USBH_TYPE_NONE;

    if (desc_itf->bInterfaceClass == USB_CLASS_HID) {
        type = USBH_TYPE_HID_GENERIC;
        for (uint8_t i = 0; i < ARRAY_SIZE(HW_IDS_MAP); i++) {
            for (uint8_t j = 0; j < HW_IDS_MAP[i].num_ids; j++) {
                if ((vid == HW_IDS_MAP[i].ids[j].vid) && 
                    (pid == HW_IDS_MAP[i].ids[j].pid)) {
                    type = HW_IDS_MAP[i].type;
                }
            }
        }
    } else {
        if (desc_itf->bInterfaceClass == USB_ITF_CLASS_XID &&
            desc_itf->bInterfaceSubClass == USB_ITF_SUBCLASS_XID) {
            type = USBH_TYPE_XID;
        } else if ((desc_itf->bInterfaceClass == USB_CLASS_VENDOR) &&
                   (desc_itf->bInterfaceSubClass == USB_SUBCLASS_XINPUT)) {
            switch (desc_itf->bInterfaceProtocol) {
            case USB_PROTOCOL_XINPUT_GP:
                type = USBH_TYPE_XINPUT;
                break;
            case USB_PROTOCOL_XINPUT_PLUGIN:
                // Wired chatpad is not supported yet
#if 0
                type = USBH_TYPE_XINPUT_CHATPAD;
#endif
                break;
            case USB_PROTOCOL_XINPUT_GP_WL:
                /*  Tinyusb mounts interfaces backwards, if itf_num/2 >= GAMEPADS_MAX
                    and we assign it a gamepad, there will be no more room 
                    left for the first gamepad. Xinput wireless interfaces are 
                    itf0=gamepad1, itf1=audio1, itf3=gamepad2, itf4=audio2, etc. */
                if ((desc_itf->bInterfaceNumber / 2) >= GAMEPADS_MAX) {
                    break;
                }
                type = USBH_TYPE_XINPUT_WL;
                break;
            default:
                break;
            }
        } else if (desc_itf->bInterfaceSubClass == USB_ITF_SUBCLASS_XGIP && 
                desc_itf->bInterfaceProtocol == USB_ITF_PROTOCOL_XGIP &&
                desc_itf->bInterfaceNumber == XGIP_ITF_NUM_GAMEPAD) {
            type = USBH_TYPE_XGIP;
        }
    }
    return type;
}

static void tuh_ctrl_xfer_cb(tuh_xfer_t* xfer) {
    HxxDevice* dev = &hxx_devices[xfer->daddr - 1];
    uint8_t* data = (xfer->buffer != nullptr) ? xfer->buffer : nullptr;
    uint16_t len =  (xfer->setup != nullptr) 
                    ? tu_le16toh(xfer->setup->wLength) : 0;
    bool success = (xfer->result == XFER_RESULT_SUCCESS);
    dev->call_ctrl_complete_cb(xfer->daddr, data, len, success);
}

static inline int32_t usb_host_ep_send(uint8_t daddr, ep_buf_t* ep_buf, 
                                       const uint8_t* data, uint16_t len) {
    TU_VERIFY((ep_buf->size > 0) && (ep_buf->epaddr != 0), -1);
    TU_VERIFY(!(ep_buf->epaddr & USB_EP_IN), -1);
    TU_VERIFY(usbh_edpt_claim(daddr, ep_buf->epaddr), -1);

    uint16_t xfer_len = MIN(len, ep_buf->size);
    memcpy(ep_buf->buf, data, xfer_len);

    if (!usbh_edpt_xfer(daddr, ep_buf->epaddr, ep_buf->buf, xfer_len)) {
        usbh_edpt_release(daddr, ep_buf->epaddr);
        return -1;
    }
    return xfer_len;
}

/* ---- HID ---- */

static bool hid_set_protocol(uint8_t daddr, uint8_t itf_num, uint8_t protocol,
                             tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
    tusb_control_request_t const request = {
        .bmRequestType_bit = {
            .recipient = TUSB_REQ_RCPT_INTERFACE,
            .type      = TUSB_REQ_TYPE_CLASS,
            .direction = TUSB_DIR_OUT
        },
        .bRequest = HID_REQ_CONTROL_SET_PROTOCOL,
        .wValue   = protocol,
        .wIndex   = itf_num,
        .wLength  = 0
    };
    tuh_xfer_t xfer = {
        .daddr       = daddr,
        .ep_addr     = 0,
        .setup       = &request,
        .buffer      = nullptr,
        .complete_cb = complete_cb,
        .user_data   = user_data
    };
    return tuh_control_xfer(&xfer);
}

static bool hid_set_idle(uint8_t daddr, uint8_t itf_num, uint16_t idle_rate,
                         tuh_xfer_cb_t complete_cb, uintptr_t user_data) {
    tusb_control_request_t const request = {
        .bmRequestType_bit = {
            .recipient = TUSB_REQ_RCPT_INTERFACE,
            .type      = TUSB_REQ_TYPE_CLASS,
            .direction = TUSB_DIR_OUT
        },
        .bRequest = HID_REQ_CONTROL_SET_IDLE,
        .wValue   = tu_htole16(idle_rate),
        .wIndex   = tu_htole16((uint16_t) itf_num),
        .wLength  = 0
    };
    tuh_xfer_t xfer = {
        .daddr       = daddr,
        .ep_addr     = 0,
        .setup       = &request,
        .buffer      = nullptr,
        .complete_cb = complete_cb,
        .user_data   = user_data
    };
    return tuh_control_xfer(&xfer);
}

static void hid_config_complete(uint8_t daddr, uint8_t itf_num, 
                                uint8_t const* desc_report, uint16_t desc_len) {
    HxxInterface* itf = hxx_devices[daddr - 1].get_itf_by_itf_num(itf_num);
    TU_VERIFY(itf != nullptr,);
    if (desc_len == 0 || desc_report == nullptr) {
        ogxm_logv("HID Mount complete without report descriptor\r\n");
        return;
    }
    tuh_hxx_mounted_cb(itf->dev_type, daddr, itf_num, desc_report, desc_len);
}

static void hid_process_set_config(tuh_xfer_t* xfer) {
    if (!((xfer->setup->bRequest == HID_REQ_CONTROL_SET_IDLE) ||
          (xfer->setup->bRequest == HID_REQ_CONTROL_SET_PROTOCOL))) {
        TU_ASSERT(xfer->result == XFER_RESULT_SUCCESS,);
    }
    uintptr_t const state = xfer->user_data;
    uint8_t const itf_num = (uint8_t)tu_le16toh(xfer->setup->wIndex);
    uint8_t const daddr = xfer->daddr;
    HxxInterface* itf = hxx_devices[daddr - 1].get_itf_by_itf_num(itf_num);
    TU_VERIFY(itf != nullptr,);

    switch (state) {
    case HID_INIT_SET_IDLE: 
        {
        const uint16_t idle_rate = 0;
        const uintptr_t next_state = (itf->hid.protocol != HID_ITF_PROTOCOL_NONE)
                                     ? HID_INIT_SET_PROTOCOL : HID_INIT_GET_REPORT_DESC;
        hid_set_idle(daddr, itf_num, idle_rate, hid_process_set_config, next_state);
        }
        break;
    case HID_INIT_SET_PROTOCOL:
        hid_set_protocol(daddr, itf_num, itf->hid.protocol, 
                         hid_process_set_config, HID_INIT_GET_REPORT_DESC);
        break;
    case HID_INIT_GET_REPORT_DESC:
        if (itf->hid.desc_report_len > CFG_TUH_ENUMERATION_BUFSIZE) {
            ogxm_loge("HID Skip Report Descriptor since it is too large %u bytes\r\n", 
                itf->hid.desc_report_len);
            hid_config_complete(daddr, itf_num, nullptr, 0);
        } else {
            tuh_descriptor_get_hid_report(daddr, itf_num, itf->hid.desc_report_type, 0,
                                          usbh_get_enum_buf(), itf->hid.desc_report_len,
                                          hid_process_set_config, HID_INIT_COMPLETE);
        }
        break;

    case HID_INIT_COMPLETE: 
        {
        uint8_t const* desc_report = usbh_get_enum_buf();
        uint16_t const desc_len = tu_le16toh(xfer->setup->wLength);
        hid_config_complete(daddr, itf_num, desc_report, desc_len);
        break;
        }
    default:
        break;
    }
}

void hid_init(uint8_t daddr, uint8_t itf_num) {
    tusb_control_request_t request;
    request.wIndex = (uint16_t)itf_num;
    tuh_xfer_t xfer = {0};
    xfer.daddr = daddr;
    xfer.result = XFER_RESULT_SUCCESS;
    xfer.setup = &request;
    xfer.user_data = HID_INIT_SET_IDLE;
    hid_process_set_config(&xfer);
}

/* ---- TUH driver ---- */

bool hxx_init_cb(void) {
    for (auto& dev : hxx_devices) {
        dev.reset_all();
    }
    return true;
}

bool hxx_deinit_cb(void) {
    return hxx_init_cb();
}

bool hxx_open_cb(uint8_t rhport, uint8_t daddr, 
                 tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
    ogxm_logv("HXX open cb\n");
    TU_VERIFY(desc_itf != nullptr, false);
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX, false);
    TU_VERIFY(desc_itf->bInterfaceNumber < HXX_INTERFACES_MAX, false);

    HxxDevice* dev = &hxx_devices[daddr - 1];
    tuh_vid_pid_get(daddr, &dev->vid, &dev->pid);

    usbh_type_t dev_type = get_itf_type(dev->vid, dev->pid, desc_itf);
    TU_VERIFY(dev_type != USBH_TYPE_NONE, false);
    if (!dev->allocate_itf(desc_itf->bInterfaceNumber, dev_type)) {
        ogxm_loge("Failed to allocate interface %d for device %d\n", 
            desc_itf->bInterfaceNumber, daddr);
        return false;
    }
    HxxInterface* itf = dev->get_itf_by_itf_num(desc_itf->bInterfaceNumber);
    TU_VERIFY(itf != nullptr, false);

    const uint8_t* desc_p = (const uint8_t*)desc_itf;
    const uint8_t* desc_end = desc_p + max_len;
    uint8_t eps = 0;
    bool opened = false;

    while ((desc_p < desc_end) && (eps < desc_itf->bNumEndpoints)) {
        desc_p += tu_desc_len(desc_p);
        if (desc_p >= desc_end) {
            break;
        }
        const desc_header_t* desc_hdr = (const desc_header_t*)desc_p;
        if ((desc_hdr->bLength == 0) || 
            (desc_hdr->bDescriptorType == TUSB_DESC_INTERFACE)) {
            break;
        }
        if ((desc_hdr->bDescriptorType == HID_DESC_TYPE_HID) &&
            (itf->dev_type >= USBH_TYPE_HID_GENERIC)) {
            const tusb_hid_descriptor_hid_t* desc_hid = 
                (const tusb_hid_descriptor_hid_t*)desc_p;
            itf->hid.desc_report_type = desc_hid->bReportType;
            itf->hid.desc_report_len = tu_unaligned_read16(&desc_hid->wReportLength);
            continue;
        }
        if (desc_hdr->bDescriptorType != TUSB_DESC_ENDPOINT) {
            continue;
        }
        eps++;

        const tusb_desc_endpoint_t* desc_ep = (const tusb_desc_endpoint_t*)desc_p;

        if (desc_ep->bEndpointAddress & USB_EP_IN) {
            if (itf->eps[EP_IN].epaddr == 0) {
                if ((tu_edpt_packet_size(desc_ep) > HXX_EPSIZE_MAX) ||
                    !tuh_edpt_open(daddr, desc_ep)) {
                    ogxm_loge("Failed to open IN endpoint %02X\n", 
                        desc_ep->bEndpointAddress);
                    continue;
                }
                itf->eps[EP_IN].epaddr = desc_ep->bEndpointAddress;
                itf->eps[EP_IN].size = tu_edpt_packet_size(desc_ep);
            } else {
                ogxm_loge("Too many IN endpoints\n");
                continue;
            }
        } else {
            if (itf->eps[EP_OUT].epaddr == 0) {
                if ((tu_edpt_packet_size(desc_ep) > HXX_EPSIZE_MAX) ||
                    !tuh_edpt_open(daddr, desc_ep)) {
                    ogxm_loge("Failed to open OUT endpoint %02X\n", 
                        desc_ep->bEndpointAddress);
                    continue;
                }
                itf->eps[EP_OUT].epaddr = desc_ep->bEndpointAddress;
                itf->eps[EP_OUT].size = tu_edpt_packet_size(desc_ep);
            } else {
                ogxm_loge("Too many IN endpoints\n");
                continue;
            }
        }
        opened = true;
    }
    ogxm_logv("Itf %d %sopened\n", itf->itf_num, opened ? "" : "not ");
    if (!opened) {
        dev->deallocate_itf(desc_itf->bInterfaceNumber);
    }
    return opened;
}

bool hxx_set_config_cb(uint8_t daddr, uint8_t itf_num) {
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX, false);
    HxxInterface* itf = hxx_devices[daddr - 1].get_itf_by_itf_num(itf_num);
    ogxm_logv("Set config callback for daddr %d, itf_num %d, dev type: %d\n", 
        daddr, itf_num, (itf != nullptr) ? itf->dev_type : -1);
    TU_VERIFY(itf != nullptr, false);

    if (itf->dev_type >= USBH_TYPE_HID_GENERIC) {
        ogxm_logv("HID device detected, initializing...\r\n");
        hid_init(daddr, itf_num);
        return true;
    }
    ogxm_logv("Non-HID device detected: %d\n", itf->dev_type);
    usbh_driver_set_config_complete(daddr, itf_num);
    tuh_hxx_mounted_cb(itf->dev_type, daddr, itf_num, nullptr, 0);
    return true;
}

bool hxx_ep_xfer_cb(uint8_t daddr, uint8_t epaddr, xfer_result_t result, uint32_t len) {
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX, false);
    HxxInterface* itf = hxx_devices[daddr - 1].get_itf_by_ep(epaddr);
    TU_VERIFY(itf != nullptr, false);
    usbh_ep_type_t ep_type = get_ep_type(itf, epaddr);

    switch (ep_type) {
    case EP_OUT:
        if (itf->eps[EP_OUT].int_complete_cb != nullptr) {
            ep_complete_cb_t int_callback = itf->eps[EP_OUT].int_complete_cb;
            itf->eps[EP_OUT].int_complete_cb = nullptr;
            int_callback(daddr, itf->itf_num, result, len);
        } else if (itf->eps[EP_OUT].ext_send_cb != nullptr) {
            hxx_send_complete_cb_t ext_callback = itf->eps[EP_OUT].ext_send_cb;
            void* ctx = itf->eps[EP_OUT].ext_send_ctx;
            itf->eps[EP_OUT].ext_send_cb = nullptr;
            itf->eps[EP_OUT].ext_send_ctx = nullptr;
            ext_callback(daddr, itf->itf_num, itf->eps[EP_OUT].buf, len, 
                         (result == XFER_RESULT_SUCCESS), ctx);
        }
        return true;
    case EP_IN:
        if (result == XFER_RESULT_SUCCESS) {
            tuh_hxx_report_received_cb(daddr, itf->itf_num, itf->eps[EP_IN].buf, len);
        } else {
            tuh_hxx_receive_report(daddr, itf->itf_num);
        }
        return true;
    default:
        return false;
    }
}

void hxx_close_cb(uint8_t daddr) {
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX,);
    HxxDevice* dev = &hxx_devices[daddr - 1];
    for (uint8_t i = 0; i < HXX_INTERFACES_MAX; i++) {
        HxxInterface* itf = dev->get_itf_by_itf_num(i);
        if (itf == nullptr) {
            continue;
        }
        tuh_hxx_unmounted_cb(daddr, itf->itf_num);
        for (auto& ep : itf->eps) {
            if (ep.epaddr != 0) {
                tuh_edpt_close(daddr, ep.epaddr);
            }
        }
        dev->deallocate_itf(itf->itf_num);
    }
}

/* ---- Public API ---- */

bool tuh_hxx_ctrl_xfer(uint8_t daddr, const tusb_control_request_t* request, uint8_t* data, 
                       hxx_ctrl_complete_cb_t complete_cb, void* context) {
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX, false);
    TU_VERIFY(request != nullptr, false);

    tuh_xfer_t xfer = {
        .daddr = daddr,
        .ep_addr = 0,
        .setup = request, 
        .buffer = data,
        .complete_cb = complete_cb ? tuh_ctrl_xfer_cb : nullptr, 
        .user_data = (uintptr_t)context
    };
    if (tuh_control_xfer(&xfer)) {
        HxxDevice* dev = &hxx_devices[daddr - 1];
        dev->set_ctrl_complete_cb(complete_cb, context);
        return true;
    }
    return false;
}

int32_t tuh_hxx_send_report_with_cb(uint8_t daddr, uint8_t itf_num, 
                                    const uint8_t* data, uint16_t len, 
                                    hxx_send_complete_cb_t complete_cb, void* context) {
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX, -1);
    HxxInterface* itf = hxx_devices[daddr - 1].get_itf_by_itf_num(itf_num);
    TU_VERIFY(itf != nullptr, -1);

    itf->eps[EP_OUT].ext_send_cb = complete_cb;
    itf->eps[EP_OUT].ext_send_ctx = context;
    return usb_host_ep_send(daddr, &itf->eps[EP_OUT], data, len);
}

bool tuh_hxx_receive_report(uint8_t daddr, uint8_t itf_num) {
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX, false);
    HxxInterface* itf = hxx_devices[daddr - 1].get_itf_by_itf_num(itf_num);
    TU_VERIFY(itf != nullptr, -1);

    TU_VERIFY(usbh_edpt_claim(daddr, itf->eps[EP_IN].epaddr), false);

    if (!usbh_edpt_xfer(daddr, itf->eps[EP_IN].epaddr, 
                        itf->eps[EP_IN].buf, itf->eps[EP_IN].size)) {
        usbh_edpt_release(daddr, itf->eps[EP_IN].epaddr);
        return false;
    }
    return true;
}

bool tuh_hxx_send_report_ready(uint8_t daddr, uint8_t itf_num) {
    TU_VERIFY((daddr - 1) < CFG_TUH_DEVICE_MAX, false);
    HxxInterface* itf = hxx_devices[daddr - 1].get_itf_by_itf_num(itf_num);
    TU_VERIFY(itf != nullptr, false);
    return !usbh_edpt_busy(daddr, itf->eps[EP_OUT].epaddr);
}