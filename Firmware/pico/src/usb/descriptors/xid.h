#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_DTYPE_XID                   ((uint8_t)0x42)
#define USB_REQ_XID_GET_CAPABILITIES    ((uint8_t)0x01)
#define USB_XID_REPORT_CAPABILITIES_IN  ((uint8_t)0x01)
#define USB_XID_REPORT_CAPABILITIES_OUT ((uint8_t)0x02)
#define USB_ITF_CLASS_XID               ((uint8_t)0x58)
#define USB_ITF_SUBCLASS_XID            ((uint8_t)0x42)

typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdXid;
    uint8_t bType;
    uint8_t bSubType;
    uint8_t bMaxInputReportSize;
    uint8_t bMaxOutputReportSize;
    uint16_t wAlternateProductIds[4];
} usb_desc_xid_t;

#ifdef __cplusplus
}
#endif