#ifndef _XBOX_ONE_DESCRIPTORS_H_
#define _XBOX_ONE_DESCRIPTORS_H_

#include <cstdint>

namespace XboxOne
{
    namespace Buttons0
    {
        static constexpr uint8_t SYNC = (1 << 0);
        static constexpr uint8_t GUIDE = (1 << 1);
        static constexpr uint8_t START = (1 << 2);
        static constexpr uint8_t BACK = (1 << 3);
        static constexpr uint8_t A = (1 << 4);
        static constexpr uint8_t B = (1 << 5);
        static constexpr uint8_t X = (1 << 6);
        static constexpr uint8_t Y = (1 << 7);
    };
    namespace Buttons1
    {
        static constexpr uint8_t DPAD_UP = (1 << 0);
        static constexpr uint8_t DPAD_DOWN = (1 << 1);
        static constexpr uint8_t DPAD_LEFT = (1 << 2);
        static constexpr uint8_t DPAD_RIGHT = (1 << 3);
        static constexpr uint8_t LB = (1 << 4);
        static constexpr uint8_t RB = (1 << 5);
        static constexpr uint8_t L3 = (1 << 6);
        static constexpr uint8_t R3 = (1 << 7);
    };

    // GIP Command IDs
    static constexpr uint8_t GIP_CMD_DEVICE_ARRIVAL = 0x02;
    static constexpr uint8_t GIP_CMD_DEVICE_STATUS = 0x03;
    static constexpr uint8_t GIP_CMD_DEVICE_DESCRIPTOR = 0x04;
    static constexpr uint8_t GIP_CMD_VIRTUAL_KEY = 0x07;   // Guide/Home; payload byte 4: 01=pressed, 00=released
    static constexpr uint8_t GIP_CMD_SET_DEVICE_STATE = 0x09;
    static constexpr uint8_t GIP_CMD_INPUT = 0x20;
    static constexpr uint8_t GIP_CMD_SYSTEM_FOCUS_CHANGE = 0xE0;

    // GIP Header Flags
    namespace GipFlags
    {
        static constexpr uint8_t NONE = 0x00;
        static constexpr uint8_t INTERNAL = 0x20; // Core protocol message (request from host)
        static constexpr uint8_t RESPONSE_FIRST = 0xF0; // First chunk of response (from device)
        static constexpr uint8_t RESPONSE_CONTINUE = 0xA0; // Continuation chunk of response (from device)
    }

    // GIP Battery Types
    namespace BatteryType
    {
        static constexpr uint8_t WIRED = 0;
        static constexpr uint8_t STANDARD = 1;
        static constexpr uint8_t CHARGE_KIT = 2;
    }

    // GIP Battery Levels
    namespace BatteryLevel
    {
        static constexpr uint8_t LOW = 0;
        static constexpr uint8_t MEDIUM = 1;
        static constexpr uint8_t HIGH = 2;
        static constexpr uint8_t FULL = 3;
    }

    // GIP Interface GUIDs (for descriptor)
    static constexpr uint8_t GUID_GAMEPAD[16] = {
        0x08, 0x2E, 0x40, 0x2C, 0x07, 0xDF, 0x45, 0xE1,
        0xA5, 0xAB, 0xA3, 0x12, 0x7A, 0xF1, 0x97, 0xB5
    };

    #pragma pack(push, 1)
    
    // GIP Message Header (8 bytes for USB GIP)
    struct GipMessageHeader
    {
        uint8_t command;
        uint8_t client : 4;
        uint8_t needsAck : 1;
        uint8_t internal : 1;
        uint8_t chunkStart : 1;
        uint8_t chunked : 1;
        uint8_t sequence;
        uint8_t length;
    };
    static_assert(sizeof(GipMessageHeader) == 4, "GipMessageHeader must be 4 bytes");

    // Device Arrival Message (GIP CMD 0x02)
    struct DeviceArrivalMessage
    {
        GipMessageHeader header;
        uint64_t deviceId;      // Unique device identifier
        uint16_t vendorId;      // 0x045E
        uint16_t productId;     // 0x02D1
        struct {
            uint16_t major;
            uint16_t minor;
            uint16_t build;
            uint16_t revision;
        } firmwareVersion;
        uint8_t unknown[8];     // Padding to make total 32 bytes
    };
    static_assert(sizeof(DeviceArrivalMessage) == 32, "DeviceArrivalMessage must be 32 bytes");

    // Device Status Message (GIP CMD 0x03)
    struct DeviceStatusMessage
    {
        GipMessageHeader header;
        uint8_t batteryLevel : 2;
        uint8_t batteryType : 2;
        uint8_t unknown : 3;
        uint8_t connected : 1;
        uint8_t unknown2[3];
    };
    static_assert(sizeof(DeviceStatusMessage) == 8, "DeviceStatusMessage must be 8 bytes");

    // Device Descriptor Message (GIP CMD 0x04)
    // Windows requests this to identify the device type (gamepad, chatpad, etc.)
    // According to GIP spec, this is a variable-size message (typically 200-500 bytes)
    // It contains a descriptor header with offsets to various metadata blocks
    // NOTE: Vendor/product IDs are NOT in the descriptor header - they're only in Device Arrival message
    struct DeviceDescriptorHeader
    {
        uint16_t unknown1;         // 0x0293 (real controller value)
        uint16_t unknown2;         // 0x0010 (real controller value)
        uint16_t unknown3;         // 0x0001 (real controller value)
        uint8_t padding[8];         // 8 bytes of zeros
        uint16_t offset_externalCommands;
        uint16_t offset_firmwareVersions;
        uint16_t offset_audioFormats;
        uint16_t offset_inputCommands;
        uint16_t offset_outputCommands;
        uint16_t offset_classNames;
        uint16_t offset_interfaceGuids;
        uint16_t offset_hidDescriptor;
        uint16_t unknown4;          // 0x004C (real controller value)
    };
    // Note: Size may be 32 or 36 bytes depending on compiler alignment
    // We'll use sizeof() in the code rather than hardcoding
    
    // Minimal Device Descriptor Message structure
    // We'll build this dynamically to include at least the interface GUID
    struct DeviceDescriptorMessage
    {
        GipMessageHeader header;   // 4 bytes
        DeviceDescriptorHeader descHeader;  // 32 bytes
        // Following buffer will contain metadata blocks
        // For a minimal gamepad descriptor, we need at least:
        // - Interface GUID block (16 bytes + 1 byte count = 17 bytes minimum)
        uint8_t buffer[64];         // Buffer for metadata (can be expanded)
    };

    // 48-byte input report (4-byte header + 44-byte payload). Header: 0x20, 0x00, sequence, 0x2C.
    // We put gamepad data at payload offset 0 (report bytes 4-17) for Linux xpad/GamepadTester.
    // Real controller uses payload offset 6 for buttons; both formats are 48 bytes total.
    static constexpr uint16_t GIP_INPUT_REPORT_SIZE = 48;
    static constexpr uint8_t GIP_INPUT_PAYLOAD_SIZE = 44;  // 0x2C

    // Input Report (GIP CMD 0x20) - logical gamepad state (we build 48-byte wire format from this)
    struct InReport
    {
        GipMessageHeader header;

        uint16_t buttons;        // Bitfield: Sync, Menu, View, A, B, X, Y, Dpad, Shoulders, Thumbs
        uint16_t trigger_l;      // 0x000 to 0x3FF (0-1023)
        uint16_t trigger_r;      // 0x000 to 0x3FF (0-1023)
        int16_t joystick_lx;
        int16_t joystick_ly;
        int16_t joystick_rx;
        int16_t joystick_ry;
        uint8_t guide_pressed;   // Home/Guide: when set, report bytes 12-13 = 0xF5FE (real controller encoding)
    };
    static_assert(sizeof(InReport) == 19, "XboxOne::InReport must be 19 bytes");

    // Wire format: GIP 0x20 payload bytes 0-1 (report bytes 4-5) use Microsoft layout:
    // B0=Sync, B1=reserved, B2=Start, B3=Back, B4=A, B5=B, B6=X, B7=Y,
    // B8=DPAD_UP, B9=DPAD_DOWN, B10=DPAD_LEFT, B11=DPAD_RIGHT, B12=LB, B13=RB, B14=L3, B15=R3.
    namespace GipWireButtons
    {
        static constexpr uint16_t SYNC = (1U << 0);
        static constexpr uint16_t START = (1U << 2);
        static constexpr uint16_t BACK = (1U << 3);
        static constexpr uint16_t A = (1U << 4);
        static constexpr uint16_t B = (1U << 5);
        static constexpr uint16_t X = (1U << 6);
        static constexpr uint16_t Y = (1U << 7);
        static constexpr uint16_t DPAD_UP = (1U << 8);
        static constexpr uint16_t DPAD_DOWN = (1U << 9);
        static constexpr uint16_t DPAD_LEFT = (1U << 10);
        static constexpr uint16_t DPAD_RIGHT = (1U << 11);
        static constexpr uint16_t LEFT_SHOULDER = (1U << 12);
        static constexpr uint16_t RIGHT_SHOULDER = (1U << 13);
        static constexpr uint16_t LEFT_THUMB = (1U << 14);
        static constexpr uint16_t RIGHT_THUMB = (1U << 15);
    }

    // Logical Gamepad bitfield (evdev-style); used after mapping from GipWireButtons.
    namespace GamepadButtons
    {
        static constexpr uint16_t A = 0x0001;              // B0
        static constexpr uint16_t B = 0x0002;              // B1
        static constexpr uint16_t X = 0x0004;              // B2
        static constexpr uint16_t Y = 0x0008;              // B3
        static constexpr uint16_t LEFT_SHOULDER = 0x0010;   // B4 LB
        static constexpr uint16_t RIGHT_SHOULDER = 0x0020;  // B5 RB
        static constexpr uint16_t LEFT_TRIGGER_BTN = 0x0040;   // B6 LT (from trigger value)
        static constexpr uint16_t RIGHT_TRIGGER_BTN = 0x0080;  // B7 RT
        static constexpr uint16_t VIEW = 0x0100;           // B8 Select/Back
        static constexpr uint16_t MENU = 0x0200;            // B9 Start
        static constexpr uint16_t LEFT_THUMB = 0x0400;      // B10 L3
        static constexpr uint16_t RIGHT_THUMB = 0x0800;    // B11 R3
        static constexpr uint16_t DPAD_UP = 0x1000;        // B12
        static constexpr uint16_t DPAD_DOWN = 0x2000;      // B13
        static constexpr uint16_t DPAD_LEFT = 0x4000;      // B14
        static constexpr uint16_t DPAD_RIGHT = 0x8000;     // B15
        // B16 = Home/Guide (evdev often as separate key; we can't fit in 16 bits here)
        static constexpr uint16_t SYNC = 0x0000;           // unused in 16-bit; map to KEY_PROG1 etc. if needed
    }

    // Set Device State / Force Feedback (GIP CMD 0x09)
    struct OutReport
    {
        GipMessageHeader header;
        uint8_t unknown1;
        uint8_t flags;          // Force feedback flags
        uint8_t leftTrigger;    // Left trigger vibration (0-255)
        uint8_t rightTrigger;   // Right trigger vibration (0-255)
        uint8_t leftMotor;      // Left motor (0-255)
        uint8_t rightMotor;     // Right motor (0-255)
        uint8_t duration;      // Duration in 10ms units
        uint8_t delay;          // Delay before starting
        uint8_t repeat;         // Repeat count
    };
    static_assert(sizeof(OutReport) == 13, "XboxOne::OutReport must be 13 bytes");

    // Force Feedback Flags
    namespace ForceFeedbackFlags
    {
        static constexpr uint8_t RIGHT_MOTOR = 0x01;
        static constexpr uint8_t LEFT_MOTOR = 0x02;
        static constexpr uint8_t RIGHT_TRIGGER = 0x04;
        static constexpr uint8_t LEFT_TRIGGER = 0x08;
    }
    #pragma pack(pop)

    // USB Descriptors
    static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
    static const uint8_t STRING_MANUFACTURER[] = "Microsoft";
    static const uint8_t STRING_PRODUCT[]      = "Xbox One Controller";
    static const uint8_t STRING_SERIAL[]       = "000000000000";
    static const uint8_t STRING_VERSION[]      = "1.0";

    static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) =
    {
        STRING_LANGUAGE,
        STRING_MANUFACTURER,
        STRING_PRODUCT,
        STRING_SERIAL,
        STRING_VERSION
    };

    // Device descriptor - Windows loads xb1usb for VID_045E&PID_02D1 (Xbox One Controller)
    static const uint8_t DEVICE_DESCRIPTORS[] =
    {
        0x12,        // bLength
        0x01,        // bDescriptorType (Device)
        0x00, 0x02,  // bcdUSB 2.00
        0xFF,        // bDeviceClass (Vendor Specific)
        0x47,        // bDeviceSubClass (71)
        0xD0,        // bDeviceProtocol (208)
        0x40,        // bMaxPacketSize0 64
        0x5E, 0x04,  // idVendor 0x045E (Microsoft)
        0xD1, 0x02,  // idProduct 0x02D1 (Xbox One Controller - Windows compatible)
        0x16, 0x05,  // bcdDevice 5.16 (match capture)
        0x01,        // iManufacturer (String Index)
        0x02,        // iProduct (String Index)
        0x03,        // iSerialNumber (String Index)
        0x01,        // bNumConfigurations 1
    };

    // Configuration descriptor - match real controller: Interface 0 has BOTH GIP endpoints (0x02 OUT, 0x82 IN)
    static const uint8_t CONFIGURATION_DESCRIPTORS[] =
    {
        // Configuration descriptor
        0x09,        // bLength
        0x02,        // bDescriptorType (Configuration)
        0x32, 0x00,  // wTotalLength 50 bytes
        0x03,        // bNumInterfaces 3
        0x01,        // bConfigurationValue 1
        0x00,        // iConfiguration (String Index)
        0xA0,        // bmAttributes (Bus Powered, Remote Wakeup supported)
        0xFA,        // bMaxPower 500mA

        // Interface 0 - GIP data (both IN and OUT on this interface, like real controller)
        0x09,        // bLength
        0x04,        // bDescriptorType (Interface)
        0x00,        // bInterfaceNumber 0
        0x00,        // bAlternateSetting 0
        0x02,        // bNumEndpoints 2
        0xFF,        // bInterfaceClass (Vendor Specific)
        0x47,        // bInterfaceSubClass (71)
        0xD0,        // bInterfaceProtocol (208)
        0x00,        // iInterface (String Index)

        // Endpoint 0x02 OUT (first EP in descriptor = 0x02)
        0x07,        // bLength
        0x05,        // bDescriptorType (Endpoint)
        0x02,        // bEndpointAddress (OUT)
        0x03,        // bmAttributes (Interrupt)
        0x40, 0x00,  // wMaxPacketSize 64
        0x04,        // bInterval 4

        // Endpoint 0x82 IN (second EP = 0x82)
        0x07,        // bLength
        0x05,        // bDescriptorType (Endpoint)
        0x82,        // bEndpointAddress (IN)
        0x03,        // bmAttributes (Interrupt)
        0x40, 0x00,  // wMaxPacketSize 64
        0x04,        // bInterval 4

        // Interface 1 - no endpoints (placeholder for 3-interface layout)
        0x09,        // bLength
        0x04,        // bDescriptorType (Interface)
        0x01,        // bInterfaceNumber 1
        0x00,        // bAlternateSetting 0
        0x00,        // bNumEndpoints 0
        0xFF,        // bInterfaceClass (Vendor Specific)
        0x47,        // bInterfaceSubClass (71)
        0xD0,        // bInterfaceProtocol (208)
        0x00,        // iInterface (String Index)

        // Interface 2 - no endpoints (placeholder for 3-interface layout)
        0x09,        // bLength
        0x04,        // bDescriptorType (Interface)
        0x02,        // bInterfaceNumber 2
        0x00,        // bAlternateSetting 0
        0x00,        // bNumEndpoints 0
        0xFF,        // bInterfaceClass (Vendor Specific)
        0x47,        // bInterfaceSubClass (71)
        0xD0,        // bInterfaceProtocol (208)
        0x00,        // iInterface (String Index)
    };

    static constexpr uint16_t DRIVER_LEN = sizeof(CONFIGURATION_DESCRIPTORS);

}; // namespace XboxOne

#endif // _XBOX_ONE_DESCRIPTORS_H_