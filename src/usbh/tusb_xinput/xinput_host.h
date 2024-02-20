// Copyright 2020, Ryan Wendland, usb64
// SPDX-License-Identifier: MIT

#ifndef _TUSB_XINPUT_HOST_H_
#define _TUSB_XINPUT_HOST_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "host/usbh.h"
#include "host/usbh_pvt.h"

//--------------------------------------------------------------------+
// Class Driver Configuration
//--------------------------------------------------------------------+

#ifndef CFG_TUH_XINPUT_EPIN_BUFSIZE
#define CFG_TUH_XINPUT_EPIN_BUFSIZE 64
#endif

#ifndef CFG_TUH_XINPUT_EPOUT_BUFSIZE
#define CFG_TUH_XINPUT_EPOUT_BUFSIZE 64
#endif

//XINPUT defines and struct format from
//https://docs.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad
#define XINPUT_GAMEPAD_DPAD_UP 0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN 0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT 0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT 0x0008
#define XINPUT_GAMEPAD_START 0x0010
#define XINPUT_GAMEPAD_BACK 0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB 0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB 0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER 0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_GUIDE 0x0400
#define XINPUT_GAMEPAD_SHARE 0x0800
#define XINPUT_GAMEPAD_A 0x1000
#define XINPUT_GAMEPAD_B 0x2000
#define XINPUT_GAMEPAD_X 0x4000
#define XINPUT_GAMEPAD_Y 0x8000
#define MAX_PACKET_SIZE 32

typedef struct xinput_gamepad
{
    uint16_t wButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    int16_t sThumbLX;
    int16_t sThumbLY;
    int16_t sThumbRX;
    int16_t sThumbRY;
} xinput_gamepad_t;

typedef enum
{
    XINPUT_UNKNOWN = 0,
    XBOXONE,
    XBOX360_WIRELESS,
    XBOX360_WIRED,
    XBOXOG
} xinput_type_t;

typedef struct
{
    xinput_type_t type;
    xinput_gamepad_t pad;
    uint8_t connected;
    uint8_t new_pad_data;
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;

    uint16_t epin_size;
    uint16_t epout_size;

    uint8_t epin_buf[CFG_TUH_XINPUT_EPIN_BUFSIZE];
    uint8_t epout_buf[CFG_TUH_XINPUT_EPOUT_BUFSIZE];

    xfer_result_t last_xfer_result;
    uint32_t last_xferred_bytes;
} xinputh_interface_t;

extern usbh_class_driver_t const usbh_xinput_driver;

/**
 * @brief Callback function called when a report is received from an XInput device.
 *
 * This function is called when a report is received from the specified XInput device.
 *
 * @param dev_addr Device address of the XInput device.
 * @param instance Instance of the XInput device.
 * @param xid_itf Pointer to the xif_itf structure containing the received report data and transfer result
 * @param len Length of the received report data.
 */
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* xid_itf, uint16_t len);

/**
 * @brief Weak callback function called when a report is sent to an XInput device.
 *
 * This function is a weakly bound callback called when a report is sent to the specified XInput device.
 * Implementing this function is optional, and the weak attribute allows it to be overridden if needed.
 *
 * @param dev_addr Device address of the XInput device.
 * @param instance Instance of the XInput device.
 * @param report Pointer to the sent report data.
 * @param len Length of the sent report data.
 */
TU_ATTR_WEAK void tuh_xinput_report_sent_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

/**
 * @brief Weak callback function called when an XInput device is unmounted.
 *
 * This function is a weakly bound callback called when an XInput device is unmounted.
 * Implementing this function is optional, and the weak attribute allows it to be overridden if needed.
 *
 * @param dev_addr Device address of the unmounted XInput device.
 * @param instance Instance of the unmounted XInput device.
 */
TU_ATTR_WEAK void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance);

/**
 * @brief Weak callback function called when an XInput device is mounted.
 *
 * This function is a weakly bound callback called when an XInput device is mounted.
 * Implementing this function is optional, and the weak attribute allows it to be overridden if needed.
 *
 * @param dev_addr Device address of the mounted XInput device.
 * @param instance Instance of the mounted XInput device.
 * @param xinput_itf Pointer to the XInput interface information.
 */
TU_ATTR_WEAK void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf);

/**
 * @brief Receive a report from an XInput device.
 *
 * This function attempts to receive a report from the specified XInput device.
 *
 * @param dev_addr Device address of the XInput device.
 * @param instance Instance of the XInput device.
 * @return True if the report is received successfully, false otherwise.
 */
bool tuh_xinput_receive_report(uint8_t dev_addr, uint8_t instance);

/**
 * @brief Send a report to an XInput device.
 *
 * This function attempts to send a report to the specified XInput device. This is a non blocking function.
 * tuh_xinput_report_sent_cb() is called when the transfer is completed.
 *
 * @param dev_addr Device address of the XInput device.
 * @param instance Instance of the XInput device.
 * @param txbuf Pointer to the data to be sent.
 * @param len Length of the data to be sent.
 * @return True if the report is sent successfully, false otherwise.
 */
bool tuh_xinput_send_report(uint8_t dev_addr, uint8_t instance, const uint8_t *txbuf, uint16_t len);

/**
 * @brief Set LED status on an XInput device. (Applicated to Xbox 360 controllers only)
 *
 * This function sets the LED status on the specified XInput device for the specified quadrant.
 *
 * @param dev_addr Device address of the XInput device.
 * @param instance Instance of the XInput device.
 * @param quadrant Quadrant of the LED to set.
 * @param block Indicates whether the operation should be blocking.
 * @return True if LED status is set successfully, false otherwise.
 */
bool tuh_xinput_set_led(uint8_t dev_addr, uint8_t instance, uint8_t quadrant, bool block);

/**
 * @brief Set rumble values on an XInput device.
 *
 * This function sets the rumble values on the specified XInput device for left and right motors.
 *
 * @param dev_addr Device address of the XInput device.
 * @param instance Instance of the XInput device.
 * @param lValue Intensity of the left motor rumble (0 to 255)
 * @param rValue Intensity of the right motor rumble. (0 to 255)
 * @param block Indicates whether the operation should be blocking.
 * @return True if rumble values are set successfully, false otherwise.
 */
bool tuh_xinput_set_rumble(uint8_t dev_addr, uint8_t instance, uint8_t lValue, uint8_t rValue, bool block);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_XINPUT_HOST_H_ */