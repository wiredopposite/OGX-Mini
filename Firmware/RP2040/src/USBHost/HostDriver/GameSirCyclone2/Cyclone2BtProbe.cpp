#include "USBHost/HostDriver/GameSirCyclone2/Cyclone2BtProbe.h"

#include <cstring>

#include "Board/Config.h"
#include "Board/ogxm_log.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Trace.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Transport.h"
#include "controller/uni_controller_type.h"

namespace {

bool name_has(const char* name, const char* needle) {
    if (!name || !needle) {
        return false;
    }
    return std::strstr(name, needle) != nullptr;
}

bool is_gamesir_cyclone_name(const char* name) {
    if (!name || !name[0]) {
        return false;
    }
    /* Official HID/Android / branded BT names; do not treat bare "Wireless Controller" as Cyclone. */
    return name_has(name, "GameSir-Cyclone") || name_has(name, "GameSir Cyclone") ||
           name_has(name, "Gamesir-Cyclone") || name_has(name, "Cyclone 2");
}

/** Sticky: this BT session has seen a Cyclone-specific advertising name (not shared DS4 alone). */
bool bt_cyclone_known_ = false;
/** Face-swap armed from BT Switch path (clear on that BT disconnect). */
bool bt_switch_face_swap_armed_ = false;

void note_bt_cyclone_known(const char* reason) {
    if (!bt_cyclone_known_) {
        bt_cyclone_known_ = true;
        OGXM_LOG("[CYCLONE2 BT] cyclone identity noted (%s) — sticky for Switch face map\n",
                 reason ? reason : "?");
    }
}

bool is_switch_pro_bt(const uni_hid_device_t* device) {
    return device && device->controller_type == CONTROLLER_TYPE_SwitchProController;
}

bool cyclone_context_for_switch_face_swap(const uni_hid_device_t* device) {
    if (!device) {
        return false;
    }
    if (is_gamesir_cyclone_name(device->name)) {
        return true;
    }
    if (bt_cyclone_known_) {
        return true;
    }
#if defined(CONFIG_EN_USB_HOST)
    if (GameSirCyclone2Trace::cyclone_session_active() ||
        GameSirCyclone2Trace::receiver_session_active()) {
        return true;
    }
#endif
    /*
     * Cyclone Switch (RED) over BT advertises as Nintendo "Pro Controller".
     * Same final XInput face remap as wired/dongle Switch. Not a driver claim —
     * only arms the existing A↔B / X↔Y output correction.
     */
    if (device->name[0] && std::strcmp(device->name, "Pro Controller") == 0) {
        return true;
    }
    return false;
}

void arm_bt_switch_face_swap_if_needed(const uni_hid_device_t* device) {
    if (!is_switch_pro_bt(device) || !cyclone_context_for_switch_face_swap(device)) {
        return;
    }
    bt_switch_face_swap_armed_ = true;
    GameSirCyclone2Trace::set_cyclone_switch_input_active(true);
    OGXM_LOG("[CYCLONE2 BT] Switch face map armed (final XInput A↔B X↔Y) name=\"%s\"\n",
             device->name[0] ? device->name : "?");
}

void clear_bt_switch_face_swap_if_armed() {
    if (!bt_switch_face_swap_armed_) {
        return;
    }
    bt_switch_face_swap_armed_ = false;
    GameSirCyclone2Trace::set_cyclone_switch_input_active(false);
    OGXM_LOG("[CYCLONE2 BT] Switch face map cleared\n");
}

void log_bt_banner(const char* phase, const uni_hid_device_t* device, const char* name) {
#if defined(CONFIG_OGXM_DEBUG)
    using namespace GameSirCyclone2Transport;
    OGXM_LOG("[CYCLONE2 BT] %s name=\"%s\" transport=BLUETOOTH\n",
             phase ? phase : "?", name && name[0] ? name : "(none)");
    if (device) {
        OGXM_LOG("[CYCLONE2 BT] VID=%04X PID=%04X controller_type=%u\n",
                 device->vendor_id, device->product_id,
                 static_cast<unsigned>(device->controller_type));
    }
#else
    (void)phase;
    (void)device;
    (void)name;
#endif
}

void log_game_pair_mode_ignored(const char* where) {
    OGXM_LOG("\n[CYCLONE2 BT]\n");
    OGXM_LOG("Game Pair Mode detected (%s)\n", where ? where : "?");
    OGXM_LOG("\n");
    OGXM_LOG("Direct Bluetooth XInput is not a supported controller transport.\n");
    OGXM_LOG("Ignoring candidate.\n");
    OGXM_LOG("Use BLUE/DS4, YELLOW/HID, or RED/Switch over Bluetooth instead.\n\n");
}

} // namespace

extern "C" int gamesir_cyclone2_bt_is_game_pair_mode(const char* name) {
    /* Exact match only — do not block unrelated devices with similar substrings. */
    return (name && name[0] && std::strcmp(name, "Game Pair Mode") == 0) ? 1 : 0;
}

extern "C" int gamesir_cyclone2_bt_should_ignore_discovered(const char* name) {
    return gamesir_cyclone2_bt_is_game_pair_mode(name);
}

extern "C" int gamesir_cyclone2_bt_is_strong_candidate(const uni_hid_device_t* device) {
    if (!device) {
        return 0;
    }
    if (gamesir_cyclone2_bt_is_game_pair_mode(device->name)) {
        return 0;
    }
    return is_gamesir_cyclone_name(device->name) ? 1 : 0;
}

extern "C" int gamesir_cyclone2_bt_reject_game_pair_mode_if_needed(uni_hid_device_t* device) {
    if (!device || !gamesir_cyclone2_bt_is_game_pair_mode(device->name)) {
        return 0;
    }
    note_bt_cyclone_known("Game Pair Mode");
    log_game_pair_mode_ignored("connected");
    uni_hid_device_disconnect(device);
    uni_hid_device_delete(device);
    return 1;
}

extern "C" void gamesir_cyclone2_bt_on_discovered(const uint8_t* bd_addr, const char* name,
                                                  uint16_t cod, uint8_t rssi) {
    (void)bd_addr;
    (void)cod;
    (void)rssi;
    if (gamesir_cyclone2_bt_is_game_pair_mode(name)) {
        note_bt_cyclone_known("Game Pair Mode discovery");
        log_game_pair_mode_ignored("discovery");
        return;
    }
    if (is_gamesir_cyclone_name(name)) {
        note_bt_cyclone_known("strong GameSir name");
#if defined(CONFIG_OGXM_DEBUG)
        OGXM_LOG("[CYCLONE2 BT] discovered strong name=\"%s\" COD=0x%04X RSSI=%d\n",
                 name ? name : "?", cod, static_cast<int>(rssi));
        log_bt_banner("DISCOVERED", nullptr, name);
#endif
        return;
    }
#if defined(CONFIG_OGXM_DEBUG)
    if (name && std::strstr(name, "Wireless Controller")) {
        OGXM_LOG("[CYCLONE2 BT] note shared DS4-style name=\"Wireless Controller\" "
                 "(generic Bluepad32 DS4 path; OGX XInput output still available)\n");
    }
#else
    (void)name;
#endif
}

extern "C" void gamesir_cyclone2_bt_on_connected(uni_hid_device_t* device) {
    if (gamesir_cyclone2_bt_reject_game_pair_mode_if_needed(device)) {
        return;
    }
    if (is_gamesir_cyclone_name(device ? device->name : nullptr)) {
        note_bt_cyclone_known("strong name connect");
    }
    if (!gamesir_cyclone2_bt_is_strong_candidate(device) &&
        !(device && device->name[0] && std::strcmp(device->name, "Pro Controller") == 0 &&
          bt_cyclone_known_)) {
        return;
    }
    log_bt_banner("CONNECTED", device, device ? device->name : nullptr);
    OGXM_LOG("[CYCLONE2 BT] supported BT transport — use DS4/HID/Switch (not XInput over BT)\n");
}

extern "C" void gamesir_cyclone2_bt_on_ready(uni_hid_device_t* device) {
    if (!device) {
        return;
    }
    if (gamesir_cyclone2_bt_is_game_pair_mode(device->name)) {
        note_bt_cyclone_known("Game Pair Mode ready");
        log_game_pair_mode_ignored("ready");
        return;
    }
    if (is_gamesir_cyclone_name(device->name)) {
        note_bt_cyclone_known("strong name ready");
    }

    /* Same final XInput face swap as wired/dongle Switch — Cyclone Xbox layout over Switch protocol. */
    arm_bt_switch_face_swap_if_needed(device);

    if (!gamesir_cyclone2_bt_is_strong_candidate(device) && !bt_switch_face_swap_armed_) {
        return;
    }
    log_bt_banner("READY", device, device->name);
    if (bt_switch_face_swap_armed_) {
        OGXM_LOG("[CYCLONE2 BT] Switch ready — Bluepad32 Switch parser + Cyclone face map\n");
    } else {
        OGXM_LOG("[CYCLONE2 BT] ready — Bluepad32 DS4/HID/Switch parser; output via OGX device driver\n");
    }
}

extern "C" void gamesir_cyclone2_bt_on_disconnected(uni_hid_device_t* device) {
    if (!device) {
        clear_bt_switch_face_swap_if_armed();
        return;
    }
    if (gamesir_cyclone2_bt_is_game_pair_mode(device->name)) {
        OGXM_LOG("[CYCLONE2 BT] Game Pair Mode cleaned up\n");
        return;
    }
    clear_bt_switch_face_swap_if_armed();
    if (!gamesir_cyclone2_bt_is_strong_candidate(device) &&
        !(device->name[0] && std::strcmp(device->name, "Pro Controller") == 0)) {
        return;
    }
    log_bt_banner("DISCONNECTED", device, device->name);
}
