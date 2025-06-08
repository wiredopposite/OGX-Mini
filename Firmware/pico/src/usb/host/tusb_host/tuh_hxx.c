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
#define HXX_INTERFACES_MAX      (GAMEPADS_MAX * 2U) // X2 for chatpads
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

typedef struct {
    uint8_t         daddr;
    uint8_t         itf_num;
    usbh_type_t     dev_type;
    struct {
        uint8_t     idle_rate;
        uint8_t     protocol;
        uint16_t    status;
        uint8_t     desc_report_type;
        uint16_t    desc_report_len;
    } hid;
    ep_buf_t        eps[EP_COUNT];
} interface_t;

typedef struct {
    uint16_t                vid;
    uint16_t                pid;
    hxx_ctrl_complete_cb_t  ctrl_complete_cb;
    void*                   ctrl_complete_ctx;
} device_t;

static interface_t interfaces[HXX_INTERFACES_MAX] = {0};
static device_t devices[CFG_TUH_DEVICE_MAX]  = {0};
// static size_t dev_size = sizeof(devices);
// static size_t itf_size = sizeof(interfaces);

/* ---- Helpers ---- */

static inline uint8_t get_itf_num(uint8_t daddr, uint8_t epaddr) {
    for (uint8_t i = 0; i < HXX_INTERFACES_MAX; i++) {
        if (interfaces[i].daddr != daddr) {
            continue;
        }
        for (uint8_t j = 0; j < EP_COUNT; j++) {
            if (interfaces[i].eps[j].epaddr == epaddr) {
                return interfaces[i].itf_num;
            }
        }
    }
    return INVALID_ITF_NUM;
}

static inline usbh_ep_type_t get_ep_type(interface_t* itf, uint8_t epaddr) {
    for (uint8_t i = 0; i < EP_COUNT; i++) {
        if (itf->eps[i].epaddr == epaddr) {
            return (usbh_ep_type_t)i;
        }
    }
    return EP_COUNT;
}

static inline interface_t* get_itf(uint8_t daddr, uint8_t itf_num) {
    for (uint8_t i = 0; i < HXX_INTERFACES_MAX; i++) {
        if ((interfaces[i].daddr == daddr) && 
            (interfaces[i].itf_num == itf_num)) {
            return &interfaces[i];
        }
    }
    return NULL;
}

static inline interface_t* get_free_itf(void) {
    for (uint8_t i = 0; i < HXX_INTERFACES_MAX; i++) {
        if (interfaces[i].itf_num == INVALID_ITF_NUM) {
            return &interfaces[i];
        }
    }
    return NULL;
}

static bool allocate_itf(device_t* dev, interface_t* itf, 
                         const tusb_desc_interface_t* desc_itf) {
    uint8_t itf_num = desc_itf->bInterfaceNumber;
    itf->dev_type = USBH_TYPE_NONE;

    if (desc_itf->bInterfaceClass == USB_CLASS_HID) {
        itf->dev_type = USBH_TYPE_HID_GENERIC;
        for (uint8_t i = 0; i < ARRAY_SIZE(HW_IDS_MAP); i++) {
            for (uint8_t j = 0; j < HW_IDS_MAP[i].num_ids; j++) {
                if ((dev->vid == HW_IDS_MAP[i].ids[j].vid) && 
                    (dev->pid == HW_IDS_MAP[i].ids[j].pid)) {
                    itf->dev_type = HW_IDS_MAP[i].type;
                }
            }
        }
    } else {
        if (desc_itf->bInterfaceClass == USB_ITF_CLASS_XID &&
            desc_itf->bInterfaceSubClass == USB_ITF_SUBCLASS_XID) {
            itf->dev_type = USBH_TYPE_XID;
        } else if (desc_itf->bInterfaceSubClass == USB_SUBCLASS_XINPUT) {
            switch (desc_itf->bInterfaceProtocol) {
            case USB_PROTOCOL_XINPUT_GP:
                itf->dev_type = USBH_TYPE_XINPUT;
                break;
            case USB_PROTOCOL_XINPUT_PLUGIN:
                itf->dev_type = USBH_TYPE_XINPUT_CHATPAD;
                break;
            case USB_PROTOCOL_XINPUT_GP_WL:
                itf->dev_type = USBH_TYPE_XINPUT_WL;
                break;
            default:
                break;
            }
        } else if (desc_itf->bInterfaceSubClass == USB_ITF_SUBCLASS_XGIP && 
                desc_itf->bInterfaceProtocol == USB_ITF_PROTOCOL_XGIP &&
                itf_num == XGIP_ITF_NUM_GAMEPAD) {
            itf->dev_type = USBH_TYPE_XGIP;
        }
    }
    itf->itf_num = (itf->dev_type != USBH_TYPE_NONE) ? itf_num : INVALID_ITF_NUM;
    return (itf->itf_num != INVALID_ITF_NUM);
}

static void tuh_ctrl_xfer_cb(tuh_xfer_t* xfer) {
    device_t* dev = &devices[xfer->daddr - 1];
    if (dev->ctrl_complete_cb != NULL) {
        hxx_ctrl_complete_cb_t complete_cb = dev->ctrl_complete_cb;
        dev->ctrl_complete_cb = NULL;
        uint8_t* data = (xfer->buffer != NULL) ? xfer->buffer : NULL;
        uint16_t len =  (xfer->setup != NULL) 
                        ? tu_le16toh(xfer->setup->wLength) : 0;
        complete_cb(xfer->daddr, data, len, 
                    (xfer->result == XFER_RESULT_SUCCESS), 
                    (void*)xfer->user_data);
    }
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
        .buffer      = NULL,
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
        .buffer      = NULL,
        .complete_cb = complete_cb,
        .user_data   = user_data
    };
    return tuh_control_xfer(&xfer);
}

static void hid_config_complete(uint8_t daddr, uint8_t itf_num, 
                                uint8_t const* desc_report, uint16_t desc_len) {
    interface_t* itf = get_itf(daddr, itf_num);
    TU_VERIFY(itf != NULL,);
    if (desc_len == 0 || desc_report == NULL) {
        ogxm_logd("HID Mount complete without report descriptor\r\n");
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
    interface_t* itf = get_itf(daddr, itf_num);
    TU_VERIFY(itf != NULL,);

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
            hid_config_complete(daddr, itf_num, NULL, 0);
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
    tuh_xfer_t xfer;
    xfer.daddr = daddr;
    xfer.result = XFER_RESULT_SUCCESS;
    xfer.setup = &request;
    xfer.user_data = HID_INIT_SET_IDLE;
    hid_process_set_config(&xfer);
}

/* ---- TUH driver ---- */

bool hxx_init_cb(void) {
    memset(devices, 0, sizeof(devices));
    memset(interfaces, 0, sizeof(interfaces));
    for (uint8_t i = 0; i < HXX_INTERFACES_MAX; i++) {
        interfaces[i].itf_num = INVALID_ITF_NUM;
    }
    return true;
}

bool hxx_deinit_cb(void) {
    hxx_init_cb();
}

bool hxx_open_cb(uint8_t rhport, uint8_t daddr, 
                 tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
    ogxm_logd("HXX open cb\n");
    if ((daddr == 0) || (daddr > CFG_TUH_DEVICE_MAX)) {
        return false;
    }
    device_t* dev = &devices[daddr - 1];
    interface_t* itf = get_free_itf();
    TU_VERIFY(itf != NULL, false);
    itf->daddr = daddr;

    if ((dev->vid == 0) && (dev->pid == 0)) {
        tuh_vid_pid_get(daddr, &dev->vid, &dev->pid);
    }
    TU_VERIFY(allocate_itf(dev, itf, desc_itf), false);

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
    ogxm_logd("Itf %d %sopened\n", itf->itf_num, opened ? "" : "not ");
    if (!opened) {
        itf->itf_num = INVALID_ITF_NUM;
    }
    return opened;
}

bool hxx_set_config_cb(uint8_t daddr, uint8_t itf_num) {
    interface_t* itf = get_itf(daddr, itf_num);
    ogxm_logd("Set config callback for daddr %d, itf_num %d, dev type: %d\n", 
        daddr, itf_num, itf ? itf->dev_type : -1);
    TU_VERIFY(itf != NULL, false);

    if (itf->dev_type >= USBH_TYPE_HID_GENERIC) {
        ogxm_logd("HID device detected, initializing...\r\n");
        hid_init(daddr, itf_num);
        return true;
    }
    ogxm_logd("Non-HID device detected: %d\n", itf->dev_type);
    usbh_driver_set_config_complete(daddr, itf_num);
    tuh_hxx_mounted_cb(itf->dev_type, daddr, itf_num, NULL, 0);
    return true;
}

bool hxx_ep_xfer_cb(uint8_t daddr, uint8_t epaddr, xfer_result_t result, uint32_t len) {
    interface_t* itf = get_itf(daddr, get_itf_num(daddr, epaddr));
    TU_VERIFY(itf != NULL, false);
    usbh_ep_type_t ep_type = get_ep_type(itf, epaddr);

    switch (ep_type) {
    case EP_OUT:
        if (itf->eps[EP_OUT].int_complete_cb != NULL) {
            ep_complete_cb_t int_callback = itf->eps[EP_OUT].int_complete_cb;
            itf->eps[EP_OUT].int_complete_cb = NULL;
            int_callback(daddr, itf->itf_num, result, len);
        } else if (itf->eps[EP_OUT].ext_send_cb != NULL) {
            hxx_send_complete_cb_t ext_callback = itf->eps[EP_OUT].ext_send_cb;
            void* ctx = itf->eps[EP_OUT].ext_send_ctx;
            itf->eps[EP_OUT].ext_send_cb = NULL;
            itf->eps[EP_OUT].ext_send_ctx = NULL;
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
    for (uint8_t i = 0; i < HXX_INTERFACES_MAX; i++) {
        if (interfaces[i].daddr != daddr ||
            interfaces[i].itf_num == INVALID_ITF_NUM) {
            continue;
        }
        tuh_hxx_unmounted_cb(daddr, interfaces[i].itf_num);
        memset(&interfaces[i], 0, sizeof(interface_t));
        interfaces[i].itf_num = INVALID_ITF_NUM;
    }
    memset(&devices[daddr - 1], 0, sizeof(device_t));
}

/* ---- Public API ---- */

bool tuh_hxx_ctrl_xfer(uint8_t daddr, const tusb_control_request_t* request, uint8_t* data, 
                       hxx_ctrl_complete_cb_t complete_cb, void* context) {
    tuh_xfer_t xfer = {
        .daddr = daddr,
        .ep_addr = 0,
        .setup = request, 
        .buffer = data,
        .complete_cb = complete_cb ? tuh_ctrl_xfer_cb : NULL, 
        .user_data = (uintptr_t)context
    };
    if (tuh_control_xfer(&xfer)) {
        device_t* dev = &devices[daddr - 1];
        dev->ctrl_complete_cb = complete_cb;
        return true;
    }
    return false;
}

int32_t tuh_hxx_send_report_with_cb(uint8_t daddr, uint8_t itf_num, 
                                    const uint8_t* data, uint16_t len, 
                                    hxx_send_complete_cb_t complete_cb, void* context) {
    TU_VERIFY(daddr <= CFG_TUH_DEVICE_MAX, -1);
    interface_t* itf = get_itf(daddr, itf_num);
    TU_VERIFY(itf != NULL, -1);

    itf->eps[EP_OUT].ext_send_cb = complete_cb;
    itf->eps[EP_OUT].ext_send_ctx = context;
    return usb_host_ep_send(daddr, &itf->eps[EP_OUT], data, len);
}

bool tuh_hxx_receive_report(uint8_t daddr, uint8_t itf_num) {
    TU_VERIFY(daddr <= CFG_TUH_DEVICE_MAX, -1);
    interface_t* itf = get_itf(daddr, itf_num);
    TU_VERIFY(itf != NULL, -1);

    TU_VERIFY(usbh_edpt_claim(daddr, itf->eps[EP_IN].epaddr), false);

    if (!usbh_edpt_xfer(daddr, itf->eps[EP_IN].epaddr, 
                        itf->eps[EP_IN].buf, itf->eps[EP_IN].size)) {
        usbh_edpt_release(daddr, itf->eps[EP_IN].epaddr);
        return false;
    }
    return true;
}