#pragma once

#include <stdint.h>
#include "common/class/rndis_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RNDIS_DATA_OFFSET(msg)  (((const usb_rndis_msg_data_t*)(msg))->DataOffset + \
                                 offsetof(usb_rndis_msg_data_t, DataOffset))

void rndis_init(uint16_t mtu, uint8_t mac_address[6]);

// Resp buf len should be at least MTU + 44 bytes, returns size of response
int32_t rndis_handle_msg(const uint8_t* msg, uint8_t* resp_buf, uint16_t resp_buf_len);

// bool rndis_format_frame_in(struct pbuf* p, uint8_t* frame_buf);

#ifdef __cplusplus
}
#endif