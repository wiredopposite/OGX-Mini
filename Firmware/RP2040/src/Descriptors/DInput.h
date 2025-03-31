#pragma once

#include <cstdint>
#include <cstring>

#include "tusb.h"

#include "Board/Config.h"

namespace DInput 
{
    static constexpr uint8_t DPAD_MASK = 0x0F;
    static constexpr uint8_t AXIS_MIN  = 0x00;
    static constexpr uint8_t AXIS_MID  = 0x80;
    static constexpr uint8_t AXIS_MAX  = 0xFF;

    namespace Buttons0 
    {
        static constexpr uint8_t SQUARE     = 0x01;
        static constexpr uint8_t CROSS      = 0x02;
        static constexpr uint8_t CIRCLE     = 0x04;
        static constexpr uint8_t TRIANGLE   = 0x08;
        static constexpr uint8_t L1         = 0x10;
        static constexpr uint8_t R1         = 0x20;
        static constexpr uint8_t L2         = 0x40;
        static constexpr uint8_t R2         = 0x80;
    };

    namespace Buttons1 
    {
        static constexpr uint8_t SELECT = 0x01;
        static constexpr uint8_t START  = 0x02;
        static constexpr uint8_t L3     = 0x04;
        static constexpr uint8_t R3     = 0x08;
        static constexpr uint8_t SYS    = 0x10;
        static constexpr uint8_t TP     = 0x20;
    };

    namespace DPad 
    {
        static constexpr uint8_t UP         = 0x00;
        static constexpr uint8_t UP_RIGHT   = 0x01;
        static constexpr uint8_t RIGHT      = 0x02;
        static constexpr uint8_t DOWN_RIGHT = 0x03;
        static constexpr uint8_t DOWN       = 0x04;
        static constexpr uint8_t DOWN_LEFT  = 0x05;
        static constexpr uint8_t LEFT       = 0x06;
        static constexpr uint8_t UP_LEFT    = 0x07;
        static constexpr uint8_t CENTER     = 0x08;
    };

    #pragma pack(push, 1)
    struct InReport
    {
        uint8_t buttons[2];
        uint8_t dpad;

        uint8_t joystick_lx;
        uint8_t joystick_ly;
        uint8_t joystick_rx;
        uint8_t joystick_ry;

        uint8_t right_axis;
        uint8_t left_axis;
        uint8_t up_axis;
        uint8_t down_axis;

        uint8_t triangle_axis;
        uint8_t circle_axis;
        uint8_t cross_axis;
        uint8_t square_axis;

        uint8_t l1_axis;
        uint8_t r1_axis;
        uint8_t l2_axis;
        uint8_t r2_axis;

        InReport()
        {
            std::memset(this, 0, sizeof(InReport));
        }
    };
    #pragma pack(pop)
    static_assert(sizeof(InReport) == 19, "DInput::InReport is misaligned");

    static const uint8_t STRING_DESC_LANGUAGE[] = { 0x09, 0x04 };
    static const uint8_t STRING_MANUFACTURER[]  = "SHANWAN";
    static const uint8_t STRING_PRODUCT[]       = "2In1 USB Joystick";
    static const uint8_t STRING_VERSION[]       = "1.0";

    static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) =
    {
        STRING_DESC_LANGUAGE,
        STRING_MANUFACTURER,
        STRING_PRODUCT,
        STRING_VERSION
    };

    static const uint8_t DEVICE_DESCRIPTORS[] =
    {
        0x12,        // bLength
        0x01,        // bDescriptorType (Device)
        0x10, 0x01,  // bcdUSB 1.10
        0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
        0x00,        // bDeviceSubClass 
        0x00,        // bDeviceProtocol 
        0x40,        // bMaxPacketSize0 64
        0x63, 0x25,  // idVendor 0x2563
        0x75, 0x05,  // idProduct 0x0575
        0x00, 0x02,  // bcdDevice 4.00
        0x01,        // iManufacturer (String Index)
        0x02,        // iProduct (String Index)
        0x00,        // iSerialNumber (String Index)
        0x01,        // bNumConfigurations 1
    };

    static const uint8_t REPORT_DESCRIPTORS[] =
    {
        0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
        0x09, 0x05,        // Usage (Game Pad)
        0xA1, 0x01,        // Collection (Application)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x01,        //   Logical Maximum (1)
        0x35, 0x00,        //   Physical Minimum (0)
        0x45, 0x01,        //   Physical Maximum (1)
        0x75, 0x01,        //   Report Size (1)
        0x95, 0x0D,        //   Report Count (13)
        0x05, 0x09,        //   Usage Page (Button)
        0x19, 0x01,        //   Usage Minimum (0x01)
        0x29, 0x0D,        //   Usage Maximum (0x0D)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x95, 0x03,        //   Report Count (3)
        0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
        0x25, 0x07,        //   Logical Maximum (7)
        0x46, 0x3B, 0x01,  //   Physical Maximum (315)
        0x75, 0x04,        //   Report Size (4)
        0x95, 0x01,        //   Report Count (1)
        0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
        0x09, 0x39,        //   Usage (Hat switch)
        0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
        0x65, 0x00,        //   Unit (None)
        0x95, 0x01,        //   Report Count (1)
        0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x46, 0xFF, 0x00,  //   Physical Maximum (255)
        0x09, 0x30,        //   Usage (X)
        0x09, 0x31,        //   Usage (Y)
        0x09, 0x32,        //   Usage (Z)
        0x09, 0x35,        //   Usage (Rz)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x04,        //   Report Count (4)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
        0x09, 0x20,        //   Usage (0x20)
        0x09, 0x21,        //   Usage (0x21)
        0x09, 0x22,        //   Usage (0x22)
        0x09, 0x23,        //   Usage (0x23)
        0x09, 0x24,        //   Usage (0x24)
        0x09, 0x25,        //   Usage (0x25)
        0x09, 0x26,        //   Usage (0x26)
        0x09, 0x27,        //   Usage (0x27)
        0x09, 0x28,        //   Usage (0x28)
        0x09, 0x29,        //   Usage (0x29)
        0x09, 0x2A,        //   Usage (0x2A)
        0x09, 0x2B,        //   Usage (0x2B)
        0x95, 0x0C,        //   Report Count (12)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x0A, 0x21, 0x26,  //   Usage (0x2621)
        0x95, 0x08,        //   Report Count (8)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x0A, 0x21, 0x26,  //   Usage (0x2621)
        0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
        0x46, 0xFF, 0x03,  //   Physical Maximum (1023)
        0x09, 0x2C,        //   Usage (0x2C)
        0x09, 0x2D,        //   Usage (0x2D)
        0x09, 0x2E,        //   Usage (0x2E)
        0x09, 0x2F,        //   Usage (0x2F)
        0x75, 0x10,        //   Report Size (16)
        0x95, 0x04,        //   Report Count (4)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0xC0,              // End Collection
    };

    // uint8_t const CONFIGURATION_DESCRIPTORS[] =
    // {
    //     0x09,        // bLength
    //     0x02,        // bDescriptorType (Configuration)
    //     0x29, 0x00,  // wTotalLength 41
    //     0x01,        // bNumInterfaces 1
    //     0x01,        // bConfigurationValue
    //     0x00,        // iConfiguration (String Index)
    //     0x80,        // bmAttributes
    //     0xFA,        // bMaxPower 500mA

    //     0x09,        // bLength
    //     0x04,        // bDescriptorType (Interface)
    //     0x00,        // bInterfaceNumber 0
    //     0x00,        // bAlternateSetting
    //     0x02,        // bNumEndpoints 2
    //     0x03,        // bInterfaceClass
    //     0x00,        // bInterfaceSubClass
    //     0x00,        // bInterfaceProtocol
    //     0x00,        // iInterface (String Index)

    //     0x09,        // bLength
    //     0x21,        // bDescriptorType (HID)
    //     0x10, 0x01,  // bcdHID 1.10
    //     0x00,        // bCountryCode
    //     0x01,        // bNumDescriptors
    //     0x22,        // bDescriptorType[0] (HID)
    //     0x89, 0x00,  // wDescriptorLength[0] 137

    //     0x07,        // bLength
    //     0x05,        // bDescriptorType (Endpoint)
    //     0x02,        // bEndpointAddress (OUT/H2D)
    //     0x03,        // bmAttributes (Interrupt)
    //     0x20, 0x00,  // wMaxPacketSize 32
    //     0x0A,        // bInterval 10 (unit depends on device speed)

    //     0x07,        // bLength
    //     0x05,        // bDescriptorType (Endpoint)
    //     0x81,        // bEndpointAddress (IN/D2H)
    //     0x03,        // bmAttributes (Interrupt)
    //     0x20, 0x00,  // wMaxPacketSize 32
    //     0x0A,        // bInterval 10 (unit depends on device speed)
    // };

    enum Itf { 
        NUM_HID1 = 0, 
#if MAX_GAMEPADS > 1
        NUM_HID2,
#endif
#if MAX_GAMEPADS > 2 
        NUM_HID3,
#endif
#if MAX_GAMEPADS > 3 
        NUM_HID4,
#endif 
        NUM_TOTAL 
    };

    #define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + (TUD_HID_DESC_LEN * MAX_GAMEPADS))

    // #define EPNUM_HID1 0x81
    // #define EPNUM_HID2 0x82
    // #define EPNUM_HID3 0x83
    // #define EPNUM_HID4 0x84

    uint8_t const CONFIGURATION_DESCRIPTORS[] = 
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(  1,
                                Itf::NUM_TOTAL,
                                0,
                                CONFIG_TOTAL_LEN,
                                TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                                500),

        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_DESCRIPTOR( Itf::NUM_HID1,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            (0x80 | (Itf::NUM_HID1 + 1)),
                            CFG_TUD_HID_EP_BUFSIZE,
                            1),
#if MAX_GAMEPADS > 1
        TUD_HID_DESCRIPTOR( Itf::NUM_HID2,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            (0x80 | (Itf::NUM_HID2 + 1)),
                            CFG_TUD_HID_EP_BUFSIZE,
                            1),
#endif
#if MAX_GAMEPADS > 2
        TUD_HID_DESCRIPTOR( Itf::NUM_HID3,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            (0x80 | (Itf::NUM_HID3 + 1)),
                            CFG_TUD_HID_EP_BUFSIZE,
                            1),
#endif
#if MAX_GAMEPADS > 3
        TUD_HID_DESCRIPTOR( Itf::NUM_HID4,
                            0,
                            HID_ITF_PROTOCOL_NONE,
                            sizeof(REPORT_DESCRIPTORS),
                            (0x80 | (Itf::NUM_HID4 + 1)),
                            CFG_TUD_HID_EP_BUFSIZE,
                            1)
#endif
    };

}; // namespace DInput