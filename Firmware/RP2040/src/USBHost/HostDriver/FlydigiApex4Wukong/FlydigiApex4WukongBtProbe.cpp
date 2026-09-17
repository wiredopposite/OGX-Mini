#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBtProbe.h"
#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBt.h"
#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBtL2cap.h"

#include <cstdio>
#include <cstring>

#include <btstack.h>
#include "gap.h"

#include "Board/ogxm_log.h"
#include "sdkconfig.h"

namespace {

constexpr uint16_t kMsVid = 0x045E;
constexpr uint16_t kXboxOneSBtPid = 0x02E0;
constexpr char kXboxWirelessName[] = "Xbox Wireless Controller";

constexpr unsigned kMaxSlots = CONFIG_BLUEPAD32_MAX_DEVICES;

bool s_banner_done[kMaxSlots]{};

bool name_is_xbox_wireless(const char* name)
{
    return name != nullptr && name[0] != '\0' &&
           std::strcmp(name, kXboxWirelessName) == 0;
}

int device_slot(const uni_hid_device_t* d)
{
    if (!d)
        return -1;
    const int idx = uni_hid_device_get_idx_for_instance(const_cast<uni_hid_device_t*>(d));
    if (idx < 0 || idx >= static_cast<int>(kMaxSlots))
        return -1;
    return idx;
}

#if defined(CONFIG_OGXM_DEBUG)

const char* conn_protocol_name(uni_bt_conn_protocol_t p)
{
    switch (p) {
        case UNI_BT_CONN_PROTOCOL_BR_EDR: return "BR/EDR (Classic)";
        case UNI_BT_CONN_PROTOCOL_BLE: return "BLE (HOGP)";
        default: return "unknown";
    }
}

const char* gap_conn_type_name(gap_connection_type_t t)
{
    switch (t) {
        case GAP_CONNECTION_ACL: return "ACL";
        case GAP_CONNECTION_LE: return "LE";
        case GAP_CONNECTION_SCO: return "SCO";
        default: return "?";
    }
}

void log_hex_block(const char* title, const uint8_t* data, uint16_t len)
{
    OGXM_LOG("%s\n", title);
    if (!data || len == 0) {
        OGXM_LOG("(empty)\n");
        return;
    }

    char line[3 * 16 + 8]{};
    uint16_t col = 0;
    size_t o = 0;
    for (uint16_t i = 0; i < len; ++i) {
        if (col == 0)
            o = static_cast<size_t>(std::snprintf(line, sizeof(line), "%04x:", i));
        o += static_cast<size_t>(std::snprintf(line + o, sizeof(line) - o, " %02x", data[i]));
        ++col;
        if (col == 16 || i + 1 == len) {
            OGXM_LOG("%s\n", line);
            col = 0;
            o = 0;
        }
    }
}

void summarize_hid_descriptor(const uint8_t* desc, uint16_t desc_len)
{
    if (!desc || desc_len == 0) {
        OGXM_LOG("HID descriptor: (none yet)\n");
        return;
    }

    OGXM_LOG("HID Descriptor Length: %u\n", static_cast<unsigned>(desc_len));
    log_hex_block("[APEX4 BT HID REPORT DESCRIPTOR]", desc, desc_len);
}

void dump_probe_banner(uni_hid_device_t* d)
{
    if (!d)
        return;

    const int slot = device_slot(d);
    char addr_str[18]{};
    std::snprintf(addr_str, sizeof(addr_str), "%s", bd_addr_to_str(d->conn.btaddr));

    gap_connection_type_t gap_type = GAP_CONNECTION_INVALID;
    if (d->conn.handle != UNI_BT_CONN_HANDLE_INVALID)
        gap_type = gap_get_connection_type(d->conn.handle);

    const bool ble = (d->hids_cid != 0 && d->hids_cid != 0xffff) ||
                     d->conn.protocol == UNI_BT_CONN_PROTOCOL_BLE ||
                     gap_type == GAP_CONNECTION_LE;

    OGXM_LOG("\n");
    OGXM_LOG("====================================================\n");
    OGXM_LOG("FLYDIGI APEX 4 WUKONG — PC BLUETOOTH\n");
    OGXM_LOG("====================================================\n");
    OGXM_LOG("APEX 4 WUKONG BLUETOOTH CANDIDATE (stage-1; parser not final until layout+SDP)\n");
    OGXM_LOG("Name: %s\n", d->name[0] ? d->name : "(none)");
    OGXM_LOG("Address: %s\n", addr_str);
    OGXM_LOG("Link: %s (gap=%s protocol=%s) slot=%d\n",
             ble ? "LE/HOGP" : "Classic",
             gap_conn_type_name(gap_type),
             conn_protocol_name(d->conn.protocol), slot);
    OGXM_LOG("VID: %04X  PID: %04X  COD: 0x%06lx\n",
             d->vendor_id, d->product_id, static_cast<unsigned long>(d->cod));
    summarize_hid_descriptor(d->hid_descriptor, d->hid_descriptor_len);
    OGXM_LOG("layout_confirmed=%d parser_installed=%d\n",
             flydigi_apex4_bt_layout_confirmed(d), flydigi_apex4_bt_parser_installed(d));
    OGXM_LOG("====================================================\n");
    uni_hid_device_dump_device(d);
}

#endif /* CONFIG_OGXM_DEBUG */

} // namespace

extern "C" int flydigi_apex4_bt_is_candidate(const uni_hid_device_t* device)
{
    if (!device)
        return 0;
    /* Identity hints only — final claim requires runtime 16-byte Report 0x01. */
    if (device->vendor_id == kMsVid && device->product_id == kXboxOneSBtPid)
        return 1;
    /* Name alone is only a candidate hint; final claim needs Report 0x01 / len 16. */
    if (name_is_xbox_wireless(device->name))
        return 1;
    return 0;
}

extern "C" void flydigi_apex4_bt_on_discovered(const uint8_t* bd_addr, const char* name, uint16_t cod, uint8_t rssi)
{
    if (!name_is_xbox_wireless(name))
        return;
#if defined(CONFIG_OGXM_DEBUG)
    char addr_str[18] = "(unknown)";
    if (bd_addr)
        std::snprintf(addr_str, sizeof(addr_str), "%s", bd_addr_to_str(const_cast<uint8_t*>(bd_addr)));
    OGXM_LOG("[APEX4 BT] discovered candidate name=\"%s\" addr=%s COD=0x%04x RSSI=%u\n",
             name ? name : "", addr_str, static_cast<unsigned>(cod), static_cast<unsigned>(rssi));
#else
    (void)bd_addr;
    (void)cod;
    (void)rssi;
#endif
}

extern "C" void flydigi_apex4_bt_on_connected(uni_hid_device_t* device)
{
    if (!flydigi_apex4_bt_is_candidate(device))
        return;
    flydigi_apex4_bt_mark_candidate(device);
#if defined(CONFIG_OGXM_DEBUG)
    OGXM_LOG("[APEX4 BT] connected candidate name=\"%s\" VID=%04X PID=%04X (probe only — no final parser yet)\n",
             device->name[0] ? device->name : "", device->vendor_id, device->product_id);
#endif
}

extern "C" void flydigi_apex4_bt_on_ready(uni_hid_device_t* device)
{
    if (!flydigi_apex4_bt_is_candidate(device))
        return;

    flydigi_apex4_bt_mark_candidate(device);
    /* Last-chance finalize if layout was confirmed before READY. */
    flydigi_apex4_bt_on_device_ready_transition(device);

    const int slot = device_slot(device);
#if defined(CONFIG_OGXM_DEBUG)
    if (slot >= 0 && !s_banner_done[slot]) {
        dump_probe_banner(device);
        s_banner_done[slot] = true;
    } else if (slot >= 0) {
        OGXM_LOG("[APEX4 BT] ready again (reconnect) slot=%d parser=%s\n", slot,
                 flydigi_apex4_bt_parser_installed(device) ? "APEX4_WUKONG_BT" : "NOT_APEX");
    }
#else
    (void)slot;
#endif
}

extern "C" void flydigi_apex4_bt_on_disconnected(uni_hid_device_t* device)
{
    const int slot = device_slot(device);
#if defined(CONFIG_OGXM_DEBUG)
    if (flydigi_apex4_bt_is_candidate(device))
        OGXM_LOG("[APEX4 BT] disconnected candidate slot=%d\n", slot);
#endif
    flydigi_apex4_bt_on_slot_disconnected(device);
    flydigi_apex4_bt_l2cap_gate_on_disconnect(device);
    if (slot >= 0)
        s_banner_done[slot] = false;
}

extern "C" void flydigi_apex4_bt_on_raw_report(uni_hid_device_t* device, const uint8_t* report, uint16_t len)
{
    /*
     * Stage 1 only: observe packets / confirm layout. Do NOT install the
     * final parser here — Bluepad32 SDP/generic selection still runs later.
     */
    flydigi_apex4_bt_note_raw_report(device, report, len);
}
