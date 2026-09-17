#ifndef FLYDIGI_APEX4_WUKONG_BT_L2CAP_H_
#define FLYDIGI_APEX4_WUKONG_BT_L2CAP_H_

/**
 * APEX 4 Classic BT — HID Control L2CAP timing gate.
 *
 * Ordinary Bluepad32 controllers keep the stock FSM. APEX candidates wait for
 * ACL + authentication + encryption, settle ~250 ms, then open PSM 0x0011.
 * RTX timeouts (0x69) retry without deleting the bond.
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

/** Register HCI handler (call once from BT init). */
void flydigi_apex4_bt_l2cap_gate_init(void);

/**
 * Intercept HID Control open for APEX candidates.
 * @return 1 if gate owns the request (deferred or will open); 0 = use stock path.
 */
int flydigi_apex4_bt_gate_open_hid_control(uni_hid_device_t* d);

/**
 * Handle L2CAP CHANNEL_OPENED for HID Control on an APEX candidate.
 * @return 1 if failure was fully handled (caller must not drop key / delete);
 *         0 if caller should continue with normal Bluepad32 failure handling
 *         (or success path).
 */
int flydigi_apex4_bt_gate_on_hid_control_result(uni_hid_device_t* d, uint8_t status, uint16_t local_cid);

/** Clear gate state when device disconnects. */
void flydigi_apex4_bt_l2cap_gate_on_disconnect(uni_hid_device_t* d);

/**
 * True if this L2CAP failure status must not delete the bond
 * (RTX timeout / resources / ERTM) — for any Classic controller.
 */
int flydigi_apex4_bt_l2cap_status_keep_bond(uint8_t status);

#ifdef __cplusplus
}
#endif

#endif /* FLYDIGI_APEX4_WUKONG_BT_L2CAP_H_ */
