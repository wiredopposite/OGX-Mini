#pragma once

#include <stdint.h>
#include "usbd/usbd.h"

#ifdef __cplusplus
extern "C" {
#endif

usbd_handle_t* uart_bridge_init(usbd_hw_type_t hw_type);
void uart_bridge_task(usbd_handle_t* handle);

#ifdef __cplusplus
}
#endif