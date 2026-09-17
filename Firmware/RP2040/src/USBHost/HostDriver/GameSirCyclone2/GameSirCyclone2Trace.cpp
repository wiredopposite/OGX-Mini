#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Trace.h"

#include <cstdio>
#include <cstring>

#include "host/usbh.h"
#include "pico/time.h"

#include "Board/Config.h"
#include "Board/ogxm_log.h"

#if defined(CONFIG_OGXM_DEBUG)

namespace GameSirCyclone2Trace {
namespace {

constexpr uint16_t kGameSirVid = 0x3537;
constexpr uint16_t kNintendoVid = 0x057E;
constexpr uint16_t kSonyVid = 0x054C;

uint32_t cable_attach_ms_ = 0;
uint32_t enum_seq_ = 0;

uint16_t prev_vid_ = 0;
uint16_t prev_pid_ = 0;
Mode prev_mode_ = Mode::Unknown;
bool saw_cyclone_xinput_ = false;
uint32_t cyclone_xinput_seen_ms_ = 0;
uint32_t last_detach_ms_ = 0;
uint8_t hid_passive_safety_addr_ = 0;
bool saw_0575_ = false;
uint32_t saw_0575_ms_ = 0;
/** Sticky physical Cyclone ownership across USB personality remounts (wired + receiver). */
bool physical_affinity_ = false;
uint16_t cached_bcd_ = 0;
uint8_t last_detach_rhport_ = 0;
uint16_t last_detach_vid_ = 0;
uint16_t last_detach_pid_ = 0;
bool last_detach_had_affinity_ = false;
bool cyclone_root_reset_requested_ = false; /* Cyclone never requests root reset. */
GameSirCyclone2Transport::ReceiverState receiver_state_ =
    GameSirCyclone2Transport::ReceiverState::None;
GameSirCyclone2Transport::Transport last_transport_ =
    GameSirCyclone2Transport::Transport::Unknown;

void arm_physical_affinity(const char* reason)
{
    (void)reason;
    /* RAM-only — never log/flash/USB/BT from here (may run near USB callbacks). */
    physical_affinity_ = true;
}

void log_switch_driver_selection(uint8_t address, uint16_t vid, uint16_t pid,
                                 const char* product, uint16_t bcd,
                                 const char* fingerprint_label, bool matched)
{
    const auto transport = infer_usb_transport(vid, pid);
    OGXM_LOG("[CYCLONE2 RXR] select addr=%u %04X:%04X bcd=%04X product=\"%s\" "
             "transport=%s recv=%s fp=%s → %s\n",
             static_cast<unsigned>(address), vid, pid, bcd,
             product ? product : "",
             GameSirCyclone2Transport::transport_name(transport),
             saw_0575_ ? "YES" : "NO",
             fingerprint_label ? fingerprint_label : "?",
             matched ? "GAMESIR_CYCLONE2" : "SWITCH_PRO");
}

/** Presentation A — bcd 0x0326 is Cyclone-unique (genuine Pro ≈ 0x0200/0x0210). */
bool is_cyclone2_switch_presentation_a(const char* product, uint16_t bcd)
{
    (void)product;
    return bcd == 0x0326;
}

/**
 * Presentation B — bcd 0x0116 observed on Cyclone Switch/dongle.
 * Do not require product string (sync string fetch is unsafe in USB callbacks).
 */
bool is_cyclone2_switch_presentation_b(const char* product, uint16_t bcd)
{
    (void)product;
    return bcd == 0x0116;
}

void utf16le_to_ascii(const uint8_t* buf, uint16_t buflen, char* out, size_t out_len)
{
    if (!out || out_len == 0)
        return;
    out[0] = '\0';
    if (!buf || buflen < 2)
        return;
    const uint8_t str_len = buf[0];
    if (str_len < 2 || buf[1] != 0x03)
        return;
    size_t o = 0;
    for (uint8_t i = 2; i + 1 < str_len && i + 1 < buflen && o + 1 < out_len; i += 2) {
        const uint8_t lo = buf[i];
        const uint8_t hi = buf[i + 1];
        out[o++] = (hi == 0 && lo >= 0x20 && lo < 0x7F) ? static_cast<char>(lo) : '?';
    }
    out[o] = '\0';
}

void fetch_string(uint8_t daddr, uint8_t index, char* out, size_t out_len)
{
    out[0] = '\0';
    if (!index)
        return;
    uint8_t buf[64]{};
    if (tuh_descriptor_get_string_sync(daddr, index, 0x0409, buf, sizeof(buf)) != XFER_RESULT_SUCCESS)
        return;
    utf16le_to_ascii(buf, sizeof(buf), out, out_len);
}

const char* xfer_type_name(uint8_t attr)
{
    switch (attr & 0x03u) {
        case 0: return "CTRL";
        case 1: return "ISO";
        case 2: return "BULK";
        case 3: return "INT";
        default: return "?";
    }
}

void dump_interfaces_brief(uint8_t daddr)
{
    uint8_t cfg[512]{};
    if (tuh_descriptor_get_configuration_sync(daddr, 0, cfg, sizeof(cfg)) != XFER_RESULT_SUCCESS) {
        OGXM_LOG("configuration descriptor fetch failed\n");
        return;
    }

    auto const* conf = reinterpret_cast<tusb_desc_configuration_t const*>(cfg);
    const uint16_t total = tu_le16toh(conf->wTotalLength);
    const uint16_t len = TU_MIN(total, static_cast<uint16_t>(sizeof(cfg)));

    OGXM_LOG("Configuration: %u\n", conf->bConfigurationValue);
    OGXM_LOG("Interface count: %u\n", conf->bNumInterfaces);
    OGXM_LOG("wTotalLength: %u\n", total);

    uint8_t const* p = tu_desc_next(cfg);
    uint8_t const* end = cfg + len;
    while (p < end && tu_desc_type(p) != 0) {
        const uint8_t dtype = tu_desc_type(p);
        const uint8_t dlen = tu_desc_len(p);
        if (dlen < 2 || p + dlen > end)
            break;

        if (dtype == TUSB_DESC_INTERFACE) {
            auto const* itf = reinterpret_cast<tusb_desc_interface_t const*>(p);
            OGXM_LOG("\nInterface %u: class=0x%02X sub=0x%02X proto=0x%02X eps=%u\n",
                     itf->bInterfaceNumber, itf->bInterfaceClass, itf->bInterfaceSubClass,
                     itf->bInterfaceProtocol, itf->bNumEndpoints);
        } else if (dtype == TUSB_DESC_ENDPOINT) {
            auto const* ep = reinterpret_cast<tusb_desc_endpoint_t const*>(p);
            const uint8_t addr = ep->bEndpointAddress;
            OGXM_LOG("  endpoint 0x%02X %s type=%s max=%u interval=%u\n",
                     addr, (addr & TUSB_DIR_IN_MASK) ? "IN" : "OUT",
                     xfer_type_name(static_cast<uint8_t>(ep->bmAttributes.xfer)),
                     static_cast<unsigned>(tu_edpt_packet_size(ep)),
                     static_cast<unsigned>(ep->bInterval));
        }
        p = tu_desc_next(p);
    }
}

} // namespace

bool is_xinput_candidate(uint16_t vid, uint16_t pid)
{
    return vid == kGameSirVid && (pid == 0x100B || pid == 0x1053);
}

Mode infer_mode(uint16_t vid, uint16_t pid)
{
    if (is_xinput_candidate(vid, pid))
        return Mode::XInput;
    if (vid == kNintendoVid && pid == 0x2009)
        return Mode::Switch;
    if (vid == kSonyVid && pid == 0x09CC)
        return Mode::Ds4;
    if (vid == kGameSirVid && pid == 0x0575)
        return Mode::Hid;
    if (vid == kGameSirVid)
        return Mode::OtherGameSir;
    return Mode::Unknown;
}

const char* mode_name(Mode m)
{
    switch (m) {
        case Mode::XInput: return "XINPUT";
        case Mode::Switch: return "SWITCH";
        case Mode::Ds4: return "DS4";
        case Mode::Hid: return "HID";
        case Mode::OtherGameSir: return "GAMESIR_OTHER";
        default: return "UNKNOWN";
    }
}

const char* led_expected(Mode m)
{
    switch (m) {
        case Mode::XInput: return "GREEN";
        case Mode::Switch: return "RED";
        case Mode::Ds4: return "BLUE";
        case Mode::Hid: return "YELLOW";
        default: return "?";
    }
}

uint32_t cable_attach_ms()
{
    return cable_attach_ms_;
}

bool cyclone_session_active()
{
    return saw_cyclone_xinput_ || physical_affinity_;
}

void note_cyclone_xinput_seen()
{
    if (!saw_cyclone_xinput_) {
        saw_cyclone_xinput_ = true;
        cyclone_xinput_seen_ms_ = to_ms_since_boot(get_absolute_time());
    }
    arm_physical_affinity("XInput personality");
}

bool should_own_switch_ns(uint16_t vid, uint16_t pid)
{
    /* Wired: after green XInput. Dongle: after 3537:0575 / sticky physical affinity. */
    return (saw_cyclone_xinput_ || saw_0575_ || physical_affinity_) &&
           vid == kNintendoVid && pid == 0x2009;
}

bool looks_like_cyclone_switch_ns(uint8_t address, uint16_t vid, uint16_t pid)
{
    if (vid != kNintendoVid || pid != 0x2009) {
        return false;
    }

    /* Receiver / prior Cyclone session: RAM affinity only — no USB I/O. */
    if (saw_0575_ || saw_cyclone_xinput_ || physical_affinity_) {
        arm_physical_affinity("receiver/session Switch remount");
        OGXM_LOG("[CYCLONE2 RXR] claim via affinity addr=%u\n", static_cast<unsigned>(address));
        return true;
    }

    /*
     * Cold Switch fingerprint: ONE device-descriptor sync for bcd only.
     * Never fetch strings/config here — nested sync + OGXM_LOG mutex hangs pre-plug boot.
     * Known Cyclone bcds: 0x0326 (A), 0x0116 (B). Genuine Pro ≈ 0x0200/0x0210.
     */
    uint16_t bcd = cached_bcd_;
    if (bcd == 0) {
        tusb_desc_device_t desc{};
        if (tuh_descriptor_get_device_sync(address, &desc, sizeof(desc)) != XFER_RESULT_SUCCESS) {
            OGXM_LOG("[CYCLONE2 RXR] fingerprint desc fail addr=%u → SWITCH_PRO\n",
                     static_cast<unsigned>(address));
            return false;
        }
        bcd = tu_le16toh(desc.bcdDevice);
        cached_bcd_ = bcd;
    }

    const bool match_a = is_cyclone2_switch_presentation_a(nullptr, bcd);
    const bool match_b = is_cyclone2_switch_presentation_b(nullptr, bcd);
    if (match_a || match_b) {
        const char* label = match_a ? "bcd0326" : "bcd0116";
        arm_physical_affinity(label);
        OGXM_LOG("[CYCLONE2 RXR] fingerprint MATCH addr=%u bcd=%04X (%s)\n",
                 static_cast<unsigned>(address), bcd, label);
        return true;
    }

    OGXM_LOG("[CYCLONE2 RXR] fingerprint miss addr=%u bcd=%04X → SWITCH_PRO\n",
             static_cast<unsigned>(address), bcd);
    return false;
}

void log_switch_driver_selection_banner(uint8_t address, uint16_t vid, uint16_t pid)
{
    const char* label = "affinity";
    if (cached_bcd_ == 0x0326) {
        label = "bcd0326";
    } else if (cached_bcd_ == 0x0116) {
        label = "bcd0116";
    } else if (saw_0575_) {
        label = "receiver";
    }
    log_switch_driver_selection(address, vid, pid, "", cached_bcd_, label, true);
}

bool should_own_ds4(uint16_t vid, uint16_t pid)
{
    return saw_cyclone_xinput_ && vid == kSonyVid && pid == 0x09CC;
}

bool should_own_hid_passive(uint16_t vid, uint16_t pid)
{
    return vid == kGameSirVid && pid == 0x0575;
}

void set_hid_passive_safety(uint8_t address, bool active)
{
    if (active) {
        hid_passive_safety_addr_ = address;
    } else if (hid_passive_safety_addr_ == address) {
        hid_passive_safety_addr_ = 0;
    }
}

bool hid_passive_safety_active(uint8_t address)
{
    return hid_passive_safety_addr_ != 0 && hid_passive_safety_addr_ == address;
}

void log_blocked_hid_out(uint8_t address, uint8_t instance, const char* caller,
                         const uint8_t* data, uint16_t len)
{
    (void)data;
    (void)len;
    OGXM_LOG("[CYCLONE2 RXR] blocked OUT addr=%u inst=%u caller=%s\n",
             static_cast<unsigned>(address), static_cast<unsigned>(instance),
             caller ? caller : "?");
}

bool receiver_session_active()
{
    return saw_0575_;
}

void note_0575_mounted()
{
    saw_0575_ = true;
    saw_0575_ms_ = to_ms_since_boot(get_absolute_time());
    receiver_state_ = GameSirCyclone2Transport::ReceiverState::ControllerOffline;
    last_transport_ = GameSirCyclone2Transport::Transport::UsbReceiver24Ghz;
    arm_physical_affinity("3537:0575");
    OGXM_LOG("[CYCLONE2 RXR] 0575 mounted (idle receiver or HID; no PadIn)\n");
}

void note_controller_personality_mounted(uint16_t vid, uint16_t pid)
{
    (void)vid;
    (void)pid;
    if (saw_0575_) {
        receiver_state_ = GameSirCyclone2Transport::ReceiverState::ControllerOnline;
        last_transport_ = GameSirCyclone2Transport::Transport::UsbReceiver24Ghz;
        OGXM_LOG("[CYCLONE2 2.4GHZ] controller personality online after 0575 session "
                 "(reuse wired protocol engine)\n");
    } else {
        last_transport_ = GameSirCyclone2Transport::Transport::Wired;
        receiver_state_ = GameSirCyclone2Transport::ReceiverState::None;
    }
}

GameSirCyclone2Transport::Transport infer_usb_transport(uint16_t vid, uint16_t pid)
{
    using namespace GameSirCyclone2Transport;
    if (vid == kGameSirVid && pid == 0x0575) {
        return Transport::UsbReceiver24Ghz;
    }
    if (saw_0575_) {
        return Transport::UsbReceiver24Ghz;
    }
    if (is_xinput_candidate(vid, pid) || (vid == kNintendoVid && pid == 0x2009) ||
        (vid == kSonyVid && pid == 0x09CC)) {
        return Transport::Wired;
    }
    return last_transport_;
}

GameSirCyclone2Transport::ReceiverState receiver_state()
{
    return receiver_state_;
}

void set_receiver_controller_online(bool online)
{
    if (!saw_0575_) {
        return;
    }
    receiver_state_ = online ? GameSirCyclone2Transport::ReceiverState::ControllerOnline
                             : GameSirCyclone2Transport::ReceiverState::ControllerOffline;
}

void log_unified_banner(uint16_t vid, uint16_t pid, const char* bt_name,
                        bool input_active, const char* protocol_engine)
{
    (void)bt_name;
    using namespace GameSirCyclone2Transport;
    const Transport t = infer_usb_transport(vid, pid);
    OGXM_LOG("[CYCLONE2 RXR] %04X:%04X transport=%s mode=%s input=%s engine=%s\n",
             vid, pid, transport_name(t), mode_name(infer_mode(vid, pid)),
             input_active ? "ACTIVE" : "WAIT",
             protocol_engine ? protocol_engine : "?");
}

void on_bus_attach(uint8_t rhport)
{
    on_bus_attach(rhport, false);
}

void on_bus_attach(uint8_t rhport, bool in_isr)
{
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    /* Mode-switch reattach is often <1s; receiver personality changes can be slower. */
    constexpr uint32_t kXinputSessionClearGapMs = 2000;
    constexpr uint32_t kReceiverAffinityClearGapMs = 15000;
    if (last_detach_ms_ != 0) {
        const uint32_t gap = now - last_detach_ms_;
        if (saw_cyclone_xinput_ && gap >= kXinputSessionClearGapMs) {
            saw_cyclone_xinput_ = false;
            cyclone_xinput_seen_ms_ = 0;
        }
        if ((saw_0575_ || physical_affinity_) && gap >= kReceiverAffinityClearGapMs) {
            saw_0575_ = false;
            saw_0575_ms_ = 0;
            physical_affinity_ = false;
            cached_bcd_ = 0;
            receiver_state_ = GameSirCyclone2Transport::ReceiverState::None;
            last_transport_ = GameSirCyclone2Transport::Transport::Unknown;
        }
    }
    if (cable_attach_ms_ == 0) {
        cable_attach_ms_ = now;
    }
    /* ISR path: RAM timestamps only — OGXM_LOG uses mutex_enter_blocking (unsafe in IRQ). */
    if (in_isr) {
        return;
    }
    OGXM_LOG("[CYCLONE2 RXR] attach t=%lums rhport=%u\n",
             static_cast<unsigned long>(now), static_cast<unsigned>(rhport));
}

void on_bus_remove(uint8_t rhport)
{
    on_bus_remove(rhport, false);
}

void on_bus_remove(uint8_t rhport, bool in_isr)
{
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    last_detach_ms_ = now;
    last_detach_rhport_ = rhport;
    last_detach_vid_ = prev_vid_;
    last_detach_pid_ = prev_pid_;
    last_detach_had_affinity_ = (saw_0575_ || physical_affinity_ || saw_cyclone_xinput_);
    /* Cyclone never requests root-port reset; personality remounts are normal. */
    cyclone_root_reset_requested_ = false;

    if (in_isr) {
        return;
    }
    OGXM_LOG("[USB DETACH] t=%lums rhport=%u addr=(bus) "
             "prev=%04X:%04X affinity=%s root_reset_requested=NO "
             "Cyclone_personality_transition=%s\n",
             static_cast<unsigned long>(now), static_cast<unsigned>(rhport),
             last_detach_vid_, last_detach_pid_,
             last_detach_had_affinity_ ? "YES" : "NO",
             last_detach_had_affinity_ ? "YES" : "NO");
}

void on_device_configured(uint8_t address)
{
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    ++enum_seq_;

    /* No *_sync here — mount callback re-entering control xfer nests tuh_task and hangs boot. */
    uint16_t vid = 0, pid = 0;
    (void)tuh_vid_pid_get(address, &vid, &pid);
    const Mode mode = infer_mode(vid, pid);

    /*
     * Cyclone receiver tracing must not run for unrelated pads (e.g. Victrix Gambit 0E6F:0250).
     * Only GameSir VID family, or an already-armed Cyclone affinity/session, continue.
     */
    const bool gamesir_family = (vid == kGameSirVid);
    const bool cyclone_session = (saw_0575_ || physical_affinity_ || saw_cyclone_xinput_);
    if (!gamesir_family && !cyclone_session)
    {
        return;
    }

    if (is_xinput_candidate(vid, pid)) {
        saw_cyclone_xinput_ = true;
        cyclone_xinput_seen_ms_ = now;
        arm_physical_affinity("XInput enum");
    }
    if (vid == kGameSirVid && pid == 0x0575) {
        saw_0575_ = true;
        saw_0575_ms_ = now;
        arm_physical_affinity("0575 enum");
        last_transport_ = GameSirCyclone2Transport::Transport::UsbReceiver24Ghz;
    }

    OGXM_LOG("[CYCLONE2 RXR] enum #%lu addr=%u %04X:%04X mode=%s affinity=%s\n",
             static_cast<unsigned long>(enum_seq_), static_cast<unsigned>(address),
             vid, pid, mode_name(mode),
             (saw_0575_ || physical_affinity_ || saw_cyclone_xinput_) ? "YES" : "NO");

    if ((saw_cyclone_xinput_ || physical_affinity_) && mode == Mode::Unknown) {
        saw_cyclone_xinput_ = false;
        cyclone_xinput_seen_ms_ = 0;
        physical_affinity_ = false;
        saw_0575_ = false;
        saw_0575_ms_ = 0;
        cached_bcd_ = 0;
    }

    prev_vid_ = vid;
    prev_pid_ = pid;
    prev_mode_ = mode;
}

void on_device_unmounted(uint8_t address)
{
    uint16_t vid = 0, pid = 0;
    (void)tuh_vid_pid_get(address, &vid, &pid);
    const bool gamesir_family = (vid == kGameSirVid);
    const bool cyclone_session = (saw_0575_ || physical_affinity_ || saw_cyclone_xinput_);
    if (!gamesir_family && !cyclone_session &&
        !(prev_vid_ == kGameSirVid || saw_0575_ || physical_affinity_))
    {
        if (hid_passive_safety_addr_ == address) {
            set_hid_passive_safety(address, false);
        }
        return;
    }

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    OGXM_LOG("[CYCLONE2 RXR] umount t=%lums addr=%u prev=%04X:%04X "
             "root_reset_requested=%s personality_transition=%s\n",
             static_cast<unsigned long>(now), static_cast<unsigned>(address),
             prev_vid_, prev_pid_,
             cyclone_root_reset_requested_ ? "YES" : "NO",
             (saw_0575_ || physical_affinity_) ? "YES" : "NO");
    last_detach_ms_ = now;
    if (hid_passive_safety_addr_ == address) {
        set_hid_passive_safety(address, false);
    }
}

void on_xinput_claimed(uint8_t address, uint8_t instance, uint8_t itf_num,
                       uint8_t ep_in, uint8_t ep_out)
{
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    /* Only log for GameSir Cyclone XInput IDs — not every Xbox GIP pad. */
    if (!is_xinput_candidate(vid, pid))
    {
        return;
    }
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    const uint32_t rel = cable_attach_ms_ ? (now - cable_attach_ms_) : 0;

    OGXM_LOG("\n[CYCLONE2 MODE TRACE] XInput interface claimed\n");
    OGXM_LOG("t=%lums (cable+%lums) VID=%04X PID=%04X\n",
             static_cast<unsigned long>(now), static_cast<unsigned long>(rel), vid, pid);
    OGXM_LOG("instance=%u itf=%u ep_in=0x%02X ep_out=0x%02X\n",
             static_cast<unsigned>(instance), static_cast<unsigned>(itf_num), ep_in, ep_out);
}

void log_host_tx(uint8_t address, const char* kind, uint8_t ep,
                 const uint8_t* data, uint16_t len)
{
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    if (!is_xinput_candidate(vid, pid) && !(saw_cyclone_xinput_ && prev_mode_ == Mode::XInput))
        return;

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    OGXM_LOG("[CYCLONE2 USB TX] t=%lums type=%s ep=0x%02X len=%u\n",
             static_cast<unsigned long>(now), kind ? kind : "?", ep,
             static_cast<unsigned>(len));
    if (data && len) {
        char line[96]{};
        size_t o = 0;
        const uint16_t n = (len > 16) ? 16 : len;
        for (uint16_t i = 0; i < n && o + 4 < sizeof(line); ++i)
            o += static_cast<size_t>(snprintf(line + o, sizeof(line) - o, "%02X ", data[i]));
        OGXM_LOG("  payload: %s%s\n", line, (len > 16) ? "..." : "");
    }
}

void log_host_ctrl(uint8_t address, uint8_t bm_req, uint8_t b_req,
                   uint16_t w_value, uint16_t w_index, uint16_t w_len)
{
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    if (!is_xinput_candidate(vid, pid))
        return;

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    OGXM_LOG("[CYCLONE2 USB TX] t=%lums type=control bm=0x%02X req=0x%02X "
             "wValue=0x%04X wIndex=0x%04X wLen=%u\n",
             static_cast<unsigned long>(now), bm_req, b_req, w_value, w_index,
             static_cast<unsigned>(w_len));
}

} // namespace GameSirCyclone2Trace

#else // !CONFIG_OGXM_DEBUG

namespace GameSirCyclone2Trace {
void on_bus_attach(uint8_t) {}
void on_bus_remove(uint8_t) {}
void on_bus_attach(uint8_t, bool) {}
void on_bus_remove(uint8_t, bool) {}
void on_device_configured(uint8_t) {}
void on_device_unmounted(uint8_t) {}
void on_xinput_claimed(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t) {}
void log_host_tx(uint8_t, const char*, uint8_t, const uint8_t*, uint16_t) {}
void log_host_ctrl(uint8_t, uint8_t, uint8_t, uint16_t, uint16_t, uint16_t) {}
bool is_xinput_candidate(uint16_t, uint16_t) { return false; }
Mode infer_mode(uint16_t, uint16_t) { return Mode::Unknown; }
const char* mode_name(Mode) { return "UNKNOWN"; }
const char* led_expected(Mode) { return "?"; }
uint32_t cable_attach_ms() { return 0; }
bool cyclone_session_active() { return false; }
void note_cyclone_xinput_seen() {}
bool should_own_switch_ns(uint16_t, uint16_t) { return false; }
bool looks_like_cyclone_switch_ns(uint8_t, uint16_t, uint16_t) { return false; }
void log_switch_driver_selection_banner(uint8_t, uint16_t, uint16_t) {}
bool should_own_ds4(uint16_t, uint16_t) { return false; }
bool should_own_hid_passive(uint16_t, uint16_t) { return false; }
void set_hid_passive_safety(uint8_t, bool) {}
bool hid_passive_safety_active(uint8_t) { return false; }
void log_blocked_hid_out(uint8_t, uint8_t, const char*, const uint8_t*, uint16_t) {}
bool receiver_session_active() { return false; }
GameSirCyclone2Transport::Transport infer_usb_transport(uint16_t, uint16_t) {
    return GameSirCyclone2Transport::Transport::Unknown;
}
GameSirCyclone2Transport::ReceiverState receiver_state() {
    return GameSirCyclone2Transport::ReceiverState::None;
}
void set_receiver_controller_online(bool) {}
void note_0575_mounted() {}
void note_controller_personality_mounted(uint16_t, uint16_t) {}
void log_unified_banner(uint16_t, uint16_t, const char*, bool, const char*) {}
} // namespace GameSirCyclone2Trace

#endif // CONFIG_OGXM_DEBUG

/* Always linked — XInput final face-swap gate (not debug-only). */
namespace GameSirCyclone2Trace {
namespace {
volatile bool g_cyclone_switch_input_active = false;
}
void set_cyclone_switch_input_active(bool active) {
    g_cyclone_switch_input_active = active;
}
bool cyclone_switch_input_active() {
    return g_cyclone_switch_input_active;
}
} // namespace GameSirCyclone2Trace
