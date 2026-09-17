#ifndef FLYDIGI_APEX4_WUKONG_BT_H_
#define FLYDIGI_APEX4_WUKONG_BT_H_

/**
 * Flydigi APEX 4 Wukong — Bluetooth PC transport.
 *
 * Two-stage lifecycle (do not permanently install before SDP/parser selection):
 *   1) Candidate + runtime layout confirm (16-byte Report 0x01)
 *   2) Final install after Bluepad32 guess_controller_type / setup_report_parser
 *
 * Decoder for Report 0x01 is stable — do not change packet field layout here
 * without a new capture.
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

/** Mark shared Xbox-like identity as APEX candidate (name / COD / early connect). */
void flydigi_apex4_bt_mark_candidate(uni_hid_device_t* device);

/** Observe a raw HID report; may confirm 16-byte Report 0x01 layout (no final claim). */
void flydigi_apex4_bt_note_raw_report(uni_hid_device_t* device, const uint8_t* report, uint16_t len);

/**
 * Called after Bluepad32 finishes VID/PID → setup_report_parser().
 * Re-installs APEX parser if candidate + layout confirmed so generic/Xbox
 * selection cannot keep ownership.
 */
void flydigi_apex4_bt_on_parser_selection_complete(uni_hid_device_t* device);

/** Called entering set_ready / set_ready_complete — last chance to own parser. */
void flydigi_apex4_bt_on_device_ready_transition(uni_hid_device_t* device);

/** True if dedicated APEX parse_input_report is currently installed. */
int flydigi_apex4_bt_parser_installed(const uni_hid_device_t* device);

int flydigi_apex4_bt_layout_confirmed(const uni_hid_device_t* device);

void flydigi_apex4_bt_parse_input_report(uni_hid_device_t* d, const uint8_t* report, uint16_t len);
void flydigi_apex4_bt_init_report(uni_hid_device_t* d);
void flydigi_apex4_bt_device_dump(uni_hid_device_t* d);

/** Clear per-slot state on disconnect. */
void flydigi_apex4_bt_on_slot_disconnected(uni_hid_device_t* device);

#ifdef __cplusplus
}
#endif

#endif /* FLYDIGI_APEX4_WUKONG_BT_H_ */
