#pragma once

#include <cstdint>

/**
 * Shared TinyUSB HID host RX arm / raw / re-arm diagnostics.
 * Does not change report parsing — only ensures interrupt-IN stays armed and logs evidence.
 */
namespace HidRxDebug {

/** After HostManager::setup_driver: arm if idle, log SUCCESS/FAILED. */
void ensure_armed_after_mount(uint8_t addr, uint8_t instance);

/** Log changing reports at the start of tuh_hid_report_received_cb (before routing). */
void log_raw_rx_if_changed(uint8_t addr, uint8_t instance, const uint8_t* report, uint16_t len);

/**
 * After process_report: if the EP is idle, the driver forgot to re-arm — re-arm and log.
 * If already pending, log the first few SUCCESS re-arms only.
 */
void ensure_rearmed_after_rx(uint8_t addr, uint8_t instance);

} // namespace HidRxDebug
