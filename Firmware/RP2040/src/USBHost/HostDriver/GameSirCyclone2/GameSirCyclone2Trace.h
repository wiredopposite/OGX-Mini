#ifndef GAMESIR_CYCLONE2_TRACE_H_
#define GAMESIR_CYCLONE2_TRACE_H_

#include <cstdint>

#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Transport.h"

/**
 * Debug-only USB personality / mode-change tracer for GameSir Cyclone 2.
 * Proves green XInput (3537:100B/1053) vs red Switch (057E:2009) re-enumeration.
 * Also tracks 2.4 GHz receiver idle (3537:0575) vs controller-online remounts.
 */
namespace GameSirCyclone2Trace {

enum class Mode : uint8_t {
    Unknown = 0,
    XInput,
    Switch,
    Ds4,
    Hid,
    OtherGameSir,
};

void on_bus_attach(uint8_t rhport);
void on_bus_remove(uint8_t rhport);

/** ISR-safe variants — no logging / no USB I/O when in_isr. */
void on_bus_attach(uint8_t rhport, bool in_isr);
void on_bus_remove(uint8_t rhport, bool in_isr);

/** Called when a device finishes configuration (tuh_mount_cb). */
void on_device_configured(uint8_t address);

/** Called on tuh_umount_cb (device gone). */
void on_device_unmounted(uint8_t address);

/** XInput class driver actually claimed an Xbox-style interface. */
void on_xinput_claimed(uint8_t address, uint8_t instance, uint8_t itf_num,
                       uint8_t ep_in, uint8_t ep_out);

/** Interrupt OUT / control TX while a Cyclone XInput candidate is active. */
void log_host_tx(uint8_t address, const char* kind, uint8_t ep,
                 const uint8_t* data, uint16_t len);

void log_host_ctrl(uint8_t address, uint8_t bm_req, uint8_t b_req,
                   uint16_t w_value, uint16_t w_index, uint16_t w_len);

bool is_xinput_candidate(uint16_t vid, uint16_t pid);
Mode infer_mode(uint16_t vid, uint16_t pid);
const char* mode_name(Mode m);
const char* led_expected(Mode m);

/** Cable-attach timestamp (ms since boot); 0 if unknown. */
uint32_t cable_attach_ms();

/**
 * True after this cable session has seen Cyclone 2 XInput (3537:100B/1053).
 * Used so red NS (057E:2009) can be owned by GameSirCyclone2Host without
 * claiming genuine Switch Pro controllers that appear cold.
 */
bool cyclone_session_active();

/** Mark sticky Cyclone XInput session (wired green mode). Call from dedicated host init. */
void note_cyclone_xinput_seen();

/**
 * Claim Switch NS (057E:2009) when physical Cyclone affinity is sticky:
 * prior XInput session and/or 2.4 GHz receiver (3537:0575) session.
 * Does NOT globally claim all 057E:2009 devices.
 */
bool should_own_switch_ns(uint16_t vid, uint16_t pid);

/**
 * Cold-plug / remount Cyclone Switch fingerprint (never global VID/PID claim).
 *
 * Known presentations (observed hardware):
 *   A) Product "Gamepad" and/or bcdDevice 0x0326
 *   B) Product "Pro Controller" AND bcdDevice 0x0116
 *      (genuine Nintendo Pro uses bcd ~0x0200/0x0210 — not claimed here)
 *
 * Also accepts 057E:2009 when receiver affinity is already sticky.
 */
bool looks_like_cyclone_switch_ns(uint8_t address, uint16_t vid, uint16_t pid);

/** One-shot mount banner for Cyclone Switch ownership (affinity or fingerprint). */
void log_switch_driver_selection_banner(uint8_t address, uint16_t vid, uint16_t pid);

/** Claim DS4 personality only when cyclone_session_active() and 054C:09CC. */
bool should_own_ds4(uint16_t vid, uint16_t pid);

/**
 * Yellow HID / idle receiver: 3537:0575 is always Cyclone-owned when seen.
 * Alone does NOT prove an active HID-mode controller (may be idle 2.4 GHz dongle).
 */
bool should_own_hid_passive(uint16_t vid, uint16_t pid);

/**
 * While yellow HID safety is active, application OUT must be suppressed.
 * Enumeration / TinyUSB class traffic is still allowed by the stack.
 */
void set_hid_passive_safety(uint8_t address, bool active);
bool hid_passive_safety_active(uint8_t address);
void log_blocked_hid_out(uint8_t address, uint8_t instance, const char* caller,
                         const uint8_t* data, uint16_t len);

/** True if this USB session saw 3537:0575 (possible idle receiver or HID mode). */
bool receiver_session_active();

/**
 * Infer USB transport for a Cyclone personality mount.
 * If 0575 was seen recently in-session and we remount as XInput/Switch/DS4 → 2.4 GHz.
 * Cold-plug XInput without prior 0575 → Wired.
 */
GameSirCyclone2Transport::Transport infer_usb_transport(uint16_t vid, uint16_t pid);

GameSirCyclone2Transport::ReceiverState receiver_state();
void set_receiver_controller_online(bool online);
void note_0575_mounted();
void note_controller_personality_mounted(uint16_t vid, uint16_t pid);

/** Unified connection banner for every Cyclone 2 USB mount. */
void log_unified_banner(uint16_t vid, uint16_t pid, const char* bt_name,
                        bool input_active, const char* protocol_engine);

/**
 * True while GAMESIR_CYCLONE2 Switch personality is active (USB or BT).
 * XInput device uses this for a final A↔B / X↔Y face swap only.
 * Always compiled (not debug-only).
 */
void set_cyclone_switch_input_active(bool active);
bool cyclone_switch_input_active();

} // namespace GameSirCyclone2Trace

#endif // GAMESIR_CYCLONE2_TRACE_H_
