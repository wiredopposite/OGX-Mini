#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBtL2cap.h"
#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBtProbe.h"
#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBt.h"

#include <cstdint>
#include <cstring>

#include <btstack.h>
#include "btstack_event.h"
#include "btstack_run_loop.h"
#include "gap.h"
#include "hci.h"
#include "l2cap.h"

#include "Board/ogxm_log.h"
#include "bt/uni_bt.h"
#include "bt/uni_bt_conn.h"
#include "bt/uni_bt_defines.h"
#include "sdkconfig.h"
#include "uni_hid_device.h"

namespace {

constexpr unsigned kMaxSlots = CONFIG_BLUEPAD32_MAX_DEVICES;
constexpr unsigned kMaxAttempts = 3;
constexpr uint32_t kSettleAfterEncMs = 250;
constexpr uint32_t kRetryDelayMs[kMaxAttempts] = {250, 350, 500};

struct SlotGate {
    bool want_hid_control{false};
    bool acl_connected{false};
    bool authenticated{false};
    bool encrypted{false};
    bool acl_connect_issued{false};
    bool auth_kick_issued{false};
    bool hid_control_pending{false};
    bool settle_timer_armed{false};
    uint8_t attempt{0}; /* 0 = not started; 1..3 = open attempts */
    uint32_t encryption_ready_ms{0};
    hci_con_handle_t handle{HCI_CON_HANDLE_INVALID};
    btstack_timer_source_t settle_timer{};
};

SlotGate s_gate[kMaxSlots]{};
btstack_packet_callback_registration_t s_hci_cb{};
bool s_inited{false};

int device_slot(const uni_hid_device_t* d)
{
    if (!d)
        return -1;
    const int idx = uni_hid_device_get_idx_for_instance(const_cast<uni_hid_device_t*>(d));
    if (idx < 0 || idx >= static_cast<int>(kMaxSlots))
        return -1;
    return idx;
}

uint32_t now_ms()
{
    return btstack_run_loop_get_time_ms();
}

const char* l2cap_status_meaning(uint8_t status)
{
    switch (status) {
        case 0x00: return "SUCCESS";
        case L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_PSM: return "REFUSED_PSM";
        case L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_SECURITY: return "REFUSED_SECURITY";
        case L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_RESOURCES: return "REFUSED_RESOURCES";
        case L2CAP_CONNECTION_RESPONSE_RESULT_ERTM_NOT_SUPPORTED: return "ERTM_NOT_SUPPORTED";
        case L2CAP_CONNECTION_RESPONSE_RESULT_RTX_TIMEOUT: return "RTX_TIMEOUT";
        default: return "OTHER";
    }
}

bool link_key_present(const bd_addr_t addr)
{
    link_key_t key{};
    link_key_type_t type = static_cast<link_key_type_t>(0xff);
    const bool have = gap_get_link_key_for_bd_addr(const_cast<uint8_t*>(addr), key, &type);
    std::memset(key, 0, sizeof(key));
    return have;
}

bool handle_alive(hci_con_handle_t handle)
{
    return handle != HCI_CON_HANDLE_INVALID && handle != UNI_BT_CONN_HANDLE_INVALID &&
           gap_get_connection_type(handle) != GAP_CONNECTION_INVALID;
}

void refresh_security_from_stack(uni_hid_device_t* d, SlotGate& g)
{
    if (!d || !handle_alive(d->conn.handle))
        return;
    g.handle = d->conn.handle;
    g.acl_connected = true;
    if (gap_authenticated(d->conn.handle))
        g.authenticated = true;
    if (gap_encryption_key_size(d->conn.handle) > 0) {
        if (!g.encrypted) {
            g.encrypted = true;
            g.encryption_ready_ms = now_ms();
        }
    }
    if (gap_security_level(d->conn.handle) >= LEVEL_2) {
        g.authenticated = true;
        if (!g.encrypted) {
            g.encrypted = true;
            g.encryption_ready_ms = now_ms();
        }
    }
}

bool security_ready(const SlotGate& g)
{
    return g.acl_connected && g.authenticated && g.encrypted;
}

void log_security_ready(uni_hid_device_t* d, const SlotGate& g)
{
    OGXM_LOG("\n[APEX4 BT]\n");
    OGXM_LOG("Security ready\n");
    OGXM_LOG("ACL: %s\n", g.acl_connected ? "YES" : "NO");
    OGXM_LOG("Authenticated: %s\n", g.authenticated ? "YES" : "NO");
    OGXM_LOG("Encrypted: %s\n", g.encrypted ? "YES" : "NO");
    OGXM_LOG("handle=0x%04x addr=%s\n", g.handle, d ? bd_addr_to_str(d->conn.btaddr) : "?");
}

void do_open_hid_control(uni_hid_device_t* d, int slot);
void maybe_progress(uni_hid_device_t* d, int slot);

void settle_timer_cb(btstack_timer_source_t* ts)
{
    const int slot = static_cast<int>(reinterpret_cast<uintptr_t>(btstack_run_loop_get_timer_context(ts)));
    if (slot < 0 || slot >= static_cast<int>(kMaxSlots))
        return;
    SlotGate& g = s_gate[slot];
    g.settle_timer_armed = false;

    uni_hid_device_t* d = uni_hid_device_get_instance_for_idx(slot);
    if (!d || !flydigi_apex4_bt_is_candidate(d))
        return;
    if (!g.want_hid_control || g.hid_control_pending)
        return;
    if (!security_ready(g) || !handle_alive(d->conn.handle)) {
        OGXM_LOG("[APEX4 BT] settle timer fired but security/ACL not ready — waiting\n");
        return;
    }
    do_open_hid_control(d, slot);
}

void cancel_settle_timer(int slot)
{
    if (slot < 0 || slot >= static_cast<int>(kMaxSlots))
        return;
    SlotGate& g = s_gate[slot];
    if (g.settle_timer_armed) {
        btstack_run_loop_remove_timer(&g.settle_timer);
        g.settle_timer_armed = false;
    }
}

void schedule_open(uni_hid_device_t* d, int slot, uint32_t delay_ms)
{
    SlotGate& g = s_gate[slot];
    if (g.hid_control_pending || g.settle_timer_armed)
        return;

    cancel_settle_timer(slot);
    g.settle_timer.process = settle_timer_cb;
    btstack_run_loop_set_timer_context(&g.settle_timer, reinterpret_cast<void*>(static_cast<uintptr_t>(slot)));
    btstack_run_loop_set_timer(&g.settle_timer, delay_ms);
    btstack_run_loop_add_timer(&g.settle_timer);
    g.settle_timer_armed = true;

    OGXM_LOG("[APEX4 BT] Scheduling HID Control PSM 0x0011 in %lu ms (attempt will be %u)\n",
             static_cast<unsigned long>(delay_ms), static_cast<unsigned>(g.attempt + 1));
    (void)d;
}

void do_open_hid_control(uni_hid_device_t* d, int slot)
{
    SlotGate& g = s_gate[slot];
    if (g.hid_control_pending) {
        OGXM_LOG("[APEX4 BT] HID Control already pending — skip duplicate open\n");
        return;
    }
    if (!d || !handle_alive(d->conn.handle)) {
        OGXM_LOG("[APEX4 BT] cannot open HID Control — ACL gone\n");
        return;
    }

    g.attempt = static_cast<uint8_t>(g.attempt + 1);
    if (g.attempt > kMaxAttempts) {
        OGXM_LOG("[APEX4 BT] HID Control attempts exhausted — disconnect, KEEP bond\n");
        g.want_hid_control = false;
        uni_hid_device_disconnect(d);
        uni_hid_device_delete(d);
        return;
    }

    const uint32_t since_enc =
        (g.encryption_ready_ms != 0) ? (now_ms() - g.encryption_ready_ms) : 0;

    OGXM_LOG("\n[APEX4 BT]\n");
    OGXM_LOG("Opening HID Control\n");
    OGXM_LOG("PSM=0x0011\n");
    OGXM_LOG("attempt=%u\n", static_cast<unsigned>(g.attempt));
    OGXM_LOG("time_since_encryption=%lums\n", static_cast<unsigned long>(since_enc));
    OGXM_LOG("handle=0x%04x\n\n", d->conn.handle);

    g.hid_control_pending = true;
    uint8_t status = l2cap_create_channel(uni_bt_packet_handler, d->conn.btaddr, BLUETOOTH_PSM_HID_CONTROL,
                                          UNI_BT_L2CAP_CHANNEL_MTU, &d->conn.control_cid);
    if (status) {
        OGXM_LOG("[APEX4 BT L2CAP] l2cap_create_channel immediate error=0x%02x\n", status);
        g.hid_control_pending = false;
        if (g.attempt < kMaxAttempts) {
            schedule_open(d, slot, kRetryDelayMs[g.attempt]);
        } else {
            OGXM_LOG("[APEX4 BT] giving up after immediate errors — KEEP bond\n");
            g.want_hid_control = false;
            uni_hid_device_disconnect(d);
            uni_hid_device_delete(d);
        }
        return;
    }
    uni_bt_conn_set_state(&d->conn, UNI_BT_CONN_STATE_L2CAP_CONTROL_CONNECTION_REQUESTED);
}

void maybe_progress(uni_hid_device_t* d, int slot)
{
    if (!d || slot < 0)
        return;
    SlotGate& g = s_gate[slot];
    if (!g.want_hid_control || g.hid_control_pending || g.settle_timer_armed)
        return;

    refresh_security_from_stack(d, g);

    if (!g.acl_connected || !handle_alive(d->conn.handle)) {
        if (!g.acl_connect_issued) {
            OGXM_LOG("[APEX4 BT] Ensuring Classic ACL before HID Control (gap_connect)\n");
            const uint8_t st = gap_connect(d->conn.btaddr, BD_ADDR_TYPE_ACL);
            g.acl_connect_issued = true;
            if (st != ERROR_CODE_SUCCESS && st != ERROR_CODE_COMMAND_DISALLOWED) {
                OGXM_LOG("[APEX4 BT] gap_connect status=0x%02x\n", st);
            }
        }
        return;
    }

    if (!g.authenticated || !g.encrypted) {
        if (!g.auth_kick_issued) {
            OGXM_LOG("[APEX4 BT] ACL up — requesting security LEVEL_2 before HID Control\n");
            gap_request_security_level(d->conn.handle, LEVEL_2);
            g.auth_kick_issued = true;
        }
        OGXM_LOG("[APEX4 BT] Waiting for auth/encryption (auth=%d enc=%d)\n", g.authenticated, g.encrypted);
        return;
    }

    log_security_ready(d, g);

    /* First attempt: 250 ms after encryption. Retries use 350 / 500. */
    uint32_t delay = kSettleAfterEncMs;
    if (g.attempt == 0) {
        const uint32_t elapsed = (g.encryption_ready_ms != 0) ? (now_ms() - g.encryption_ready_ms) : 0;
        delay = (elapsed >= kSettleAfterEncMs) ? 0 : (kSettleAfterEncMs - elapsed);
    } else if (g.attempt < kMaxAttempts) {
        delay = kRetryDelayMs[g.attempt];
    }

    if (delay == 0)
        do_open_hid_control(d, slot);
    else
        schedule_open(d, slot, delay);
}

void on_hci_event(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET)
        return;

    const uint8_t event = hci_event_packet_get_type(packet);
    bd_addr_t addr{};

    switch (event) {
        case HCI_EVENT_CONNECTION_COMPLETE: {
            const uint8_t status = hci_event_connection_complete_get_status(packet);
            hci_event_connection_complete_get_bd_addr(packet, addr);
            const hci_con_handle_t handle = hci_event_connection_complete_get_connection_handle(packet);
            uni_hid_device_t* d = uni_hid_device_get_instance_for_address(addr);
            if (!d || !flydigi_apex4_bt_is_candidate(d) || status != 0)
                break;
            const int slot = device_slot(d);
            if (slot < 0)
                break;
            SlotGate& g = s_gate[slot];
            g.acl_connected = true;
            g.acl_connect_issued = true;
            g.handle = handle;
            flydigi_apex4_bt_mark_candidate(d);
            OGXM_LOG("[APEX4 BT] ACL connected handle=0x%04x\n", handle);
            if (g.want_hid_control)
                maybe_progress(d, slot);
            break;
        }
        case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT: {
            const uint8_t status = hci_event_authentication_complete_get_status(packet);
            const hci_con_handle_t handle = hci_event_authentication_complete_get_connection_handle(packet);
            uni_hid_device_t* d = uni_hid_device_get_instance_for_connection_handle(handle);
            if (!d || !flydigi_apex4_bt_is_candidate(d))
                break;
            const int slot = device_slot(d);
            if (slot < 0)
                break;
            SlotGate& g = s_gate[slot];
            g.handle = handle;
            g.acl_connected = true;
            if (status == 0) {
                g.authenticated = true;
                OGXM_LOG("[APEX4 BT] authenticated=YES handle=0x%04x\n", handle);
            } else {
                OGXM_LOG("[APEX4 BT] AUTHENTICATION_COMPLETE failed status=0x%02x\n", status);
            }
            if (g.want_hid_control)
                maybe_progress(d, slot);
            break;
        }
        case HCI_EVENT_ENCRYPTION_CHANGE:
        case HCI_EVENT_ENCRYPTION_CHANGE_V2: {
            const uint8_t status = hci_event_encryption_change_get_status(packet);
            const hci_con_handle_t handle = hci_event_encryption_change_get_connection_handle(packet);
            const uint8_t enc = hci_event_encryption_change_get_encryption_enabled(packet);
            uni_hid_device_t* d = uni_hid_device_get_instance_for_connection_handle(handle);
            if (!d || !flydigi_apex4_bt_is_candidate(d))
                break;
            const int slot = device_slot(d);
            if (slot < 0)
                break;
            SlotGate& g = s_gate[slot];
            g.handle = handle;
            g.acl_connected = true;
            if (status == 0 && enc) {
                g.encrypted = true;
                g.encryption_ready_ms = now_ms();
                /* Bonded reconnects often encrypt without a separate auth-complete race. */
                if (gap_authenticated(handle) || gap_security_level(handle) >= LEVEL_2)
                    g.authenticated = true;
                OGXM_LOG("[APEX4 BT] encrypted=YES handle=0x%04x\n", handle);
            }
            if (g.want_hid_control)
                maybe_progress(d, slot);
            break;
        }
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            const hci_con_handle_t handle = hci_event_disconnection_complete_get_connection_handle(packet);
            for (unsigned i = 0; i < kMaxSlots; ++i) {
                if (s_gate[i].handle == handle) {
                    cancel_settle_timer(static_cast<int>(i));
                    s_gate[i] = {};
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

} // namespace

extern "C" int flydigi_apex4_bt_l2cap_status_keep_bond(uint8_t status)
{
    return (status == L2CAP_CONNECTION_RESPONSE_RESULT_RTX_TIMEOUT ||
            status == L2CAP_CONNECTION_RESPONSE_RESULT_REFUSED_RESOURCES ||
            status == L2CAP_CONNECTION_RESPONSE_RESULT_ERTM_NOT_SUPPORTED)
               ? 1
               : 0;
}

extern "C" void flydigi_apex4_bt_l2cap_gate_init(void)
{
    if (s_inited)
        return;
    s_hci_cb.callback = &on_hci_event;
    hci_add_event_handler(&s_hci_cb);
    s_inited = true;
    OGXM_LOG("[APEX4 BT] L2CAP security gate registered (HID Control after auth+enc)\n");
}

extern "C" void flydigi_apex4_bt_l2cap_gate_on_disconnect(uni_hid_device_t* d)
{
    const int slot = device_slot(d);
    if (slot < 0)
        return;
    cancel_settle_timer(slot);
    s_gate[slot] = {};
}

extern "C" int flydigi_apex4_bt_gate_open_hid_control(uni_hid_device_t* d)
{
    if (!d || !flydigi_apex4_bt_is_candidate(d))
        return 0;

    flydigi_apex4_bt_mark_candidate(d);
    const int slot = device_slot(d);
    if (slot < 0)
        return 0;

    SlotGate& g = s_gate[slot];
    g.want_hid_control = true;
    refresh_security_from_stack(d, g);

    if (g.hid_control_pending) {
        OGXM_LOG("[APEX4 BT] HID Control open already pending — ignoring duplicate FSM request\n");
        return 1;
    }
    if (g.settle_timer_armed) {
        OGXM_LOG("[APEX4 BT] HID Control settle already scheduled — ignoring duplicate\n");
        return 1;
    }

    OGXM_LOG("[APEX4 BT] FSM requested HID Control — gating until ACL+auth+enc\n");
    maybe_progress(d, slot);
    return 1;
}

extern "C" int flydigi_apex4_bt_gate_on_hid_control_result(uni_hid_device_t* d, uint8_t status, uint16_t local_cid)
{
    if (!d || !flydigi_apex4_bt_is_candidate(d))
        return 0;

    const int slot = device_slot(d);
    if (slot < 0)
        return 0;

    SlotGate& g = s_gate[slot];
    g.hid_control_pending = false;

    if (status == 0) {
        OGXM_LOG("\n[APEX4 BT L2CAP]\n");
        OGXM_LOG("channel=HID_CONTROL\n");
        OGXM_LOG("PSM=0x0011\n");
        OGXM_LOG("attempt=%u\n", static_cast<unsigned>(g.attempt));
        OGXM_LOG("SUCCESS\n");
        OGXM_LOG("CID=0x%04x\n\n", local_cid);
        g.want_hid_control = false;
        return 0; /* continue normal success path */
    }

    if (!flydigi_apex4_bt_l2cap_status_keep_bond(status))
        return 0; /* let Bluepad32 do default (may drop key) */

    const uint32_t since_enc =
        (g.encryption_ready_ms != 0) ? (now_ms() - g.encryption_ready_ms) : 0;

    OGXM_LOG("\n[APEX4 BT L2CAP]\n");
    OGXM_LOG("channel=HID_CONTROL\n");
    OGXM_LOG("PSM=0x0011\n");
    OGXM_LOG("status=0x%02x\n", status);
    OGXM_LOG("meaning=%s\n", l2cap_status_meaning(status));
    OGXM_LOG("attempt=%u\n", static_cast<unsigned>(g.attempt));
    OGXM_LOG("ACL handle=0x%04x\n", d->conn.handle);
    OGXM_LOG("authenticated=%s\n", g.authenticated ? "YES" : "NO");
    OGXM_LOG("encrypted=%s\n", g.encrypted ? "YES" : "NO");
    OGXM_LOG("time_since_encryption=%lums\n", static_cast<unsigned long>(since_enc));
    OGXM_LOG("link_key_present=%s\n", link_key_present(d->conn.btaddr) ? "YES" : "NO");

    const bool acl_up = handle_alive(d->conn.handle);
    if (!acl_up) {
        OGXM_LOG("action=ABORT\n");
        OGXM_LOG("bond_action=KEEP\n");
        OGXM_LOG("reason=ACL down\n\n");
        g.want_hid_control = false;
        /* Caller must not drop key; still allow disconnect/delete of device object. */
        uni_hid_device_disconnect(d);
        uni_hid_device_delete(d);
        return 1;
    }

    if (g.attempt < kMaxAttempts && security_ready(g)) {
        const uint32_t delay = kRetryDelayMs[g.attempt]; /* next attempt index */
        OGXM_LOG("action=RETRY\n");
        OGXM_LOG("bond_action=KEEP\n");
        OGXM_LOG("Retrying in %lums...\n\n", static_cast<unsigned long>(delay));
        g.want_hid_control = true;
        /* Stay in a state that will not re-fire remote_name L2CAP path. */
        uni_bt_conn_set_state(&d->conn, UNI_BT_CONN_STATE_L2CAP_CONTROL_CONNECTION_REQUESTED);
        schedule_open(d, slot, delay);
        return 1;
    }

    OGXM_LOG("action=GIVE_UP\n");
    OGXM_LOG("bond_action=KEEP\n");
    OGXM_LOG("Disconnecting device object; bond retained.\n\n");
    g.want_hid_control = false;
    uni_hid_device_disconnect(d);
    uni_hid_device_delete(d);
    return 1;
}
