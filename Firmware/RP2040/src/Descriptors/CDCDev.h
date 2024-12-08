#ifndef _CDC_DEV_DESCRIPTORS_H_
#define _CDC_DEV_DESCRIPTORS_H_

#include <cstdint>
#include <cstring>

#include "tusb.h"

namespace CDCDesc 
{
    #define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
    #define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                              _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

    // #define USB_VID   0xCafe
    // #define USB_BCD   0x0200

    static const tusb_desc_device_t DEVICE_DESCRIPTORS =
    {
        .bLength            = sizeof(tusb_desc_device_t),
        .bDescriptorType    = TUSB_DESC_DEVICE,
        .bcdUSB             = 0x0200,

        .bDeviceClass       = TUSB_CLASS_MISC,
        .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol    = MISC_PROTOCOL_IAD,

        .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor           = 0xCafe,
        .idProduct          = USB_PID,
        .bcdDevice          = 0x0100,

        .iManufacturer      = 0x01,
        .iProduct           = 0x02,
        .iSerialNumber      = 0x03,

        .bNumConfigurations = 0x01
    };

    enum Itf
    {
        NUM_CDC = 0,
        NUM_CDC_DATA,
        NUM_TOTAL
    };

    #define EPNUM_CDC_NOTIF   0x81
    #define EPNUM_CDC_OUT     0x02
    #define EPNUM_CDC_IN      0x82

    static constexpr int CONFIG_LEN = (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN);

    static const uint8_t CONFIGURATION_DESCRIPTORS[] =
    {
        TUD_CONFIG_DESCRIPTOR(1, Itf::NUM_TOTAL, 0, CONFIG_LEN, 0x00, 500),
        TUD_CDC_DESCRIPTOR(Itf::NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
    };

    enum
    {
        STRID_LANGID = 0,
        STRID_MANUFACTURER,
        STRID_PRODUCT,
        STRID_SERIAL,
    };

    static const char *STRING_DESCRIPTORS[] =
    {
        (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
        "TinyUSB",                     // 1: Manufacturer
        "TinyUSB Device",              // 2: Product
        NULL,                          // 3: Serials will use unique ID if possible
        "TinyUSB CDC",                 // 4: CDC Interface
    };

    static uint16_t _desc_str[32 + 1];

}; // namespace CDCDesc

#endif // _CDC_DEV_DESCRIPTORS_H_