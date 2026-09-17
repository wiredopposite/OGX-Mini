#ifndef FLYDIGI_APEX4_WUKONG_BT_PROBE_H_
#define FLYDIGI_APEX4_WUKONG_BT_PROBE_H_

/**
 * Bluetooth candidate probe + lifecycle hooks for Flydigi APEX 4 Wukong.
 *
 * Architecture:
 *   FlydigiApex4WukongDriver
 *       +-- Wired PC Transport  (FlydigiApex4WukongHost)
 *       +-- Bluetooth PC Transport (FlydigiApex4WukongBt + this probe)
 *
 * IMPORTANT: 045E:02E0 / "Xbox Wireless Controller" is a shared identity.
 * We install a dedicated Report 0x01 parser on candidates without modifying
 * the Xbox Bluepad32 parser source. Production fingerprinting can tighten later.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "uni_hid_device.h"
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** True if name and/or VID:PID match the shared Xbox BT identity used by APEX 4 PC Bluetooth. */
int flydigi_apex4_bt_is_candidate(const uni_hid_device_t* device);

/** Discovery-time note (name/COD often available before VID/PID). */
void flydigi_apex4_bt_on_discovered(const uint8_t* bd_addr, const char* name, uint16_t cod, uint8_t rssi);

/** Connected (HID not necessarily ready yet). */
void flydigi_apex4_bt_on_connected(uni_hid_device_t* device);

/** Device ready — dump identity, connection, HID descriptor summary (candidate only). */
void flydigi_apex4_bt_on_ready(uni_hid_device_t* device);

/** Disconnect lifecycle log (candidate only). */
void flydigi_apex4_bt_on_disconnected(uni_hid_device_t* device);

/**
 * Raw HID input report hook (BLE HOGP or Classic interrupt payload after 0xA1).
 * Logs only when candidate and report bytes change.
 */
void flydigi_apex4_bt_on_raw_report(uni_hid_device_t* device, const uint8_t* report, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* FLYDIGI_APEX4_WUKONG_BT_PROBE_H_ */
