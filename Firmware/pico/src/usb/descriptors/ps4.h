#pragma once

#include <stdint.h>
#include "common/usb_util.h"
#include "common/usb_def.h"
#include "common/class/hid_def.h"
#include "assert_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint8_t PS4_DESC_DEVICE[] = {
    0x12,        // bLength
    0x01,        // bDescriptorType (Device)
    0x00, 0x02,  // bcdUSB 2.00
    0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
    0x00,        // bDeviceSubClass 
    0x00,        // bDeviceProtocol 
    0x40,        // bMaxPacketSize0 64
    0x4C, 0x05,  // idVendor 0x054C
    0xCC, 0x09,  // idProduct 0x09CC
    0x00, 0x01,  // bcdDevice 2.00
    0x01,        // iManufacturer (String Index)
    0x02,        // iProduct (String Index)
    0x00,        // iSerialNumber (String Index)
    0x01,        // bNumConfigurations 1
};

static const uint8_t PS4_DESC_CONFIG[] = {
    0x09,        // bLength
    0x02,        // bDescriptorType (Configuration)
    0xE1, 0x00,  // wTotalLength 225
    0x04,        // bNumInterfaces 4
    0x01,        // bConfigurationValue
    0x00,        // iConfiguration (String Index)
    0xC0,        // bmAttributes Self Powered
    0xFA,        // bMaxPower 500mA

    // ITF 0
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber 0
    0x00,        // bAlternateSetting
    0x00,        // bNumEndpoints 0
    0x01,        // bInterfaceClass (Audio)
    0x01,        // bInterfaceSubClass (Audio Control)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x0A,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
    0x00, 0x01,  // bcdADC 1.00
    0x47, 0x00,  // wTotalLength 71
    0x02,        // binCollection 0x02
    0x01,        // baInterfaceNr 1
    0x02,        // baInterfaceNr 2

    0x0C,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x02,        // bDescriptorSubtype (CS_INTERFACE -> INPUT_TERMINAL)
    0x01,        // bTerminalID
    0x01, 0x01,  // wTerminalType (USB Streaming)
    0x06,        // bAssocTerminal
    0x02,        // bNrChannels 2
    0x03, 0x00,  // wChannelConfig (Left and Right Front)
    0x00,        // iChannelNames
    0x00,        // iTerminal

    0x0A,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x06,        // bDescriptorSubtype (CS_INTERFACE -> FEATURE_UNIT)
    0x02,        // bUnitID
    0x01,        // bSourceID
    0x01,        // bControlSize 1
    0x03, 0x00,  // bmaControls[0] (Mute,Volume)
    0x00, 0x00,  // bmaControls[1] (None)

    0x09,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x03,        // bDescriptorSubtype (CS_INTERFACE -> OUTPUT_TERMINAL)
    0x03,        // bTerminalID
    0x02, 0x04,  // wTerminalType (Headset)
    0x04,        // bAssocTerminal
    0x02,        // bSourceID
    0x00,        // iTerminal

    0x0C,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x02,        // bDescriptorSubtype (CS_INTERFACE -> INPUT_TERMINAL)
    0x04,        // bTerminalID
    0x02, 0x04,  // wTerminalType (Headset)
    0x03,        // bAssocTerminal
    0x01,        // bNrChannels 1
    0x00, 0x00,  // wChannelConfig
    0x00,        // iChannelNames
    0x00,        // iTerminal

    0x09,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x06,        // bDescriptorSubtype (CS_INTERFACE -> FEATURE_UNIT)
    0x05,        // bUnitID
    0x04,        // bSourceID
    0x01,        // bControlSize 1
    0x03, 0x00,  // bmaControls[0] (Mute,Volume)
    0x00,        // iFeature

    0x09,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x03,        // bDescriptorSubtype (CS_INTERFACE -> OUTPUT_TERMINAL)
    0x06,        // bTerminalID
    0x01, 0x01,  // wTerminalType (USB Streaming)
    0x01,        // bAssocTerminal
    0x05,        // bSourceID
    0x00,        // iTerminal

    // ITF 1: Audio Streaming
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x01,        // bInterfaceNumber 1
    0x00,        // bAlternateSetting
    0x00,        // bNumEndpoints 0
    0x01,        // bInterfaceClass (Audio)
    0x02,        // bInterfaceSubClass (Audio Streaming)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x01,        // bInterfaceNumber 1
    0x01,        // bAlternateSetting
    0x01,        // bNumEndpoints 1
    0x01,        // bInterfaceClass (Audio)
    0x02,        // bInterfaceSubClass (Audio Streaming)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x07,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x01,        // bDescriptorSubtype (CS_INTERFACE -> AS_GENERAL)
    0x01,        // bTerminalLink
    0x01,        // bDelay 1
    0x01, 0x00,  // wFormatTag (PCM)

    0x0B,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x02,        // bDescriptorSubtype (CS_INTERFACE -> FORMAT_TYPE)
    0x01,        // bFormatType 1
    0x02,        // bNrChannels (Stereo)
    0x02,        // bSubFrameSize 2
    0x10,        // bBitResolution 16
    0x01,        // bSamFreqType 1
    0x00, 0x7D, 0x00,  // tSamFreq[1] 32000 Hz

    0x09,        // bLength
    0x05,        // bDescriptorType (See Next Line)
    0x01,        // bEndpointAddress (OUT/H2D)
    0x09,        // bmAttributes (Isochronous, Adaptive, Data EP)
    0x84, 0x00,  // wMaxPacketSize 132
    0x01,        // bInterval 1 (unit depends on device speed)
    0x00,        // bRefresh
    0x00,        // bSyncAddress

    0x07,        // bLength
    0x25,        // bDescriptorType (See Next Line)
    0x01,        // bDescriptorSubtype (CS_ENDPOINT -> EP_GENERAL)
    0x00,        // bmAttributes (None)
    0x00,        // bLockDelayUnits
    0x00, 0x00,  // wLockDelay 0

    // ITF 2: Audio Streaming
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x02,        // bInterfaceNumber 2
    0x00,        // bAlternateSetting
    0x00,        // bNumEndpoints 0
    0x01,        // bInterfaceClass (Audio)
    0x02,        // bInterfaceSubClass (Audio Streaming)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x02,        // bInterfaceNumber 2
    0x01,        // bAlternateSetting
    0x01,        // bNumEndpoints 1
    0x01,        // bInterfaceClass (Audio)
    0x02,        // bInterfaceSubClass (Audio Streaming)
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x07,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x01,        // bDescriptorSubtype (CS_INTERFACE -> AS_GENERAL)
    0x06,        // bTerminalLink
    0x01,        // bDelay 1
    0x01, 0x00,  // wFormatTag (PCM)

    0x0B,        // bLength
    0x24,        // bDescriptorType (See Next Line)
    0x02,        // bDescriptorSubtype (CS_INTERFACE -> FORMAT_TYPE)
    0x01,        // bFormatType 1
    0x01,        // bNrChannels (Mono)
    0x02,        // bSubFrameSize 2
    0x10,        // bBitResolution 16
    0x01,        // bSamFreqType 1
    0x80, 0x3E, 0x00,  // tSamFreq[1] 16000 Hz

    0x09,        // bLength
    0x05,        // bDescriptorType (See Next Line)
    0x82,        // bEndpointAddress (IN/D2H)
    0x05,        // bmAttributes (Isochronous, Async, Data EP)
    0x22, 0x00,  // wMaxPacketSize 34
    0x01,        // bInterval 1 (unit depends on device speed)
    0x00,        // bRefresh
    0x00,        // bSyncAddress

    0x07,        // bLength
    0x25,        // bDescriptorType (See Next Line)
    0x01,        // bDescriptorSubtype (CS_ENDPOINT -> EP_GENERAL)
    0x00,        // bmAttributes (None)
    0x00,        // bLockDelayUnits
    0x00, 0x00,  // wLockDelay 0

    // ITF 3: HID
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x03,        // bInterfaceNumber 3
    0x00,        // bAlternateSetting
    0x02,        // bNumEndpoints 2
    0x03,        // bInterfaceClass
    0x00,        // bInterfaceSubClass
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x09,        // bLength
    0x21,        // bDescriptorType (HID)
    0x11, 0x01,  // bcdHID 1.11
    0x00,        // bCountryCode
    0x01,        // bNumDescriptors
    0x22,        // bDescriptorType[0] (HID)
    0xFB, 0x01,  // wDescriptorLength[0] 507

    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x84,        // bEndpointAddress (IN/D2H)
    0x03,        // bmAttributes (Interrupt)
    0x40, 0x00,  // wMaxPacketSize 64
    0x05,        // bInterval 5 (unit depends on device speed)

    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x03,        // bEndpointAddress (OUT/H2D)
    0x03,        // bmAttributes (Interrupt)
    0x40, 0x00,  // wMaxPacketSize 64
    0x05,        // bInterval 5 (unit depends on device speed)
};

static const uint8_t PS4_DESC_REPORT[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0E,        //   Usage Maximum (0x0E)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0E,        //   Report Count (14)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x75, 0x06,        //   Report Size (6)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x7F,        //   Logical Maximum (127)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x21,        //   Usage (0x21)
    0x95, 0x36,        //   Report Count (54)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x05,        //   Report ID (5)
    0x09, 0x22,        //   Usage (0x22)
    0x95, 0x1F,        //   Report Count (31)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x04,        //   Report ID (4)
    0x09, 0x23,        //   Usage (0x23)
    0x95, 0x24,        //   Report Count (36)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x24,        //   Usage (0x24)
    0x95, 0x24,        //   Report Count (36)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x08,        //   Report ID (8)
    0x09, 0x25,        //   Usage (0x25)
    0x95, 0x03,        //   Report Count (3)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x10,        //   Report ID (16)
    0x09, 0x26,        //   Usage (0x26)
    0x95, 0x04,        //   Report Count (4)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x11,        //   Report ID (17)
    0x09, 0x27,        //   Usage (0x27)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x12,        //   Report ID (18)
    0x06, 0x02, 0xFF,  //   Usage Page (Vendor Defined 0xFF02)
    0x09, 0x21,        //   Usage (0x21)
    0x95, 0x0F,        //   Report Count (15)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x13,        //   Report ID (19)
    0x09, 0x22,        //   Usage (0x22)
    0x95, 0x16,        //   Report Count (22)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x14,        //   Report ID (20)
    0x06, 0x05, 0xFF,  //   Usage Page (Vendor Defined 0xFF05)
    0x09, 0x20,        //   Usage (0x20)
    0x95, 0x10,        //   Report Count (16)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x15,        //   Report ID (21)
    0x09, 0x21,        //   Usage (0x21)
    0x95, 0x2C,        //   Report Count (44)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x06, 0x80, 0xFF,  //   Usage Page (Vendor Defined 0xFF80)
    0x85, 0x80,        //   Report ID (-128)
    0x09, 0x20,        //   Usage (0x20)
    0x95, 0x06,        //   Report Count (6)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x81,        //   Report ID (-127)
    0x09, 0x21,        //   Usage (0x21)
    0x95, 0x06,        //   Report Count (6)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x82,        //   Report ID (-126)
    0x09, 0x22,        //   Usage (0x22)
    0x95, 0x05,        //   Report Count (5)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x83,        //   Report ID (-125)
    0x09, 0x23,        //   Usage (0x23)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x84,        //   Report ID (-124)
    0x09, 0x24,        //   Usage (0x24)
    0x95, 0x04,        //   Report Count (4)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x85,        //   Report ID (-123)
    0x09, 0x25,        //   Usage (0x25)
    0x95, 0x06,        //   Report Count (6)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x86,        //   Report ID (-122)
    0x09, 0x26,        //   Usage (0x26)
    0x95, 0x06,        //   Report Count (6)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x87,        //   Report ID (-121)
    0x09, 0x27,        //   Usage (0x27)
    0x95, 0x23,        //   Report Count (35)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x88,        //   Report ID (-120)
    0x09, 0x28,        //   Usage (0x28)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x89,        //   Report ID (-119)
    0x09, 0x29,        //   Usage (0x29)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x90,        //   Report ID (-112)
    0x09, 0x30,        //   Usage (0x30)
    0x95, 0x05,        //   Report Count (5)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x91,        //   Report ID (-111)
    0x09, 0x31,        //   Usage (0x31)
    0x95, 0x03,        //   Report Count (3)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x92,        //   Report ID (-110)
    0x09, 0x32,        //   Usage (0x32)
    0x95, 0x03,        //   Report Count (3)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x93,        //   Report ID (-109)
    0x09, 0x33,        //   Usage (0x33)
    0x95, 0x0C,        //   Report Count (12)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x94,        //   Report ID (-108)
    0x09, 0x34,        //   Usage (0x34)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA0,        //   Report ID (-96)
    0x09, 0x40,        //   Usage (0x40)
    0x95, 0x06,        //   Report Count (6)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA1,        //   Report ID (-95)
    0x09, 0x41,        //   Usage (0x41)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA2,        //   Report ID (-94)
    0x09, 0x42,        //   Usage (0x42)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA3,        //   Report ID (-93)
    0x09, 0x43,        //   Usage (0x43)
    0x95, 0x30,        //   Report Count (48)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA4,        //   Report ID (-92)
    0x09, 0x44,        //   Usage (0x44)
    0x95, 0x0D,        //   Report Count (13)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF0,        //   Report ID (-16)
    0x09, 0x47,        //   Usage (0x47)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF1,        //   Report ID (-15)
    0x09, 0x48,        //   Usage (0x48)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF2,        //   Report ID (-14)
    0x09, 0x49,        //   Usage (0x49)
    0x95, 0x0F,        //   Report Count (15)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA7,        //   Report ID (-89)
    0x09, 0x4A,        //   Usage (0x4A)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA8,        //   Report ID (-88)
    0x09, 0x4B,        //   Usage (0x4B)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xA9,        //   Report ID (-87)
    0x09, 0x4C,        //   Usage (0x4C)
    0x95, 0x08,        //   Report Count (8)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xAA,        //   Report ID (-86)
    0x09, 0x4E,        //   Usage (0x4E)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xAB,        //   Report ID (-85)
    0x09, 0x4F,        //   Usage (0x4F)
    0x95, 0x39,        //   Report Count (57)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xAC,        //   Report ID (-84)
    0x09, 0x50,        //   Usage (0x50)
    0x95, 0x39,        //   Report Count (57)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xAD,        //   Report ID (-83)
    0x09, 0x51,        //   Usage (0x51)
    0x95, 0x0B,        //   Report Count (11)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xAE,        //   Report ID (-82)
    0x09, 0x52,        //   Usage (0x52)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xAF,        //   Report ID (-81)
    0x09, 0x53,        //   Usage (0x53)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xB0,        //   Report ID (-80)
    0x09, 0x54,        //   Usage (0x54)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xE0,        //   Report ID (-32)
    0x09, 0x57,        //   Usage (0x57)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xB3,        //   Report ID (-77)
    0x09, 0x55,        //   Usage (0x55)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xB4,        //   Report ID (-76)
    0x09, 0x55,        //   Usage (0x55)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xB5,        //   Report ID (-75)
    0x09, 0x56,        //   Usage (0x56)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xD0,        //   Report ID (-48)
    0x09, 0x58,        //   Usage (0x58)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xD4,        //   Report ID (-44)
    0x09, 0x59,        //   Usage (0x59)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

#define PS4_REPORT_ID_OUT   ((uint8_t)0x05)
#define PS4_HAT_MASK        ((uint8_t)0x0F)

#define PS4_BUTTON0_SQUARE   ((uint8_t)0x10)
#define PS4_BUTTON0_CROSS    ((uint8_t)0x20)
#define PS4_BUTTON0_CIRCLE   ((uint8_t)0x40)
#define PS4_BUTTON0_TRIANGLE ((uint8_t)0x80)
#define PS4_BUTTON1_L1       ((uint8_t)0x01)
#define PS4_BUTTON1_R1       ((uint8_t)0x02)
#define PS4_BUTTON1_L2       ((uint8_t)0x04)
#define PS4_BUTTON1_R2       ((uint8_t)0x08)
#define PS4_BUTTON1_SHARE    ((uint8_t)0x10)
#define PS4_BUTTON1_OPTIONS  ((uint8_t)0x20)
#define PS4_BUTTON1_L3       ((uint8_t)0x40)
#define PS4_BUTTON1_R3       ((uint8_t)0x80)
#define PS4_BUTTON2_SYS      ((uint8_t)0x01)
#define PS4_BUTTON2_TP       ((uint8_t)0x02)

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;

    uint8_t joystick_lx;
    uint8_t joystick_ly;
    uint8_t joystick_rx;
    uint8_t joystick_ry;

    union {
        struct {
            uint8_t hat : 4; // 0-7 = direction, 8 = center, 9-15 = reserved
            uint8_t : 4;
        };
        uint8_t buttons0;
    };
    uint8_t buttons1;
    uint8_t buttons2;

    uint8_t trigger_l;
    uint8_t trigger_r;

    uint8_t unk[22];
} ps4_report_in_t;
_STATIC_ASSERT(sizeof(ps4_report_in_t) == 32, "ps4_report_in_t size mismatch");

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;

    union {
        struct {
            uint8_t rumble : 1;
            uint8_t led : 1;
            uint8_t led_blink : 1;
            uint8_t ext_write : 1;
            uint8_t left_volume : 1;
            uint8_t right_volume : 1;
            uint8_t mic_volume : 1;
            uint8_t speaker_volume : 1;
        };
        uint8_t raw;
    } flags;

    uint8_t flags2;

    uint8_t reserved;

    uint8_t motor_right;
    uint8_t motor_left;

    uint8_t lightbar_red;
    uint8_t lightbar_green;
    uint8_t lightbar_blue;
    uint8_t lightbar_blink_on;
    uint8_t lightbar_blink_off;

    uint8_t ext_data[8];

    uint8_t volume_left;
    uint8_t volume_right;
    uint8_t volume_mic;
    uint8_t volume_speaker;

    uint8_t unk[9];
} ps4_report_out_t;
_STATIC_ASSERT(sizeof(ps4_report_out_t) == 32, "ps4_report_out_t size mismatch");

typedef enum {
    PS4_LED_BLUE = 0,
    PS4_LED_RED,
    PS4_LED_GREEN,
    PS4_LED_PINK,
    PS4_LED_COUNT,
} ps4_led_color_t;

static const uint8_t PS4_LED_COLORS[PS4_LED_COUNT][3] = {
    { 0x00, 0x00, 0x40 },
    { 0x40, 0x00, 0x00 },
    { 0x00, 0x40, 0x00 },
    { 0x20, 0x00, 0x20 },
};

#ifdef __cplusplus
}
#endif