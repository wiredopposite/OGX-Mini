// #pragma once

// #include <stdint.h>
// #include <stdbool.h>
// #include <stddef.h>
// #include "common/tusb_types.h"
// #include "usb/host/host.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

// typedef void (*xa_send_complete_cb_t)(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len, bool success, void* context);

// void tuh_xaudio_init_cb(usbh_type_t type, uint8_t daddr, uint8_t itf_num);
// void tuh_xaudio_mounted_cb(uint8_t daddr, uint8_t itf_num);
// void tuh_xaudio_unmounted_cb(uint8_t daddr, uint8_t itf_num);
// void tuh_xaudio_data_received_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len);
// void tuh_xaudio_data_ctrl_received_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len);

// bool tuh_xaudio_receive_data(uint8_t daddr, uint8_t itf_num);
// bool tuh_xaudio_receive_data_ctrl(uint8_t daddr, uint8_t itf_num);

// int32_t tuh_xaudio_send_data_with_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len, 
//                                      xa_send_complete_cb_t complete_cb, void* context);

// int32_t tuh_xaudio_send_data_ctrl_with_cb(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len, 
//                                           xa_send_complete_cb_t complete_cb, void* context);

// static inline int32_t tuh_xaudio_send_data(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len) {
//     return tuh_xaudio_send_data_with_cb(daddr, itf_num, data, len, NULL, NULL);
// }

// static inline int32_t tuh_xaudio_send_data_ctrl(uint8_t daddr, uint8_t itf_num, const uint8_t* data, uint16_t len) {
//     return tuh_xaudio_send_data_ctrl_with_cb(daddr, itf_num, data, len, NULL, NULL);
// }

// /* ---- Class driver ---- */

// bool xaudio_init_cb(void);
// bool xaudio_deinit_cb(void);
// bool xaudio_open_cb(uint8_t rhport, uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len);
// bool xaudio_set_config_cb(uint8_t daddr, uint8_t itf_num);
// bool xaudio_ep_xfer_cb(uint8_t daddr, uint8_t epaddr, xfer_result_t result, uint32_t len);
// void xaudio_close_cb(uint8_t daddr);

// #ifdef __cplusplus
// }
// #endif