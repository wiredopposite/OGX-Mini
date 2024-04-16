#ifndef SWITCH_PRO_DESCRIPTORS_H_
#define SWITCH_PRO_DESCRIPTORS_H_

#include <stdint.h>

#define SWITCH_PRO_ENDPOINT_SIZE 64

// commands
#define CMD_HID 0x80
#define SUBCMD_HANDSHAKE 0x02
#define SUBCMD_DISABLE_TIMEOUT 0x04

// out report commands
#define CMD_RUMBLE_ONLY 0x10
#define CMD_AND_RUMBLE 0x01

// out report subcommands
#define CMD_LED 0x30
#define CMD_LED_HOME 0x38
#define CMD_GYRO 0x40
#define CMD_MODE 0x03
#define SUBCMD_FULL_REPORT_MODE 0x30

struct SwitchProReport
{
    uint8_t reportId;
    uint8_t timer;

    uint8_t connInfo : 4;
    uint8_t battery  : 4;

    uint8_t y  : 1;
    uint8_t x  : 1;
    uint8_t b  : 1;
    uint8_t a  : 1;
    uint8_t    : 2;
    uint8_t r  : 1;
    uint8_t zr : 1;

    uint8_t minus   : 1;
    uint8_t plus    : 1;
    uint8_t stickR  : 1;
    uint8_t stickL  : 1;
    uint8_t home    : 1;
    uint8_t capture : 1;
    uint8_t         : 0;

    uint8_t down  : 1;
    uint8_t up    : 1;
    uint8_t right : 1;
    uint8_t left  : 1;
    uint8_t       : 2;
    uint8_t l     : 1;
    uint8_t zl    : 1;

    uint16_t leftX  : 12;
    uint16_t leftY  : 12;
    uint16_t rightX : 12;
    uint16_t rightY : 12;

    uint8_t vibrator;

    uint16_t accelerX;
    uint16_t accelerY;
    uint16_t accelerZ;

    uint16_t velocityX;
    uint16_t velocityY;
    uint16_t velocityZ;
}
__attribute__((packed));

struct SwitchProOutReport
{
    uint8_t command;
    uint8_t sequence_counter;
    uint8_t rumble_l[4];
    uint8_t rumble_r[4];
    uint8_t sub_command;
    uint8_t sub_command_args[3];
}
__attribute__((packed));

// static const uint8_t switch_pro_string_language[]     = { 0x09, 0x04 };
// static const uint8_t switch_pro_string_manufacturer[] = "Nintnedo Co., Ltd.";
// static const uint8_t switch_pro_string_product[]      = "Pro Controller";
// static const uint8_t switch_pro_string_version[]      = "000000000001";

// static const uint8_t *switch_pro_string_descriptors[] __attribute__((unused)) =
// {
// 	switch_pro_string_language,
// 	switch_pro_string_product,      // switch these?
// 	switch_pro_string_manufacturer, // ?
// 	switch_pro_string_version
// };

// static const uint8_t switch_pro_device_descriptor[] =
// {
//     0x12,        // bLength
//     0x01,        // bDescriptorType (Device)
//     0x00, 0x02,  // bcdUSB 2.00
//     0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
//     0x00,        // bDeviceSubClass 
//     0x00,        // bDeviceProtocol 
//     0x40,        // bMaxPacketSize0 64
//     0x7E, 0x05,  // idVendor 0x057E
//     0x09, 0x20,  // idProduct 0x2009
//     0x00, 0x02,  // bcdDevice 4.00
//     0x01,        // iManufacturer (String Index)
//     0x02,        // iProduct (String Index)
//     0x03,        // iSerialNumber (String Index)
//     0x01,        // bNumConfigurations 1
// };

// static const uint8_t switch_pro_configuration_descriptor[] =
// {
//     0x09,        //   bLength
//     0x02,        //   bDescriptorType (Configuration)
//     0x29, 0x00,  //   wTotalLength 41
//     0x01,        //   bNumInterfaces 1
//     0x01,        //   bConfigurationValue
//     0x00,        //   iConfiguration (String Index)
//     0xA0,        //   bmAttributes Remote Wakeup
//     0xFA,        //   bMaxPower 500mA

//     0x09,        //   bLength
//     0x04,        //   bDescriptorType (Interface)
//     0x00,        //   bInterfaceNumber 0
//     0x00,        //   bAlternateSetting
//     0x02,        //   bNumEndpoints 2
//     0x03,        //   bInterfaceClass
//     0x00,        //   bInterfaceSubClass
//     0x00,        //   bInterfaceProtocol
//     0x00,        //   iInterface (String Index)

//     0x09,        //   bLength
//     0x21,        //   bDescriptorType (HID)
//     0x11, 0x01,  //   bcdHID 1.11
//     0x00,        //   bCountryCode
//     0x01,        //   bNumDescriptors
//     0x22,        //   bDescriptorType[0] (HID)
//     0xCB, 0x00,  //   wDescriptorLength[0] 203

//     0x07,        //   bLength
//     0x05,        //   bDescriptorType (Endpoint)
//     0x81,        //   bEndpointAddress (IN/D2H)
//     0x03,        //   bmAttributes (Interrupt)
//     0x40, 0x00,  //   wMaxPacketSize 64
//     0x08,        //   bInterval 8 (unit depends on device speed)

//     0x07,        //   bLength
//     0x05,        //   bDescriptorType (Endpoint)
//     0x01,        //   bEndpointAddress (OUT/H2D)
//     0x03,        //   bmAttributes (Interrupt)
//     0x40, 0x00,  //   wMaxPacketSize 64
//     0x08,        //   bInterval 8 (unit depends on device speed)
// };

// static const uint8_t switch_pro_report_descriptor[] =
// {
//     0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
//     0x15, 0x00,        // Logical Minimum (0)
//     0x09, 0x04,        // Usage (Joystick)
//     0xA1, 0x01,        // Collection (Application)
//     0x85, 0x30,        //   Report ID (48)
//     0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
//     0x05, 0x09,        //   Usage Page (Button)
//     0x19, 0x01,        //   Usage Minimum (0x01)
//     0x29, 0x0A,        //   Usage Maximum (0x0A)
//     0x15, 0x00,        //   Logical Minimum (0)
//     0x25, 0x01,        //   Logical Maximum (1)
//     0x75, 0x01,        //   Report Size (1)
//     0x95, 0x0A,        //   Report Count (10)
//     0x55, 0x00,        //   Unit Exponent (0)
//     0x65, 0x00,        //   Unit (None)
//     0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x05, 0x09,        //   Usage Page (Button)
//     0x19, 0x0B,        //   Usage Minimum (0x0B)
//     0x29, 0x0E,        //   Usage Maximum (0x0E)
//     0x15, 0x00,        //   Logical Minimum (0)
//     0x25, 0x01,        //   Logical Maximum (1)
//     0x75, 0x01,        //   Report Size (1)
//     0x95, 0x04,        //   Report Count (4)
//     0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x75, 0x01,        //   Report Size (1)
//     0x95, 0x02,        //   Report Count (2)
//     0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x0B, 0x01, 0x00, 0x01, 0x00,  //   Usage (0x010001)
//     0xA1, 0x00,        //   Collection (Physical)
//     0x0B, 0x30, 0x00, 0x01, 0x00,  //     Usage (0x010030)
//     0x0B, 0x31, 0x00, 0x01, 0x00,  //     Usage (0x010031)
//     0x0B, 0x32, 0x00, 0x01, 0x00,  //     Usage (0x010032)
//     0x0B, 0x35, 0x00, 0x01, 0x00,  //     Usage (0x010035)
//     0x15, 0x00,        //     Logical Minimum (0)
//     0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
//     0x75, 0x10,        //     Report Size (16)
//     0x95, 0x04,        //     Report Count (4)
//     0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0xC0,              //   End Collection
//     0x0B, 0x39, 0x00, 0x01, 0x00,  //   Usage (0x010039)
//     0x15, 0x00,        //   Logical Minimum (0)
//     0x25, 0x07,        //   Logical Maximum (7)
//     0x35, 0x00,        //   Physical Minimum (0)
//     0x46, 0x3B, 0x01,  //   Physical Maximum (315)
//     0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
//     0x75, 0x04,        //   Report Size (4)
//     0x95, 0x01,        //   Report Count (1)
//     0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x05, 0x09,        //   Usage Page (Button)
//     0x19, 0x0F,        //   Usage Minimum (0x0F)
//     0x29, 0x12,        //   Usage Maximum (0x12)
//     0x15, 0x00,        //   Logical Minimum (0)
//     0x25, 0x01,        //   Logical Maximum (1)
//     0x75, 0x01,        //   Report Size (1)
//     0x95, 0x04,        //   Report Count (4)
//     0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x75, 0x08,        //   Report Size (8)
//     0x95, 0x34,        //   Report Count (52)
//     0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
//     0x85, 0x21,        //   Report ID (33)
//     0x09, 0x01,        //   Usage (0x01)
//     0x75, 0x08,        //   Report Size (8)
//     0x95, 0x3F,        //   Report Count (63)
//     0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x85, 0x81,        //   Report ID (-127)
//     0x09, 0x02,        //   Usage (0x02)
//     0x75, 0x08,        //   Report Size (8)
//     0x95, 0x3F,        //   Report Count (63)
//     0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//     0x85, 0x01,        //   Report ID (1)
//     0x09, 0x03,        //   Usage (0x03)
//     0x75, 0x08,        //   Report Size (8)
//     0x95, 0x3F,        //   Report Count (63)
//     0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
//     0x85, 0x10,        //   Report ID (16)
//     0x09, 0x04,        //   Usage (0x04)
//     0x75, 0x08,        //   Report Size (8)
//     0x95, 0x3F,        //   Report Count (63)
//     0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
//     0x85, 0x80,        //   Report ID (-128)
//     0x09, 0x05,        //   Usage (0x05)
//     0x75, 0x08,        //   Report Size (8)
//     0x95, 0x3F,        //   Report Count (63)
//     0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
//     0x85, 0x82,        //   Report ID (-126)
//     0x09, 0x06,        //   Usage (0x06)
//     0x75, 0x08,        //   Report Size (8)
//     0x95, 0x3F,        //   Report Count (63)
//     0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
//     0xC0,              // End Collection
// };

#endif // SWITCH_PRO_DESCRIPTORS_H_