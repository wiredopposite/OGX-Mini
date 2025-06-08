#pragma once

#include <stdint.h>
#include "common/usb_def.h"
#include "usb/descriptors/xid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XBSB_EPSIZE_CTRL    ((uint8_t)8)
#define XBSB_EPSIZE_IN      ((uint16_t)32)
#define XBSB_EPSIZE_OUT     ((uint16_t)32)
#define XBSB_EPADDR_IN      ((uint8_t)2 | USB_EP_DIR_IN)
#define XBSB_EPADDR_OUT     ((uint8_t)1)

static const usb_desc_device_t XBSB_DESC_DEVICE = {
    .bLength            = sizeof(usb_desc_device_t),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = USB_BCD_VERSION_1_1,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = XBSB_EPSIZE_CTRL,
    .idVendor           = 0x0A7B,
    .idProduct          = 0x0289,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0,
    .iProduct           = 0,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

/*  See: https://github.com/caosdoar/SteelBattalionDriver/blob/master/sys/hidusbsteelbattalion.h 
    Might use this later */
#if 0
static const uint8_t XBSB_DESC_REPORT[] = {
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,                    // USAGE (Game Pad)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x95, 0x27,                    //   REPORT_COUNT (39)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
	0x05, 0x09,                    //   USAGE_PAGE (Button)
	0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
	0x29, 0x27,                    //   USAGE_MAXIMUM (Button 39)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
	0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
	0x09, 0x01,                    //   USAGE (Pointer)
	0xa1, 0x00,                    //   COLLECTION (Physical)
	0x09, 0x30,                    //     USAGE (X)
	0x09, 0x31,                    //     USAGE (Y)
	0x09, 0x32,                    //     USAGE (Z)
	0x09, 0x33,                    //     USAGE (Rx)
	0x09, 0x34,                    //     USAGE (Ry)
	0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
	0x75, 0x08,                    //     REPORT_SIZE (8)
	0x95, 0x05,                    //     REPORT_COUNT (5)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)
	0xc0,                          //   END_COLLECTION
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
	0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
	0x09, 0x36,                    //   USAGE (Slider)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x05, 0x02,                    //   USAGE_PAGE (Simulation Controls)
	0x09, 0xc5,                    //   USAGE (Brake)
	0x09, 0xbb,                    //   USAGE (Throttle)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x95, 0x02,                    //   REPORT_COUNT (2)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x09, 0xc2,                    //   USAGE (Weapons Select)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x09, 0xc7,                    //   USAGE (Shifter)
	0x15, 0x80,                    //   LOGICAL_MINIMUM (-128)
	0x25, 0x7f,                    //   LOGICAL_MAXIMUM (127)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0xc0                           // END_COLLECTION
};
#endif

typedef struct __attribute__((packed)) {
    usb_desc_config_t   config;
    usb_desc_itf_t      itf;
    usb_desc_endpoint_t ep_in;
    usb_desc_endpoint_t ep_out;
} xbsb_desc_config_t;

static const xbsb_desc_config_t XBSB_DESC_CONFIG = {
    .config = {
        .bLength                = sizeof(usb_desc_config_t),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(xbsb_desc_config_t),
        .bNumInterfaces         = 1,
        .bConfigurationValue    = 1,
        .iConfiguration         = 0,
        .bmAttributes           = USB_ATTR_RESERVED,
        .bMaxPower              = 0x32
    },
    .itf = {
        .bLength                = sizeof(usb_desc_itf_t),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_ITF_CLASS_XID,
        .bInterfaceSubClass     = USB_ITF_SUBCLASS_XID,
        .bInterfaceProtocol     = 0,
        .iInterface             = 0
    },
    .ep_in = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = XBSB_EPADDR_IN,
        .bmAttributes           = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize         = XBSB_EPSIZE_IN,
        .bInterval              = 4
    },
    .ep_out = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = XBSB_EPADDR_OUT,
        .bmAttributes           = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize         = XBSB_EPSIZE_OUT,
        .bInterval              = 4
    }
};

#define XBSB_AIMING_MID                     ((uint16_t)32768)
#define XBSB_BTN2_TOGGLE_MID                ((uint16_t)0xFFFC)

#define XBSB_BTN0_RIGHTJOYMAINWEAPON        ((uint16_t)0x0001)
#define XBSB_BTN0_RIGHTJOYFIRE              ((uint16_t)0x0002)
#define XBSB_BTN0_RIGHTJOYLOCKON            ((uint16_t)0x0004)
#define XBSB_BTN0_EJECT                     ((uint16_t)0x0008)
#define XBSB_BTN0_COCKPITHATCH              ((uint16_t)0x0010)
#define XBSB_BTN0_IGNITION                  ((uint16_t)0x0020)
#define XBSB_BTN0_START                     ((uint16_t)0x0040)
#define XBSB_BTN0_MULTIMONOPENCLOSE         ((uint16_t)0x0080)
#define XBSB_BTN0_MULTIMONMAPZOOMINOUT      ((uint16_t)0x0100)
#define XBSB_BTN0_MULTIMONMODESELECT        ((uint16_t)0x0200)
#define XBSB_BTN0_MULTIMONSUBMONITOR        ((uint16_t)0x0400)
#define XBSB_BTN0_MAINMONZOOMIN             ((uint16_t)0x0800)
#define XBSB_BTN0_MAINMONZOOMOUT            ((uint16_t)0x1000)
#define XBSB_BTN0_FUNCTIONFSS               ((uint16_t)0x2000)
#define XBSB_BTN0_FUNCTIONMANIPULATOR       ((uint16_t)0x4000)
#define XBSB_BTN0_FUNCTIONLINECOLORCHANGE   ((uint16_t)0x8000)

#define XBSB_BTN1_WASHING                   ((uint16_t)0x0001)
#define XBSB_BTN1_EXTINGUISHER              ((uint16_t)0x0002)
#define XBSB_BTN1_CHAFF                     ((uint16_t)0x0004)
#define XBSB_BTN1_FUNCTIONTANKDETACH        ((uint16_t)0x0008)
#define XBSB_BTN1_FUNCTIONOVERRIDE          ((uint16_t)0x0010)
#define XBSB_BTN1_FUNCTIONNIGHTSCOPE        ((uint16_t)0x0020)
#define XBSB_BTN1_FUNCTIONF1                ((uint16_t)0x0040)
#define XBSB_BTN1_FUNCTIONF2                ((uint16_t)0x0080)
#define XBSB_BTN1_FUNCTIONF3                ((uint16_t)0x0100)
#define XBSB_BTN1_WEAPONCONMAIN             ((uint16_t)0x0200)
#define XBSB_BTN1_WEAPONCONSUB              ((uint16_t)0x0400)
#define XBSB_BTN1_WEAPONCONMAGAZINE         ((uint16_t)0x0800)
#define XBSB_BTN1_COMM1                     ((uint16_t)0x1000)
#define XBSB_BTN1_COMM2                     ((uint16_t)0x2000)
#define XBSB_BTN1_COMM3                     ((uint16_t)0x4000)
#define XBSB_BTN1_COMM4                     ((uint16_t)0x8000)

#define XBSB_BTN2_COMM5                     ((uint16_t)0x0001)
#define XBSB_BTN2_LEFTJOYSIGHTCHANGE        ((uint16_t)0x0002)
#define XBSB_BTN2_TOGGLEFILTERCONTROL       ((uint16_t)0x0004)
#define XBSB_BTN2_TOGGLEOXYGENSUPPLY        ((uint16_t)0x0008)
#define XBSB_BTN2_TOGGLEFUELFLOWRATE        ((uint16_t)0x0010)
#define XBSB_BTN2_TOGGLEBUFFREMATERIAL      ((uint16_t)0x0020)
#define XBSB_BTN2_TOGGLEVTLOCATION          ((uint16_t)0x0040)

#define XBSB_AXIS_AIMINGX                   ((uint16_t)0x0001)
#define XBSB_AXIS_AIMINGY                   ((uint16_t)0x0002)
#define XBSB_AXIS_LEVER                     ((uint16_t)0x0004)
#define XBSB_AXIS_SIGHTX                    ((uint16_t)0x0008)
#define XBSB_AXIS_SIGHTY                    ((uint16_t)0x0010)
#define XBSB_AXIS_LPEDAL                    ((uint16_t)0x0020)
#define XBSB_AXIS_MPEDAL                    ((uint16_t)0x0040)
#define XBSB_AXIS_RPEDAL                    ((uint16_t)0x0080)
#define XBSB_AXIS_TUNER                     ((uint16_t)0x0100)
#define XBSB_AXIS_GEAR                      ((uint16_t)0x0200)

#define XBSB_GEAR_R                         ((int8_t)7)
#define XBSB_GEAR_N                         ((int8_t)8)
#define XBSB_GEAR_1                         ((int8_t)9)
#define XBSB_GEAR_2                         ((int8_t)10)
#define XBSB_GEAR_3                         ((int8_t)11)
#define XBSB_GEAR_4                         ((int8_t)12)
#define XBSB_GEAR_5                         ((int8_t)13)

#define XBSB_DIAL_MIN                       ((int8_t)1)
#define XBSB_DIAL_MAX                       ((int8_t)15)

typedef struct xbsb_report_in_ {
    uint8_t     zero;
    uint8_t     bLength;
    uint16_t    dButtons[3];
    uint16_t    aimingX;       //0 to 2^16 left to right
    uint16_t    aimingY;       //0 to 2^16 top to bottom
    int16_t     rotationLever;
    int16_t     sightChangeX;
    int16_t     sightChangeY;
    uint16_t    leftPedal;      //Sidestep, 0x0000 to 0xFF00
    uint16_t    middlePedal;    //Brake, 0x0000 to 0xFF00
    uint16_t    rightPedal;     //Acceleration, 0x0000 to oxFF00
    int8_t      tunerDial;        //0-15 is from 9oclock, around clockwise
    int8_t      gearLever;        //7-13 is gears R,1,2,3,4,5
} xbsb_report_in_t;
_Static_assert(sizeof(xbsb_report_in_t) == 26, "xbsb_report_in_t size mismatch");

typedef struct xbsb_report_out_ {
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
} xbsb_report_out_t;
_Static_assert(sizeof(xbsb_report_out_t) == 22, "xbsb_report_out_t size mismatch");

static const usb_desc_xid_t XBSB_DESC_XID = {
    .bLength                 = sizeof(usb_desc_xid_t),
    .bDescriptorType         = USB_DTYPE_XID,
    .bcdXid                  = 0x0100,
    .bType                   = 0x80,
    .bSubType                = 0x01,
    .bMaxInputReportSize     = sizeof(xbsb_report_in_t),
    .bMaxOutputReportSize    = sizeof(xbsb_report_out_t),
    .wAlternateProductIds[0] = 0xFFFF,
    .wAlternateProductIds[1] = 0xFFFF,
    .wAlternateProductIds[2] = 0xFFFF,
    .wAlternateProductIds[3] = 0xFFFF
};

#ifdef __cplusplus
}
#endif