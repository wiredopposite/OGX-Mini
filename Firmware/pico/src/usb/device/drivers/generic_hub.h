#pragma once

#include <stdint.h>
#include "usbd/usbd.h"
#include "usb/device/device_private.h"

#ifdef __cplusplus
extern "C" {
#endif

usbd_handle_t* generic_hub_init(const usb_device_driver_cfg_t* config);
/* Needed so driver can reset downstream ports in response to feature requests */
void generic_hub_register_ds_port(usbd_handle_t* handle);

#ifdef __cplusplus
}
#endif