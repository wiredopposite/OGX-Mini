#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "common/tusb_types.h"
#include "usb/host/host_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- HID/XID/XInput/GIP Class Driver --- */

#define USB_REQUEST_TYPE(recipient, type, direction) \
    ((uint8_t)(((recipient) & 0x1F) | (((type) & 0x03) << 5) | (((direction) & 0x01) << 7)))

typedef void (*hxx_send_complete_cb_t)(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len, bool success, void* context);
typedef void (*hxx_ctrl_complete_cb_t)(uint8_t daddr, const uint8_t* data, uint16_t len, bool success, void* context);

/* ---- USB Host Callbacks ---- */

/**
 * @brief Called when the USB interface is mounted.
 * 
 * @param daddr The device address.
 * @param itf_num The interface number.
 * @param desc_report Pointer to the descriptor report if available.
 * @param len Length of the descriptor report.
 */
void tuh_hxx_mounted_cb(usbh_type_t type, uint8_t daddr, uint8_t itf_num, const uint8_t* desc_report, uint16_t len);

/**
 * @brief Called when the USB interface is unmounted.
 * 
 * @param daddr The device address.
 * @param itf_num The interface number.
 */
void tuh_hxx_unmounted_cb(uint8_t daddr, uint8_t itf_num);

/**
 * @brief Called when a report is received from the USB host.
 * 
 * @param daddr The device address.
 * @param itf_num The interface number.
 * @param data Pointer to the received data.
 * @param len Length of the received data.
 */
void tuh_hxx_report_received_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len);

/* ---- USB Host API ---- */

/**
 * @brief Send a control transfer to the USB device.
 * 
 * @param daddr The device address.
 * @param request Pointer to the control request.
 * @param data Pointer to the data buffer for the control transfer.
 * @param complete_cb Callback function to be called when the transfer is complete.
 * @param context User-defined context to be passed to the callback.
 * 
 * @return true if the control transfer was initiated successfully, false otherwise.
 */
bool tuh_hxx_ctrl_xfer(uint8_t daddr, const tusb_control_request_t* request, uint8_t* data, 
                       hxx_ctrl_complete_cb_t complete_cb, void* context);

/**
 * @brief Queue a receive report request for the USB device.
 * 
 * @param daddr The device address.
 * @param itf_num The interface number.
 * 
 * @return true if the request was queued successfully, false otherwise.
 */
bool tuh_hxx_receive_report(uint8_t daddr, uint8_t itf_num);

/**
 * @brief Check if the USB device is ready to send a report.
 * 
 * @param daddr The device address.
 * @param itf_num The interface number.
 * 
 * @return true if the device is ready to send a report, false otherwise.
 */
bool tuh_hxx_send_report_ready(uint8_t daddr, uint8_t itf_num);

/**
 * @brief Send a report to the USB device with a callback.
 * 
 * @param daddr The device address.
 * @param itf_num The interface number.
 * @param data Pointer to the data buffer to be sent.
 * @param len Length of the data buffer.
 * @param complete_cb Callback function to be called when the transfer is complete.
 * @param context User-defined context to be passed to the callback.
 * 
 * @return The length of the send operation, or -1 on failure.
 */
int32_t tuh_hxx_send_report_with_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len, 
                                    hxx_send_complete_cb_t complete_cb, void* context);
/**
 * @brief Send a report to the USB device.
 * 
 * @param daddr The device address.
 * @param itf_num The interface number.
 * @param data Pointer to the data buffer to be sent.
 * @param len Length of the data buffer.
 * 
 * @return The length of the send operation, or -1 on failure.
 */
static inline int32_t tuh_hxx_send_report(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len) {
    return tuh_hxx_send_report_with_cb(daddr, itf_num, data, len, NULL, NULL);
}

/* ---- Class driver ---- */

bool hxx_init_cb(void);
bool hxx_deinit_cb(void);
bool hxx_open_cb(uint8_t rhport, uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len);
bool hxx_set_config_cb(uint8_t daddr, uint8_t itf_num);
bool hxx_ep_xfer_cb(uint8_t daddr, uint8_t epaddr, xfer_result_t result, uint32_t len);
void hxx_close_cb(uint8_t daddr);

#ifdef __cplusplus
}
#endif