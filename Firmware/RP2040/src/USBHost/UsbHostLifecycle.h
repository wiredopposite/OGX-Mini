#pragma once

#include <cstdint>

#include "Input/InputTransport.h"

/**
 * Shared USB host enumeration / recovery policy.
 * One automatic root-port reset max per attach cycle; never hammer the bus
 * merely because a device is connected but not yet mounted.
 */
namespace UsbHostLifecycle {

enum class WatchAction : uint8_t {
    NONE = 0,
    /** Perform exactly one tuh_rhport_reset_bus cycle, then call on_recovery_reset_done(). */
    ONE_ROOT_RESET,
    /** USB host stays up; Bluetooth scans may resume. */
    RESUME_BT_SCANS,
};

void reset_for_host_stop();
/** Call after successful tuh_init while a cable may already be present. */
void on_host_started(bool root_connected_at_init);

void on_bus_attach(uint8_t rhport);
void on_bus_remove(uint8_t rhport);
void on_device_configured(uint8_t daddr);
void on_device_unmounted(uint8_t daddr);
/** Class driver opened an interface — counts as enumeration progress. */
void on_interface_opened(const char* tag);

UsbHostState state();
bool recovery_attempted();
const char* state_name(UsbHostState st);
const char* last_progress_name();

/**
 * Periodic watchdog (≈250–500 ms). May request ONE root reset or BT scan resume.
 * Does not call TinyUSB APIs that reset the bus — caller performs the reset.
 */
WatchAction watch(bool root_connected, bool tuh_device_configured, bool host_pad_mounted);

/** After the caller completes ONE_ROOT_RESET. */
void on_recovery_reset_done();

} // namespace UsbHostLifecycle
