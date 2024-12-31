#include "tusb_option.h"
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)

#include <cstring>
#include <algorithm>

#include "tusb.h"
#include "device/usbd_pvt.h"

#include "Descriptors/XInput.h"
#include "USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.h"

namespace tud_xinput {

static constexpr uint16_t ENDPOINT_SIZE = 32;

uint8_t endpoint_in_ = 0xFF;
uint8_t endpoint_out_ = 0xFF;
uint8_t ep_in_buffer_[ENDPOINT_SIZE];
uint8_t ep_out_buffer_[ENDPOINT_SIZE];

//Class Driver 

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
    init();
}

static uint16_t open(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length)
{
	uint16_t driver_length = sizeof(tusb_desc_interface_t) + (itf_descriptor->bNumEndpoints * sizeof(tusb_desc_endpoint_t)) + 16;

	TU_VERIFY(max_length >= driver_length, 0);

	uint8_t const *current_descriptor = tu_desc_next(itf_descriptor);
	uint8_t found_endpoints = 0;
	while ((found_endpoints < itf_descriptor->bNumEndpoints) && (driver_length <= max_length))
	{
		tusb_desc_endpoint_t const *endpoint_descriptor = (tusb_desc_endpoint_t const *)current_descriptor;
		if (TUSB_DESC_ENDPOINT == tu_desc_type(endpoint_descriptor))
		{
			TU_ASSERT(usbd_edpt_open(rhport, endpoint_descriptor));

			if (tu_edpt_dir(endpoint_descriptor->bEndpointAddress) == TUSB_DIR_IN)
            {
				endpoint_in_ = endpoint_descriptor->bEndpointAddress;
            }
			else
            {
				endpoint_out_ = endpoint_descriptor->bEndpointAddress;
            }
            
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

static bool xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
	if (ep_addr == endpoint_out_) 
    {
        usbd_edpt_xfer(BOARD_TUD_RHPORT, endpoint_out_, ep_out_buffer_, ENDPOINT_SIZE);
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
        .xfer_cb = xfer_cb,
        .sof = NULL
    };
    return &tud_class_driver_;
}

bool send_report_ready()
{
    if (tud_ready() && 
        (endpoint_in_ != 0xFF) && 
        (!usbd_edpt_busy(BOARD_TUD_RHPORT, endpoint_in_)))
    {
        return true;
    }
    return false;
}

bool receive_report_ready()
{
    if (tud_ready() && 
        (endpoint_out_ != 0xFF) && 
        (!usbd_edpt_busy(BOARD_TUD_RHPORT, endpoint_out_)))
    {
        return true;
    }
    return false;
}

bool send_report(const uint8_t *report, uint16_t len)
{
    if (send_report_ready())
    {
        std::memcpy(ep_in_buffer_, report, std::min(len, ENDPOINT_SIZE));
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

    std::memcpy(report, ep_out_buffer_, std::min(len, ENDPOINT_SIZE));
    return true;
}

} // namespace tud_xinput

#endif // (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)