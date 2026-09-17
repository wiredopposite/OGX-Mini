#ifndef GAMESIR_G7_PRO_BT_H_
#define GAMESIR_G7_PRO_BT_H_

/**
 * GameSir G7 Pro — Classic Bluetooth HID transport.
 *
 * Wired USB uses GameSirG7ProHost (19-byte DInput-shaped report).
 * Bluetooth uses a different layout (Report ID 0x07, 11 bytes) and often
 * fails SDP VID/PID — claim by remote name and skip SDP.
 */

#include <stdbool.h>
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

/** True if name is a GameSir G7 Pro BT identity; sets VID/PID 3537:1022. */
bool gamesir_g7pro_bt_does_name_match(uni_hid_device_t* d, const char* name);

/** Install dedicated Report 0x07 parser (no HID descriptor / SDP required). */
void gamesir_g7pro_bt_install_parser(uni_hid_device_t* d);

bool gamesir_g7pro_bt_parser_installed(const uni_hid_device_t* d);

void gamesir_g7pro_bt_parse_input_report(uni_hid_device_t* d, const uint8_t* report, uint16_t len);
void gamesir_g7pro_bt_init_report(uni_hid_device_t* d);

#ifdef __cplusplus
}
#endif

#endif /* GAMESIR_G7_PRO_BT_H_ */
