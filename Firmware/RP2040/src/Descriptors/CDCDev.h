#ifndef _CDC_DEV_DESCRIPTORS_H_
#define _CDC_DEV_DESCRIPTORS_H_

#include <cstdint>
#include <cstring>

#include "tusb.h"

namespace CDCDesc 
{
    #define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )

    static const uint16_t VID = 0xCafe;
    static const uint16_t PID = (0x4000 | _PID_MAP(CDC, 0));
    static const uint16_t USB_VER = 0x0200;

    const tusb_desc_device_t DESC_DEVICE = 
    {
        .bLength            = sizeof(tusb_desc_device_t),
        .bDescriptorType    = TUSB_DESC_DEVICE,
        .bcdUSB             = USB_VER,
        .bDeviceClass       = TUSB_CLASS_MISC,
        .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol    = MISC_PROTOCOL_IAD,
        .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor           = VID,
        .idProduct          = PID,
        .bcdDevice          = 0x0100,
        .iManufacturer      = 0x01,
        .iProduct           = 0x02,
        .iSerialNumber      = 0x03,
        .bNumConfigurations = 0x01
    };

    enum Itf
    {
        CDC_0 = 0,
        CDC_0_DATA,
        ITF_TOTAL
    };

    static const int CONFIG_LEN = (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN);

    static const uint8_t DESC_CONFIG[] =
    {
        TUD_CONFIG_DESCRIPTOR(1, Itf::ITF_TOTAL, 0, CONFIG_LEN, 0x00, 500),
        TUD_CDC_DESCRIPTOR(Itf::CDC_0, 4, 0x80 | (Itf::CDC_0 + 1), 8, (Itf::CDC_0 + 2), 0x80 | (Itf::CDC_0 + 2), 64),
    };

    static const uint8_t STRING_DESC_LANGUAGE[] = { 0x09, 0x04 };
    static const uint8_t STRING_MANUFACTURER[]  = "Wired Opposite";
    static const uint8_t STRING_PRODUCT[]       = "OGX-Mini";
    static const uint8_t STRING_INTERFACE[]     = "OGX-Mini CDC";

    static const uint8_t *DESC_STRING[] =
    {
        STRING_DESC_LANGUAGE,
        STRING_MANUFACTURER,
        STRING_PRODUCT,
        nullptr, //Serial
        STRING_INTERFACE
    };

}; // namespace CDCDesc

#endif // _CDC_DEV_DESCRIPTORS_H_