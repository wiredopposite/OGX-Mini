#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    USBD_TYPE_XBOXOG_GP = 0,
    USBD_TYPE_XBOXOG_SB,
    USBD_TYPE_XBOXOG_XR,
    USBD_TYPE_XINPUT,
    USBD_TYPE_PS3,
    USBD_TYPE_PSCLASSIC,
    USBD_TYPE_SWITCH,
    // USBD_TYPE_WEBAPP,
    // USBD_TYPE_UART_BRIDGE,
    USBD_TYPE_COUNT
} usbd_type_t;

#ifdef __cplusplus
}
#endif