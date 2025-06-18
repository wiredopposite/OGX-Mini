#pragma once

#include "ring/ring_usb.h"

typedef enum {
    RING_BT_TYPE_NONE = 0,
    RING_BT_TYPE_PAD,
    RING_BT_TYPE_RUMBLE,
    RING_BT_TYPE_PCM,
} ring_bt_type_t;

typedef ring_usb_t ring_bluetooth_t;
typedef ring_usb_packet_t ring_bluetooth_packet_t;

#define RING_BLUETOOTH_SIZE       RING_USB_SIZE
#define RING_BLUETOOTH_BUF_SIZE   RING_USB_BUF_SIZE

#define ring_bluetooth_push   ring_usb_push
#define ring_bluetooth_pop    ring_usb_pop
#define ring_bluetooth_empty  ring_usb_empty