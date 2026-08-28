#include "tusb_option.h"
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "tusb.h"
#include "device/usbd_pvt.h"

#include "Descriptors/XInput.h"
#include "USBDevice/DeviceManager.h"
#include "USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h"

namespace tud_xinput {

static constexpr uint16_t ENDPOINT_SIZE = 32;
static constexpr uint8_t DESC_TYPE_VENDOR = 0x21;
static constexpr uint8_t DESC_TYPE_SEC   = 0x41;

uint8_t endpoint_in_ = 0xFF;
uint8_t endpoint_out_ = 0xFF;
uint8_t ep_in_buffer_[ENDPOINT_SIZE];
uint8_t ep_out_buffer_[ENDPOINT_SIZE];

static void init(void)
{
    endpoint_in_ = 0xFF;
    endpoint_out_ = 0xFF;
    std::memset(ep_out_buffer_, 0, ENDPOINT_SIZE);
    std::memset(ep_in_buffer_, 0, ENDPOINT_SIZE);
}

static bool deinit(void)
{
    init();
    return true;
}

static void reset(uint8_t rhport)
{
    (void)rhport;
    init();
}

static inline uint16_t tu_desc_len(uint8_t const* desc) { return desc[0]; }

// Match joypad-os: claim all 4 interfaces, but only OPEN endpoints for gamepad (0x01).
// Audio (0x03) and plugin (0x02) endpoints are skipped (stub). Security (0xFD/0x13) has no EPs.
static uint16_t open(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length)
{
    uint16_t drv_len = sizeof(tusb_desc_interface_t);
    uint8_t const *p_desc = tu_desc_next((uint8_t const*)itf_descriptor);

    if (itf_descriptor->bInterfaceClass != 0xFF)
        return 0;

    // --- Interface 0: Gamepad (0x5D/0x01) — open endpoints ---
    if (itf_descriptor->bInterfaceSubClass == 0x5D && itf_descriptor->bInterfaceProtocol == 0x01)
    {
        if (p_desc[1] == DESC_TYPE_VENDOR) {
            drv_len += tu_desc_len(p_desc);
            p_desc = tu_desc_next(p_desc);
        }
        for (uint8_t i = 0; i < itf_descriptor->bNumEndpoints; i++) {
            tusb_desc_endpoint_t const *ep_desc = (tusb_desc_endpoint_t const *)p_desc;
            if (TUSB_DESC_ENDPOINT != tu_desc_type(ep_desc)) break;
            TU_VERIFY(usbd_edpt_open(rhport, ep_desc), 0);
            if (tu_edpt_dir(ep_desc->bEndpointAddress) == TUSB_DIR_IN)
                endpoint_in_ = ep_desc->bEndpointAddress;
            else
                endpoint_out_ = ep_desc->bEndpointAddress;
            drv_len += sizeof(tusb_desc_endpoint_t);
            p_desc = tu_desc_next(p_desc);
        }
        if (endpoint_out_ != 0xFF)
            usbd_edpt_xfer(rhport, endpoint_out_, ep_out_buffer_, ENDPOINT_SIZE);
        TU_VERIFY(max_length >= drv_len, 0);
        return drv_len;
    }

    // --- Interface 1: Audio (0x5D/0x03) — skip endpoints (stub) ---
    if (itf_descriptor->bInterfaceSubClass == 0x5D && itf_descriptor->bInterfaceProtocol == 0x03)
    {
        if (p_desc[1] == DESC_TYPE_VENDOR) {
            drv_len += tu_desc_len(p_desc);
            p_desc = tu_desc_next(p_desc);
        }
        for (uint8_t i = 0; i < itf_descriptor->bNumEndpoints; i++) {
            drv_len += sizeof(tusb_desc_endpoint_t);
            p_desc = tu_desc_next(p_desc);
        }
        TU_VERIFY(max_length >= drv_len, 0);
        return drv_len;
    }

    // --- Interface 2: Plugin (0x5D/0x02) — skip endpoint (stub) ---
    if (itf_descriptor->bInterfaceSubClass == 0x5D && itf_descriptor->bInterfaceProtocol == 0x02)
    {
        if (p_desc[1] == DESC_TYPE_VENDOR) {
            drv_len += tu_desc_len(p_desc);
            p_desc = tu_desc_next(p_desc);
        }
        for (uint8_t i = 0; i < itf_descriptor->bNumEndpoints; i++) {
            drv_len += sizeof(tusb_desc_endpoint_t);
            p_desc = tu_desc_next(p_desc);
        }
        TU_VERIFY(max_length >= drv_len, 0);
        return drv_len;
    }

    // --- Interface 3: Security (0xFD/0x13) — skip 0x41 descriptor, no EPs ---
    if (itf_descriptor->bInterfaceSubClass == 0xFD && itf_descriptor->bInterfaceProtocol == 0x13)
    {
        if (p_desc[1] == DESC_TYPE_SEC) {
            drv_len += tu_desc_len(p_desc);
        }
        TU_VERIFY(max_length >= drv_len, 0);
        return drv_len;
    }

    return 0;
}

// Forward interface-directed control requests (e.g. XSM3 from 360) to the device driver.
// The 360 may send XSM3 as Class requests to the security interface; TinyUSB routes those here.
static bool control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    printf("tud_xinput ctrl bReq 0x%02x stage %u type 0x%02x wIdx 0x%04x\n",
        (unsigned)request->bRequest, (unsigned)stage, (unsigned)request->bmRequestType, (unsigned)request->wIndex);
    return DeviceManager::get_instance().get_driver()->vendor_control_xfer_cb(rhport, stage, request);
}

static bool xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    (void)result;
    (void)xferred_bytes;
    if (ep_addr == endpoint_out_)
        usbd_edpt_xfer(rhport, endpoint_out_, ep_out_buffer_, ENDPOINT_SIZE);
    return true;
}

const usbd_class_driver_t* class_driver()
{
    static const usbd_class_driver_t tud_class_driver_ =
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XINPUT",
#else
        .name = NULL,
#endif
        .init = init,
        .deinit = deinit,
        .reset = reset,
        .open = open,
        .control_xfer_cb = control_xfer_cb,
        .xfer_cb = xfer_cb,
        .sof = NULL
    };
    return &tud_class_driver_;
}

bool send_report_ready()
{
    return tud_ready() &&
           (endpoint_in_ != 0xFF) &&
           (!usbd_edpt_busy(BOARD_TUD_RHPORT, endpoint_in_));
}

bool receive_report_ready()
{
    return tud_ready() &&
           (endpoint_out_ != 0xFF) &&
           (!usbd_edpt_busy(BOARD_TUD_RHPORT, endpoint_out_));
}

bool send_report(const uint8_t *report, uint16_t len)
{
    if (send_report_ready())
    {
        std::memcpy(ep_in_buffer_, report, std::min(len, (uint16_t)ENDPOINT_SIZE));
        usbd_edpt_claim(BOARD_TUD_RHPORT, endpoint_in_);
        usbd_edpt_xfer(BOARD_TUD_RHPORT, endpoint_in_, ep_in_buffer_, sizeof(XInput::InReport));
        usbd_edpt_release(BOARD_TUD_RHPORT, endpoint_in_);
        return true;
    }
    return false;
}

bool receive_report(uint8_t *report, uint16_t len)
{
    if (receive_report_ready())
    {
        usbd_edpt_claim(BOARD_TUD_RHPORT, endpoint_out_);
        usbd_edpt_xfer(BOARD_TUD_RHPORT, endpoint_out_, ep_out_buffer_, ENDPOINT_SIZE);
        usbd_edpt_release(BOARD_TUD_RHPORT, endpoint_out_);
    }
    std::memcpy(report, ep_out_buffer_, std::min(len, (uint16_t)ENDPOINT_SIZE));
    return true;
}

} // namespace tud_xinput

#endif // (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)
