#pragma once

#include <cstdint>

/**
 * Cyclone 2 transport / receiver state (shared by USB wired, 2.4 GHz dongle, BT).
 * Physical driver remains GAMESIR_CYCLONE2; protocol engines are reused per mode.
 */
namespace GameSirCyclone2Transport {

enum class Transport : uint8_t {
    Unknown = 0,
    Wired,
    UsbReceiver24Ghz,
    Bluetooth,
};

enum class ReceiverState : uint8_t {
    None = 0,
    ReceiverPresent,
    ControllerOffline,
    ControllerOnline,
};

enum class Mode : uint8_t {
    Unknown = 0,
    XInput,
    Ds4,
    Switch,
    Hid,
};

inline const char* transport_name(Transport t) {
    switch (t) {
        case Transport::Wired: return "WIRED";
        case Transport::UsbReceiver24Ghz: return "2.4GHZ_RECEIVER";
        case Transport::Bluetooth: return "BLUETOOTH";
        default: return "UNKNOWN";
    }
}

inline const char* receiver_state_name(ReceiverState s) {
    switch (s) {
        case ReceiverState::ReceiverPresent: return "RECEIVER_PRESENT";
        case ReceiverState::ControllerOffline: return "CONTROLLER_OFFLINE";
        case ReceiverState::ControllerOnline: return "CONTROLLER_ONLINE";
        default: return "NONE";
    }
}

inline const char* mode_name(Mode m) {
    switch (m) {
        case Mode::XInput: return "XINPUT";
        case Mode::Ds4: return "DS4";
        case Mode::Switch: return "SWITCH";
        case Mode::Hid: return "HID";
        default: return "UNKNOWN";
    }
}

inline const char* protocol_engine_name(Mode m) {
    switch (m) {
        case Mode::XInput: return "XINPUT";
        case Mode::Ds4: return "DS4";
        case Mode::Switch: return "SWITCH_PRO_STANDARD";
        case Mode::Hid: return "HID_PASSIVE";
        default: return "UNKNOWN";
    }
}

inline Mode mode_from_vid_pid(uint16_t vid, uint16_t pid) {
    if (vid == 0x3537 && (pid == 0x100B || pid == 0x1053)) {
        return Mode::XInput;
    }
    if (vid == 0x054C && pid == 0x09CC) {
        return Mode::Ds4;
    }
    if (vid == 0x057E && pid == 0x2009) {
        return Mode::Switch;
    }
    if (vid == 0x3537 && pid == 0x0575) {
        return Mode::Hid;
    }
    return Mode::Unknown;
}

} // namespace GameSirCyclone2Transport
