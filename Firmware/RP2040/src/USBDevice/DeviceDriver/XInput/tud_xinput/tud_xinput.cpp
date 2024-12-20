#include "tusb_option.h"

#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)

#include <cstring>
#include <algorithm>

#include "tusb.h"
#include "device/usbd_pvt.h"

#include "Descriptors/XInput.h"
#include "USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h"

namespace tud_xinput {

static constexpr uint8_t ENDPOINT_SIZE = 32;

uint8_t endpoint_in_ = 0xFF;
uint8_t endpoint_out_ = 0xFF;
uint8_t out_buffer_[ENDPOINT_SIZE];
uint8_t in_buffer_[ENDPOINT_SIZE];

//Class Driver 

static void init(void)
{
    endpoint_in_ = 0xFF;
    endpoint_out_ = 0xFF;
    std::memset(out_buffer_, 0, ENDPOINT_SIZE);
}

static bool deinit(void)
{
    init();
    return true;
}

static void reset(uint8_t rhport)
{
    init();
}

static uint16_t open(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length)
{
	uint16_t driver_length = static_cast<uint16_t>(sizeof(tusb_desc_interface_t) + (itf_descriptor->bNumEndpoints * sizeof(tusb_desc_endpoint_t)) + 16);

	TU_VERIFY(max_length >= driver_length, 0);

	uint8_t const *current_descriptor = tu_desc_next(itf_descriptor);
	uint8_t found_endpoints = 0;

	while ((found_endpoints < itf_descriptor->bNumEndpoints) && (driver_length <= max_length))
	{
		tusb_desc_endpoint_t const *endpoint_descriptor = reinterpret_cast<const tusb_desc_endpoint_t*>(current_descriptor);

		if (TUSB_DESC_ENDPOINT == tu_desc_type(endpoint_descriptor))
		{
			TU_ASSERT(usbd_edpt_open(rhport, endpoint_descriptor));

			if (tu_edpt_dir(endpoint_descriptor->bEndpointAddress) == TUSB_DIR_IN)
				endpoint_in_ = endpoint_descriptor->bEndpointAddress;
			else
				endpoint_out_ = endpoint_descriptor->bEndpointAddress;

			++found_endpoints;
		}

		current_descriptor = tu_desc_next(current_descriptor);
	}
	return driver_length;
}

static bool control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
	return true;
}

static bool xfer_callback(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
	if (ep_addr == endpoint_out_) 
    {
        usbd_edpt_xfer(BOARD_TUD_RHPORT, endpoint_out_, out_buffer_, ENDPOINT_SIZE);
    }
	return true;
}

// Public API

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
        .xfer_cb = xfer_callback,
        .sof = NULL
    };
    return &tud_class_driver_;
}

bool send_report_ready()
{
    TU_VERIFY(tud_ready() && endpoint_in_ != 0xFF && !usbd_edpt_busy(BOARD_TUD_RHPORT, endpoint_in_));
    return true;
}

bool send_report(const uint8_t *report, uint16_t len)
{
    usbd_edpt_claim(BOARD_TUD_RHPORT, endpoint_in_);
    usbd_edpt_xfer(BOARD_TUD_RHPORT, endpoint_in_, const_cast<uint8_t*>(report), ENDPOINT_SIZE);
    usbd_edpt_release(BOARD_TUD_RHPORT, endpoint_in_);
    return true;
}

bool receive_report(uint8_t *report, uint16_t len)
{
    TU_VERIFY(endpoint_out_ != 0xFF && len <= ENDPOINT_SIZE);

    if (tud_ready() && !usbd_edpt_busy(BOARD_TUD_RHPORT, endpoint_out_))
    {
        usbd_edpt_xfer(BOARD_TUD_RHPORT, endpoint_out_, out_buffer_, len);
    }

    std::memcpy(report, out_buffer_, len);
    return true;
}

}; // namespace TUDXInput

#endif // (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)