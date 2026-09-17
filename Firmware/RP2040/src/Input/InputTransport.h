#pragma once

#include <cstdint>

/**
 * Explicit input transport ownership for a controller slot.
 * Do not infer transport from partially initialized Bluepad32 / USB structures.
 */
enum class InputTransport : uint8_t {
    NONE = 0,
    USB,
    BLUETOOTH_CLASSIC,
    BLUETOOTH_LE,
};

/**
 * Shared USB host lifecycle — separate from Bluetooth scan enable.
 * `root_connected && !mounted` is transitional, not automatically an error.
 */
enum class UsbHostState : uint8_t {
    DISCONNECTED = 0,
    ATTACHED_GRACE,
    ENUMERATING,
    MOUNTED,
    ENUMERATION_FAILED,
    RECOVERY_WAIT,
};
