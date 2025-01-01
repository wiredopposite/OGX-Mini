#ifndef _XBOX_OG_DESCRIPTORS_H_
#define _XBOX_OG_DESCRIPTORS_H_

#include <cstdint>

#include "tusb.h"

namespace XboxOG 
{
    //Control request constants

    static constexpr uint8_t GET_DESC_REQ_TYPE = 0xC1;
    static constexpr uint8_t GET_DESC_REQ = 0x06;
    static constexpr uint16_t GET_DESC_VALUE = 0x4200;
    static constexpr uint8_t GET_DESC_SUBTYPE_GP_DUKE = 0x01;
    static constexpr uint8_t GET_DESC_SUBTYPE_GP_S = 0x02;
    static constexpr uint8_t GET_DESC_SUBTYPE_WHEEL = 0x10;
    static constexpr uint8_t GET_DESC_SUBTYPE_STICK = 0x20;
    static constexpr uint8_t GET_DESC_SUBTYPE_LIGHTGUN = 0x50;

    static constexpr uint8_t GET_CAP_REQ_TYPE = 0xC1;
    static constexpr uint8_t GET_CAP_REQ = 0x01;
    static constexpr uint16_t GET_CAP_VALUE_IN = 0x0100;
    static constexpr uint16_t GET_CAP_VALUE_OUT = 0x0200;

    static constexpr uint8_t SET_REPORT_REQ_TYPE = 0x21;
    static constexpr uint8_t SET_REPORT_REQ = 0x09;
    static constexpr uint16_t SET_REPORT_VALUE = 0x0200;

    static constexpr uint8_t GET_REPORT_REQ_TYPE = 0xA1;
    static constexpr uint8_t GET_REPORT_REQ = 1;
    static constexpr uint16_t GET_REPORT_VALUE = 0x0100;

    namespace GP //Duke/S
    {
        namespace Buttons 
        {
            static constexpr uint8_t DPAD_UP = (1 << 0);
            static constexpr uint8_t DPAD_DOWN = (1 << 1);
            static constexpr uint8_t DPAD_LEFT = (1 << 2);
            static constexpr uint8_t DPAD_RIGHT = (1 << 3);
            static constexpr uint8_t START = (1 << 4);
            static constexpr uint8_t BACK = (1 << 5);
            static constexpr uint8_t L3 = (1 << 6);
            static constexpr uint8_t R3 = (1 << 7);
        };

        #pragma pack(push, 1)
        struct InReport
        {
            uint8_t reserved1;
            uint8_t report_len;
            uint8_t buttons;
            uint8_t reserved2;
            uint8_t a;
            uint8_t b;
            uint8_t x;
            uint8_t y;
            uint8_t black;
            uint8_t white;
            uint8_t trigger_l;
            uint8_t trigger_r;
            int16_t joystick_lx;
            int16_t joystick_ly;
            int16_t joystick_rx;
            int16_t joystick_ry;
        };
        static_assert(sizeof(InReport) == 20, "XboxOG::InReport is not the correct size");

        struct OutReport
        {
            uint8_t reserved;
            uint8_t report_len;
            uint16_t rumble_l;
            uint16_t rumble_r;
        };
        static_assert(sizeof(OutReport) == 6, "XboxOG::OutReport is not the correct size");
        #pragma pack(pop)

        static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
        static const uint8_t STRING_MANUFACTURER[] = "";
        static const uint8_t STRING_PRODUCT[]      = "";
        static const uint8_t STRING_VERSION[]      = "1.0";

        static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) =
        {
            STRING_LANGUAGE,
            STRING_MANUFACTURER,
            STRING_PRODUCT,
            STRING_VERSION
        };

        static const tusb_desc_device_t DEVICE_DESCRIPTORS =
        {
            .bLength = sizeof(tusb_desc_device_t),
            .bDescriptorType = TUSB_DESC_DEVICE,
            .bcdUSB = 0x0110,
            .bDeviceClass = 0x00,
            .bDeviceSubClass = 0x00,
            .bDeviceProtocol = 0x00,
            .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
            .idVendor = 0x045E,
            .idProduct = 0x0289,
            .bcdDevice = 0x0121,
            .iManufacturer = 0x00,
            .iProduct = 0x00,
            .iSerialNumber = 0x00,
            .bNumConfigurations = 0x01
        };

        static constexpr uint8_t INTERFACE_CLASS = 0x58;
        static constexpr uint8_t INTERFACE_SUBCLASS = 0x42;
        static constexpr uint16_t DRIVER_LEN = 9+7+7;
        static constexpr uint16_t CONFIG_DESC_TOTAL_LEN = DRIVER_LEN  + TUD_CONFIG_DESC_LEN;
        
        enum Interface
        {
            NUM_DUKE = 0,
            NUM_TOTAL
        };

        #define TUD_XID_DUKE_DESCRIPTOR(_itfnum, _epout, _epin) \
            /* Interface */\
            9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, INTERFACE_CLASS, INTERFACE_SUBCLASS, 0x00, 0x00,\
            /* Endpoint In */\
            7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4, \
            /* Endpoint Out */\
            7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4

        static const uint8_t CONFIGURATION_DESCRIPTORS[] =
        {
            // Config number, interface count, string index, total length, attribute, power in mA
            TUD_CONFIG_DESCRIPTOR(  1, 
                                    Interface::NUM_TOTAL, 
                                    0, 
                                    CONFIG_DESC_TOTAL_LEN, 
                                    TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 
                                    500),

            TUD_XID_DUKE_DESCRIPTOR(Interface::NUM_DUKE, 
                                    Interface::NUM_DUKE + 1, 
                                    0x80 | (Interface::NUM_DUKE + 1)),
        };

        static const uint8_t XID_DEVICE_DESCRIPTORS[] =
        {
            0x10,
            0x42,
            0x00, 0x01,
            0x01,
            0x02,
            0x14,
            0x06,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };

        static const uint8_t XID_CAPABILITIES_IN[] =
        {
            0x00,
            0x14,
            0xFF,
            0x00,
            0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF
        };

        static const uint8_t XID_CAPABILITIES_OUT[] =
        {
            0x00,
            0x06,
            0xFF, 0xFF, 0xFF, 0xFF
        };

    }; // GP

    namespace SB // Steel Battalion
    {
        static constexpr uint16_t AIMING_MID = 32768;
        static constexpr uint16_t BUTTONS2_TOGGLE_MID = 0xFFFC;

        namespace Buttons0
        {
            static constexpr uint16_t RIGHTJOYMAINWEAPON      = 0x0001;
            static constexpr uint16_t RIGHTJOYFIRE            = 0x0002;
            static constexpr uint16_t RIGHTJOYLOCKON          = 0x0004;
            static constexpr uint16_t EJECT                   = 0x0008;
            static constexpr uint16_t COCKPITHATCH            = 0x0010;
            static constexpr uint16_t IGNITION                = 0x0020;
            static constexpr uint16_t START                   = 0x0040;
            static constexpr uint16_t MULTIMONOPENCLOSE       = 0x0080;
            static constexpr uint16_t MULTIMONMAPZOOMINOUT    = 0x0100;
            static constexpr uint16_t MULTIMONMODESELECT      = 0x0200;
            static constexpr uint16_t MULTIMONSUBMONITOR      = 0x0400;
            static constexpr uint16_t MAINMONZOOMIN           = 0x0800;
            static constexpr uint16_t MAINMONZOOMOUT          = 0x1000;
            static constexpr uint16_t FUNCTIONFSS             = 0x2000;
            static constexpr uint16_t FUNCTIONMANIPULATOR     = 0x4000;
            static constexpr uint16_t FUNCTIONLINECOLORCHANGE = 0x8000;
        };

        namespace Buttons1
        {
            static constexpr uint16_t WASHING            = 0x0001;
            static constexpr uint16_t EXTINGUISHER       = 0x0002;
            static constexpr uint16_t CHAFF              = 0x0004;
            static constexpr uint16_t FUNCTIONTANKDETACH = 0x0008;
            static constexpr uint16_t FUNCTIONOVERRIDE   = 0x0010;
            static constexpr uint16_t FUNCTIONNIGHTSCOPE = 0x0020;
            static constexpr uint16_t FUNCTIONF1         = 0x0040;
            static constexpr uint16_t FUNCTIONF2         = 0x0080;
            static constexpr uint16_t FUNCTIONF3         = 0x0100;
            static constexpr uint16_t WEAPONCONMAIN      = 0x0200;
            static constexpr uint16_t WEAPONCONSUB       = 0x0400;
            static constexpr uint16_t WEAPONCONMAGAZINE  = 0x0800;
            static constexpr uint16_t COMM1              = 0x1000;
            static constexpr uint16_t COMM2              = 0x2000;
            static constexpr uint16_t COMM3              = 0x4000;
            static constexpr uint16_t COMM4              = 0x8000;
        };

        namespace Buttons2
        {
            static constexpr uint16_t COMM5                = 0x0001;
            static constexpr uint16_t LEFTJOYSIGHTCHANGE   = 0x0002;
            static constexpr uint16_t TOGGLEFILTERCONTROL  = 0x0004;
            static constexpr uint16_t TOGGLEOXYGENSUPPLY   = 0x0008;
            static constexpr uint16_t TOGGLEFUELFLOWRATE   = 0x0010;
            static constexpr uint16_t TOGGLEBUFFREMATERIAL = 0x0020;
            static constexpr uint16_t TOGGLEVTLOCATION     = 0x0040;
        };

        namespace Axis
        {
            static constexpr uint16_t AIMINGX = 0x0001;
            static constexpr uint16_t AIMINGY = 0x0002;
            static constexpr uint16_t LEVER   = 0x0004;
            static constexpr uint16_t SIGHTX  = 0x0008;
            static constexpr uint16_t SIGHTY  = 0x0010;
            static constexpr uint16_t LPEDAL  = 0x0020;
            static constexpr uint16_t MPEDAL  = 0x0040;
            static constexpr uint16_t RPEDAL  = 0x0080;
            static constexpr uint16_t TUNER   = 0x0100;
            static constexpr uint16_t GEAR    = 0x0200;
        };

        namespace Gear
        {
            static constexpr int8_t R = 7;
            static constexpr int8_t N = 8;
            static constexpr int8_t G1 = 9;
            static constexpr int8_t G2 = 10;
            static constexpr int8_t G3 = 11;
            static constexpr int8_t G4 = 12;
            static constexpr int8_t G5 = 13;
        };

        #pragma pack(push, 1)
        struct InReport
        {
            uint8_t zero;
            uint8_t bLength;
            uint16_t dButtons[3];
            uint16_t aimingX;       //0 to 2^16 left to right
            uint16_t aimingY;       //0 to 2^16 top to bottom
            int16_t rotationLever;
            int16_t sightChangeX;
            int16_t sightChangeY;
            uint16_t leftPedal;      //Sidestep, 0x0000 to 0xFF00
            uint16_t middlePedal;    //Brake, 0x0000 to 0xFF00
            uint16_t rightPedal;     //Acceleration, 0x0000 to oxFF00
            int8_t tunerDial;        //0-15 is from 9oclock, around clockwise
            int8_t gearLever;        //7-13 is gears R,1,2,3,4,5
        };
        static_assert(sizeof(InReport) == 26, "XboxOGSB::InReport is not the correct size");

        struct OutReport
        {
            uint8_t zero;
            uint8_t bLength;
            uint8_t CockpitHatch_EmergencyEject;
            uint8_t Start_Ignition;
            uint8_t MapZoomInOut_OpenClose;
            uint8_t SubMonitorModeSelect_ModeSelect;
            uint8_t MainMonitorZoomOut_MainMonitorZoomIn;
            uint8_t Manipulator_ForecastShootingSystem;
            uint8_t Washing_LineColorChange;
            uint8_t Chaff_Extinguisher;
            uint8_t Override_TankDetach;
            uint8_t F1_NightScope;
            uint8_t F3_F2;
            uint8_t SubWeaponControl_MainWeaponControl;
            uint8_t Comm1_MagazineChange;
            uint8_t Comm3_Comm2;
            uint8_t Comm5_Comm4;
            uint8_t GearR_;
            uint8_t Gear1_GearN;
            uint8_t Gear3_Gear2;
            uint8_t Gear5_Gear4;
            uint8_t dummy;
        };
        static_assert(sizeof(OutReport) == 22, "XboxOGSB::OutReport is not the correct size");
        #pragma pack(pop)

        static const uint8_t STRING_LANGUAGE[]     = { 0x09, 0x04 };
        static const uint8_t STRING_MANUFACTURER[] = "";
        static const uint8_t STRING_PRODUCT[]      = "";
        static const uint8_t STRING_VERSION[]      = "1.0";

        static const uint8_t *STRING_DESCRIPTORS[] __attribute__((unused)) =
        {
            STRING_LANGUAGE,
            STRING_MANUFACTURER,
            STRING_PRODUCT,
            STRING_VERSION
        };

        static constexpr uint8_t INTERFACE_CLASS = 0x58;
        static constexpr uint8_t INTERFACE_SUBCLASS = 0x42;
        static constexpr uint16_t DRIVER_LEN = 9+7+7;
        static constexpr uint16_t CONFIG_DESC_TOTAL_LEN = DRIVER_LEN + TUD_CONFIG_DESC_LEN;

        enum Interface
        {
            NUM_STEELBATTALION = 0,
            NUM_TOTAL
        };

        #define TUD_XID_SB_DESCRIPTOR(_itfnum, _epout, _epin) \
            /* Interface */\
            9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, INTERFACE_CLASS, INTERFACE_SUBCLASS, 0x00, 0x00,\
            /* Endpoint In */\
            7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4, \
            /* Endpoint Out */\
            7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4

        static uint8_t const CONFIGURATION_DESCRIPTORS[] =
        {
            TUD_CONFIG_DESCRIPTOR(  1, 
                                    Interface::NUM_TOTAL, 
                                    0, 
                                    CONFIG_DESC_TOTAL_LEN, 
                                    TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 
                                    500),
            TUD_XID_SB_DESCRIPTOR(  Interface::NUM_STEELBATTALION, 
                                    Interface::NUM_STEELBATTALION + 1, 
                                    0x80 | (Interface::NUM_STEELBATTALION + 1)),
        };

        static const uint8_t XID_DEVICE_DESCRIPTORS[] =
        {
            0x10,
            0x42,
            0x00, 0x01,
            0x80,
            0x01,
            0x1A,
            0x16,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };

        static const uint8_t XID_CAPABILITIES_IN[] =
        {
            0x00,
            0x1A,
            0xFF,
            0xFF,
            0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF
        };

        static const uint8_t XID_CAPABILITIES_OUT[] =
        {
            0x00,
            0x16,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF
        };

    }; // SB

    namespace XR //XRemote
    {
        #define XID_REMOTE_INTERFACE_CLASS 0x58
        #define XID_REMOTE_INTERFACE_SUBCLASS 0x42
        #define XID_XREMOTE_ROM_CLASS 0x59

        namespace ButtonCode
        {
            static constexpr uint16_t SELECT     = 0x0A0B;
            static constexpr uint16_t UP         = 0x0AA6;
            static constexpr uint16_t DOWN       = 0x0AA7;
            static constexpr uint16_t RIGHT      = 0x0AA8;
            static constexpr uint16_t LEFT       = 0x0AA9;
            static constexpr uint16_t INFO       = 0x0AC3;
            static constexpr uint16_t NINE       = 0x0AC6;
            static constexpr uint16_t EIGHT      = 0x0AC7;
            static constexpr uint16_t SEVEN      = 0x0AC8;
            static constexpr uint16_t SIX        = 0x0AC9;
            static constexpr uint16_t FIVE       = 0x0ACA;
            static constexpr uint16_t FOUR       = 0x0ACB;
            static constexpr uint16_t THREE      = 0x0ACC;
            static constexpr uint16_t TWO        = 0x0ACD;
            static constexpr uint16_t ONE        = 0x0ACE;
            static constexpr uint16_t ZERO       = 0x0ACF;
            static constexpr uint16_t DISPLAY    = 0x0AD5;
            static constexpr uint16_t BACK       = 0x0AD8;
            static constexpr uint16_t SKIP_MINUS = 0x0ADD;
            static constexpr uint16_t SKIP_PLUS  = 0x0ADF;
            static constexpr uint16_t STOP       = 0x0AE0;
            static constexpr uint16_t REVERSE    = 0x0AE2;
            static constexpr uint16_t FORWARD    = 0x0AE3;
            static constexpr uint16_t TITLE      = 0x0AE5;
            static constexpr uint16_t PAUSE      = 0x0AE6;
            static constexpr uint16_t PLAY       = 0x0AEA;
            static constexpr uint16_t MENU       = 0x0AF7;
        };

        #pragma pack(push, 1)
        struct InReport
        {
            uint8_t zero;
            uint8_t bLength;
            uint16_t buttonCode;
            uint16_t timeElapsed; // ms since last button press
        };
        static_assert(sizeof(InReport) == 6, "XboxOGXRemote::InReport is not the correct size");
        #pragma pack(pop)

        static constexpr uint16_t DRIVER_LEN = 9+7+9;

        static const uint8_t DEVICE_DESCRIPTORS[] =
        {
            0x12,        // bLength
            0x01,        // bDescriptorType (Device)
            0x10, 0x01,  // bcdUSB 1.10
            0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
            0x00,        // bDeviceSubClass 
            0x00,        // bDeviceProtocol 
            0x40,        // bMaxPacketSize0 64
            0x5E, 0x04,  // idVendor 0x045E
            0x84, 0x02,  // idProduct 0x0284
            0x30, 0x01,  // bcdDevice 2.30
            0x00,        // iManufacturer (String Index)
            0x00,        // iProduct (String Index)
            0x00,        // iSerialNumber (String Index)
            0x01,        // bNumConfigurations 1
        };

        static const uint8_t CONFIGURATION_DESCRIPTORS[] =
        {
            0x09,        // bLength
            0x02,        // bDescriptorType (Configuration)
            0x22, 0x00,  // wTotalLength 34
            0x02,        // bNumInterfaces 2
            0x01,        // bConfigurationValue
            0x00,        // iConfiguration (String Index)
            0x80,        // bmAttributes
            0xFA,        // bMaxPower 500mA

            0x09,        // bLength
            0x04,        // bDescriptorType (Interface)
            0x00,        // bInterfaceNumber 0
            0x00,        // bAlternateSetting
            0x01,        // bNumEndpoints 1
            0x58,        // bInterfaceClass
            0x42,        // bInterfaceSubClass
            0x00,        // bInterfaceProtocol
            0x00,        // iInterface (String Index)

            0x07,        // bLength
            0x05,        // bDescriptorType (Endpoint)
            0x81,        // bEndpointAddress (IN/D2H)
            0x03,        // bmAttributes (Interrupt)
            0x08, 0x00,  // wMaxPacketSize 8
            0x10,        // bInterval 16 (unit depends on device speed)

            0x09,        // bLength
            0x04,        // bDescriptorType (Interface)
            0x01,        // bInterfaceNumber 1
            0x00,        // bAlternateSetting
            0x00,        // bNumEndpoints 0
            0x59,        // bInterfaceClass
            0x00,        // bInterfaceSubClass
            0x00,        // bInterfaceProtocol
            0x00,        // iInterface (String Index)
        };

        static const uint8_t XID_DEVICE_DESCRIPTORS[] = 
        {
            0x08,
            0x42,
            0x00, 0x01,
            0x03,
            0x00,
            0x06,
            0x00
        };

    }; // XR

    namespace COM
    {
        static constexpr uint8_t INTERFACE_CLASS = 0x78;
        static constexpr uint8_t INTERFACE_SUBCLASS = 0x00;

        static const uint8_t DEVICE_DESCRIPTORS[] = 
        {
            0x12,        // bLength
            0x01,        // bDescriptorType (Device)
            0x10, 0x01,  // bcdUSB 1.10
            0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
            0x00,        // bDeviceSubClass 
            0x00,        // bDeviceProtocol 
            0x08,        // bMaxPacketSize0 8
            0x5E, 0x04,  // idVendor 0x045E
            0x83, 0x02,  // idProduct 0x0283
            0x58, 0x01,  // bcdDevice 2.58
            0x00,        // iManufacturer (String Index)
            0x00,        // iProduct (String Index)
            0x00,        // iSerialNumber (String Index)
            0x01,        // bNumConfigurations 1
        };

        static const uint8_t CONFIGURATION_DESCRIPTORS[]
        {
            0x09,        // bLength
            0x02,        // bDescriptorType (Configuration)
            0x2D, 0x00,  // wTotalLength 45
            0x02,        // bNumInterfaces 2
            0x01,        // bConfigurationValue
            0x00,        // iConfiguration (String Index)
            0x80,        // bmAttributes
            0x32,        // bMaxPower 100mA

            0x09,        // bLength
            0x04,        // bDescriptorType (Interface)
            0x00,        // bInterfaceNumber 0
            0x00,        // bAlternateSetting
            0x01,        // bNumEndpoints 1
            0x78,        // bInterfaceClass
            0x00,        // bInterfaceSubClass
            0x00,        // bInterfaceProtocol
            0x00,        // iInterface (String Index)

            0x09,        // bLength
            0x05,        // bDescriptorType (Endpoint)
            0x04,        // bEndpointAddress (OUT/H2D)
            0x05,        // bmAttributes (Isochronous, Async, Data EP)
            0x30, 0x00,  // wMaxPacketSize 48
            0x01,        // bInterval 1 (unit depends on device speed)
            0x00, 0x00, 
            0x09,        // bLength
            0x04,        // bDescriptorType (Interface)
            0x01,        // bInterfaceNumber 1
            0x00,        // bAlternateSetting
            0x01,        // bNumEndpoints 1
            0x78,        // bInterfaceClass
            0x00,        // bInterfaceSubClass
            0x00,        // bInterfaceProtocol
            0x00,        // iInterface (String Index)

            0x09,        // bLength
            0x05,        // bDescriptorType (Endpoint)
            0x85,        // bEndpointAddress (IN/D2H)
            0x05,        // bmAttributes (Isochronous, Async, Data EP)
            0x30, 0x00,  // wMaxPacketSize 48
            0x01,        // bInterval 1 (unit depends on device speed)
            0x00, 0x00, 
        };

    }; // COM

}; // namespace XboxOG 

#endif // _XBOX_OG_DESCRIPTORS_H_