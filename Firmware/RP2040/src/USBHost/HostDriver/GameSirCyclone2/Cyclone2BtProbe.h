#pragma once

#include <cstdint>

/**
 * Bluetooth discovery/lifecycle probe for GameSir Cyclone 2.
 *
 * Supported direct BT transports: DS4 (blue), HID/Android (yellow), Switch (red).
 * Direct Bluetooth XInput is NOT supported by GameSir — "Game Pair Mode" must be ignored.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_hid_device.h"

/** True for strong Cyclone BT name fingerprints (not shared Sony DS4 alone). */
int gamesir_cyclone2_bt_is_strong_candidate(const uni_hid_device_t* device);

/**
 * Exact Cyclone "Game Pair Mode" advertising name — not a usable BT XInput gamepad.
 * Returns 1 only for that exact name (no loose substring matching).
 */
int gamesir_cyclone2_bt_is_game_pair_mode(const char* name);

/** Discovery filter: ignore Game Pair Mode before a 20s connect timeout. */
int gamesir_cyclone2_bt_should_ignore_discovered(const char* name);

void gamesir_cyclone2_bt_on_discovered(const uint8_t* bd_addr, const char* name, uint16_t cod,
                                       uint8_t rssi);

void gamesir_cyclone2_bt_on_connected(uni_hid_device_t* device);
void gamesir_cyclone2_bt_on_ready(uni_hid_device_t* device);
void gamesir_cyclone2_bt_on_disconnected(uni_hid_device_t* device);

/**
 * If device is Game Pair Mode, log and disconnect/delete via normal Bluepad cleanup.
 * Returns 1 if ignored/cleaned up (caller must not continue treating as a pad).
 */
int gamesir_cyclone2_bt_reject_game_pair_mode_if_needed(uni_hid_device_t* device);

#ifdef __cplusplus
}
#endif
