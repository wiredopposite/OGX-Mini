#pragma once

#include <cstdint>

#include "Input/InputTransport.h"
#include "USBHost/HostDriver/HostDriverTypes.h"

/**
 * Per–OGX gamepad-index ownership (USB HostManager ↔ PadIn ↔ device output).
 * Distinct from Bluepad32 BT slots — do not confuse the two in logs.
 */
namespace InputSlot {

struct State {
    InputTransport transport{InputTransport::NONE};
    uint8_t usb_addr{0};
    uint8_t usb_instance{0};
    HostDriverType physical_driver{HostDriverType::UNKNOWN};
    /** Protocol label for logs (SWITCH / DS4 / XINPUT / …); may differ from physical_driver. */
    const char* protocol{"NONE"};
};

const char* transport_name(InputTransport t);
const char* driver_name(HostDriverType t);
/** Infer protocol label from VID/PID for dedicated multi-personality hosts. */
const char* protocol_for_ids(HostDriverType physical, uint16_t vid, uint16_t pid);

State get(uint8_t slot);
void clear(uint8_t slot, const char* reason);

/** True if this OGX gamepad index is owned by an active USB host driver. */
bool usb_owns(uint8_t slot);

/**
 * Bind USB host ownership to an OGX gamepad slot after HostManager::setup_driver succeeds.
 * Logs [USB SLOT BIND] / [SLOT STATE CHANGE].
 */
void bind_usb(uint8_t slot, uint8_t usb_addr, uint8_t usb_instance,
              HostDriverType physical, uint16_t vid, uint16_t pid,
              const char* caller);

/** Dump all MAX_GAMEPADS slots (and note BT slots are separate). */
void log_all(const char* context);

/** Rate-limited HID RX → slot route confirmation. */
void log_hid_rx_route(uint8_t usb_addr, uint8_t usb_instance, uint8_t report_id, uint16_t len,
                      uint8_t owning_slot, HostDriverType physical);

/** Hot-path counters (no printf). Call poll_perf_log() ~1 Hz from main loop. */
void note_hid_rx();
void note_decoded();
void note_rearm(bool ok);
void note_xinput_sent();
void note_xinput_busy();
void poll_perf_log();

} // namespace InputSlot
