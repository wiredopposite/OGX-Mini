#ifndef BLUEPAD32_CLASSIC_PAIRING_DEBUG_H_
#define BLUEPAD32_CLASSIC_PAIRING_DEBUG_H_

/**
 * Debug instrumentation + Classic pairing helpers for Pico W / Pico 2 W.
 *
 * - Boot dump of Bluepad32 / BTstack versions and OGX patch status
 * - HCI/L2CAP security event logging (Debug builds)
 * - Incoming Classic auth kick after ACL connect (starts SSP when no bond)
 * - Optional one-shot key clear via OGXM_BT_CLEAR_KEYS_ON_BOOT
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Call once from Bluepad32 init_complete (BTstack thread). */
void ogxm_classic_pairing_debug_init(void);

/** Delete all stored BR/EDR + LE bonds (safe: schedules on BT thread). */
void ogxm_bt_debug_clear_keys(void);

/** List stored Classic link keys (no secret bytes). */
void ogxm_bt_debug_list_keys(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUEPAD32_CLASSIC_PAIRING_DEBUG_H_ */
