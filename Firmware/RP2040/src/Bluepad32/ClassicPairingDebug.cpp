#include "Bluepad32/ClassicPairingDebug.h"

#include <cstdio>
#include <cstring>

#include <btstack.h>
#include "btstack_event.h"
#include "btstack_run_loop.h"
#include "btstack_util.h"
#include "gap.h"
#include "hci.h"
#include "l2cap.h"

#include "Board/ogxm_log.h"
#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4WukongBtL2cap.h"
#include "sdkconfig.h"
#include "uni.h"
#include "uni_btstack_version_compat.h"
#include "uni_version.h"
#include "bt/uni_bt.h"
#include "bt/uni_bt_bredr.h"

namespace {

btstack_packet_callback_registration_t s_hci_cb{};
btstack_packet_callback_registration_t s_l2cap_cb{};

constexpr unsigned kMaxAuthKick = 4;
struct AuthKick {
    bool used{false};
    hci_con_handle_t handle{HCI_CON_HANDLE_INVALID};
    btstack_timer_source_t timer{};
};
AuthKick s_auth_kicks[kMaxAuthKick]{};

const char* hci_status_name(uint8_t status)
{
    switch (status) {
        case 0x00: return "SUCCESS";
        case 0x01: return "UNKNOWN_HCI_COMMAND";
        case 0x02: return "UNKNOWN_CONNECTION_IDENTIFIER";
        case 0x03: return "HARDWARE_FAILURE";
        case 0x04: return "PAGE_TIMEOUT";
        case 0x05: return "AUTHENTICATION_FAILURE";
        case 0x06: return "PIN_OR_KEY_MISSING";
        case 0x07: return "MEMORY_CAPACITY_EXCEEDED";
        case 0x08: return "CONNECTION_TIMEOUT";
        case 0x09: return "CONNECTION_LIMIT_EXCEEDED";
        case 0x0c: return "COMMAND_DISALLOWED";
        case 0x0d: return "CONNECTION_REJECTED_LIMITED_RESOURCES";
        case 0x0e: return "CONNECTION_REJECTED_SECURITY";
        case 0x0f: return "CONNECTION_REJECTED_UNACCEPTABLE_BD_ADDR";
        case 0x13: return "REMOTE_USER_TERMINATED_CONNECTION";
        case 0x14: return "REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES";
        case 0x15: return "REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF";
        case 0x16: return "CONNECTION_TERMINATED_BY_LOCAL_HOST";
        case 0x19: return "PAIRING_NOT_ALLOWED";
        case 0x1f: return "UNSPECIFIED_ERROR";
        case 0x22: return "LMP_RESPONSE_TIMEOUT";
        case 0x29: return "PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED";
        case 0x3e: return "CONNECTION_FAILED_TO_BE_ESTABLISHED";
        default: return "UNKNOWN";
    }
}

const char* link_key_type_name(link_key_type_t t)
{
    switch (t) {
        case COMBINATION_KEY: return "COMBINATION";
        case LOCAL_UNIT_KEY: return "LOCAL_UNIT";
        case REMOTE_UNIT_KEY: return "REMOTE_UNIT";
        case DEBUG_COMBINATION_KEY: return "DEBUG_COMBINATION";
        case UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192: return "UNAUTH_P192";
        case AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192: return "AUTH_P192";
        case CHANGED_COMBINATION_KEY: return "CHANGED_COMBINATION";
        case UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256: return "UNAUTH_P256";
        case AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256: return "AUTH_P256";
        default: return "INVALID_OR_OTHER";
    }
}

void log_status(const char* tag, uint8_t status)
{
    OGXM_LOG("[BT SECURITY] %s status=0x%02x %s\n", tag, status, hci_status_name(status));
}

void dump_versions_and_config()
{
    OGXM_LOG("\n");
    OGXM_LOG("====================================================\n");
    OGXM_LOG("OGXM Bluetooth versions / Classic security\n");
    OGXM_LOG("====================================================\n");
    OGXM_LOG("Bluepad32: v%s\n", UNI_VERSION_STRING);
    OGXM_LOG("BTstack: v%s (header BTSTACK_VERSION_STRING)\n", BTSTACK_VERSION_STRING);
    OGXM_LOG("BTstack source: Firmware/external/bluepad32/external/btstack\n");
    OGXM_LOG("  (PICO_BTSTACK_PATH → Bluepad32 tree; NOT Pico SDK lib/btstack)\n");
    OGXM_LOG("Pico SDK: see Firmware/external/pico-sdk (submodule)\n");
    OGXM_LOG("\n");
    OGXM_LOG("Bluepad32 / OGX BTstack patches (CMake apply_lib_patches):\n");
    OGXM_LOG("  btstack_l2cap.diff: APPLIED (incoming L2CAP security check forced OK)\n");
    OGXM_LOG("  btstack_hids_num_reports.diff: APPLIED\n");
    OGXM_LOG("  bluepad32_uni.diff: APPLIED\n");
    OGXM_LOG("  bluepad32_8bitdo_pids.diff: APPLIED\n");
    OGXM_LOG("  pico_sdk_hids_host.diff: APPLIED\n");
    OGXM_LOG("  bluepad32/external/patches/0002-l2cap-*.patch: NOT auto-applied by OGX\n");
    OGXM_LOG("\n");
    OGXM_LOG("Effective Classic security (BTstack defaults + Bluepad32):\n");
    OGXM_LOG("  gap_get_security_level(): %u\n", static_cast<unsigned>(gap_get_security_level()));
#if defined(CONFIG_BLUEPAD32_GAP_SECURITY)
    OGXM_LOG("  CONFIG_BLUEPAD32_GAP_SECURITY: 1 (uni_bt_bredr_setup sets gap_set_security_level)\n");
#else
    OGXM_LOG("  CONFIG_BLUEPAD32_GAP_SECURITY: 0\n");
#endif
    OGXM_LOG("  gap_ssp_set_*: Bluepad32 does not override — BTstack defaults:\n");
    OGXM_LOG("    ssp_enable=1\n");
    OGXM_LOG("    ssp_io_capability=NO_INPUT_NO_OUTPUT (0x03)\n");
    OGXM_LOG("    ssp_authentication_requirement=GENERAL_BONDING no MITM\n");
    OGXM_LOG("    ssp_auto_accept=1\n");
    OGXM_LOG("    secure_connections_enable=1 (if controller supports)\n");
    OGXM_LOG("  OGXM note: L2CAP security wait is bypassed by btstack_l2cap.diff;\n");
    OGXM_LOG("  incoming ACL now kicks gap_request_security_level(LEVEL_2) so SSP can start.\n");
    OGXM_LOG("====================================================\n");
    OGXM_LOG("\n");
}

void auth_kick_timer_cb(btstack_timer_source_t* ts)
{
    AuthKick* slot = nullptr;
    for (unsigned i = 0; i < kMaxAuthKick; ++i) {
        if (&s_auth_kicks[i].timer == ts) {
            slot = &s_auth_kicks[i];
            break;
        }
    }
    if (!slot || !slot->used)
        return;

    const hci_con_handle_t handle = slot->handle;
    slot->used = false;
    slot->handle = HCI_CON_HANDLE_INVALID;

    if (handle == HCI_CON_HANDLE_INVALID)
        return;

    /* Skip if connection already gone. */
    if (gap_get_connection_type(handle) == GAP_CONNECTION_INVALID) {
        OGXM_LOG("[BT SECURITY] auth kick skipped — handle 0x%04x gone\n", handle);
        return;
    }

    const gap_security_level_t cur = gap_security_level(handle);
    OGXM_LOG("[BT SECURITY] Incoming Classic auth kick handle=0x%04x current_level=%u → request LEVEL_2\n",
             handle, static_cast<unsigned>(cur));
    if (cur >= LEVEL_2) {
        OGXM_LOG("[BT SECURITY] already >= LEVEL_2 — no authenticate request\n");
        return;
    }
    gap_request_security_level(handle, LEVEL_2);
}

void schedule_incoming_auth_kick(hci_con_handle_t handle)
{
    for (unsigned i = 0; i < kMaxAuthKick; ++i) {
        if (s_auth_kicks[i].used && s_auth_kicks[i].handle == handle)
            return; /* already scheduled */
    }
    for (unsigned i = 0; i < kMaxAuthKick; ++i) {
        if (s_auth_kicks[i].used)
            continue;
        AuthKick& slot = s_auth_kicks[i];
        slot.used = true;
        slot.handle = handle;
        slot.timer.process = auth_kick_timer_cb;
        slot.timer.context = nullptr;
        /* Delay so LINK_KEY_REQUEST can complete (reply or negative) first. */
        btstack_run_loop_set_timer(&slot.timer, 150);
        btstack_run_loop_add_timer(&slot.timer);
        OGXM_LOG("[BT SECURITY] scheduled auth kick for handle=0x%04x in 150ms\n", handle);
        return;
    }
    OGXM_LOG("[BT SECURITY] auth kick table full — cannot schedule handle=0x%04x\n", handle);
}

void log_link_key_request(const bd_addr_t addr)
{
    link_key_t key{};
    link_key_type_t type = static_cast<link_key_type_t>(0xff);
    const bool have = gap_get_link_key_for_bd_addr(const_cast<uint8_t*>(addr), key, &type);
    /* Wipe stack copy of secret immediately — never log key bytes. */
    std::memset(key, 0, sizeof(key));

    OGXM_LOG("\n[APEX4 BT SECURITY] (also applies to any Classic pad)\n");
    OGXM_LOG("LINK_KEY_REQUEST\n");
    OGXM_LOG("addr=%s\n", bd_addr_to_str(const_cast<uint8_t*>(addr)));
    if (have) {
        OGXM_LOG("Stored key: YES\n");
        OGXM_LOG("Key type: %u (%s)\n", static_cast<unsigned>(type), link_key_type_name(type));
        OGXM_LOG("Action: BTstack will LINK_KEY_REQUEST_REPLY if level sufficient, else NEGATIVE\n");
    } else {
        OGXM_LOG("Stored key: NO\n");
        OGXM_LOG("Action: LINK_KEY_REQUEST_NEGATIVE_REPLY\n");
        OGXM_LOG("Waiting for fresh SSP pairing...\n");
    }
    OGXM_LOG("\n");
}

#if defined(CONFIG_OGXM_DEBUG)
void hci_security_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET)
        return;

    const uint8_t event = hci_event_packet_get_type(packet);
    bd_addr_t addr{};

    switch (event) {
        case HCI_EVENT_CONNECTION_REQUEST: {
            const uint8_t link_type = hci_event_connection_request_get_link_type(packet);
            hci_event_connection_request_get_bd_addr(packet, addr);
            const uint32_t cod = hci_event_connection_request_get_class_of_device(packet);
            OGXM_LOG("[BT HCI] CONNECTION_REQUEST addr=%s link_type=%u COD=0x%06lx\n",
                     bd_addr_to_str(addr), link_type, static_cast<unsigned long>(cod));
            break;
        }
        case HCI_EVENT_CONNECTION_COMPLETE: {
            const uint8_t status = hci_event_connection_complete_get_status(packet);
            hci_event_connection_complete_get_bd_addr(packet, addr);
            const hci_con_handle_t handle = hci_event_connection_complete_get_connection_handle(packet);
            log_status("CONNECTION_COMPLETE", status);
            OGXM_LOG("[BT HCI] CONNECTION_COMPLETE addr=%s handle=0x%04x\n", bd_addr_to_str(addr), handle);
            if (status == 0) {
                /*
                 * Incoming Classic gamepads (Xbox / APEX4 / etc.) often wait for the host
                 * (after role switch to master) to start authentication. Bluepad32 skips
                 * gap_request_security_level when CONFIG_BLUEPAD32_GAP_SECURITY=1, and OGX's
                 * L2CAP patch bypasses the security wait that would otherwise start it.
                 * Kick LEVEL_2 after LINK_KEY exchange so SSP/Just Works can proceed.
                 */
                schedule_incoming_auth_kick(handle);
            }
            break;
        }
        case HCI_EVENT_LINK_KEY_REQUEST:
            hci_event_link_key_request_get_bd_addr(packet, addr);
            log_link_key_request(addr);
            break;
        case HCI_EVENT_LINK_KEY_NOTIFICATION: {
            /* Same layout as LINK_KEY_REQUEST for BD_ADDR; key type at offset 24. */
            reverse_bd_addr(&packet[2], addr);
            const link_key_type_t type = static_cast<link_key_type_t>(packet[24]);
            OGXM_LOG("[BT HCI] LINK_KEY_NOTIFICATION addr=%s type=%u (%s) (key bytes not logged)\n",
                     bd_addr_to_str(addr), static_cast<unsigned>(type), link_key_type_name(type));
            break;
        }
        case HCI_EVENT_PIN_CODE_REQUEST:
            hci_event_pin_code_request_get_bd_addr(packet, addr);
            OGXM_LOG("[BT HCI] PIN_CODE_REQUEST addr=%s\n", bd_addr_to_str(addr));
            break;
        case HCI_EVENT_IO_CAPABILITY_REQUEST:
            hci_event_io_capability_request_get_bd_addr(packet, addr);
            OGXM_LOG("[BT HCI] IO_CAPABILITY_REQUEST addr=%s\n", bd_addr_to_str(addr));
            break;
        case HCI_EVENT_IO_CAPABILITY_RESPONSE: {
            hci_event_io_capability_response_get_bd_addr(packet, addr);
            const uint8_t io = hci_event_io_capability_response_get_io_capability(packet);
            const uint8_t auth = hci_event_io_capability_response_get_authentication_requirements(packet);
            OGXM_LOG("[BT HCI] IO_CAPABILITY_RESPONSE addr=%s io_cap=0x%02x auth_req=0x%02x\n",
                     bd_addr_to_str(addr), io, auth);
            break;
        }
        case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
            hci_event_user_confirmation_request_get_bd_addr(packet, addr);
            const uint32_t numeric = hci_event_user_confirmation_request_get_numeric_value(packet);
            OGXM_LOG("[BT HCI] USER_CONFIRMATION_REQUEST addr=%s numeric=%lu (ssp_auto_accept expected)\n",
                     bd_addr_to_str(addr), static_cast<unsigned long>(numeric));
            break;
        }
        case HCI_EVENT_USER_PASSKEY_REQUEST:
            hci_event_user_passkey_request_get_bd_addr(packet, addr);
            OGXM_LOG("[BT HCI] USER_PASSKEY_REQUEST addr=%s\n", bd_addr_to_str(addr));
            break;
        case HCI_EVENT_SIMPLE_PAIRING_COMPLETE: {
            hci_event_simple_pairing_complete_get_bd_addr(packet, addr);
            const uint8_t status = hci_event_simple_pairing_complete_get_status(packet);
            log_status("SIMPLE_PAIRING_COMPLETE", status);
            OGXM_LOG("[BT HCI] SIMPLE_PAIRING_COMPLETE addr=%s\n", bd_addr_to_str(addr));
            break;
        }
        case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT: {
            const uint8_t status = hci_event_authentication_complete_get_status(packet);
            const hci_con_handle_t handle = hci_event_authentication_complete_get_connection_handle(packet);
            log_status("AUTHENTICATION_COMPLETE", status);
            OGXM_LOG("[BT HCI] AUTHENTICATION_COMPLETE handle=0x%04x\n", handle);
            break;
        }
        case HCI_EVENT_ENCRYPTION_CHANGE:
        case HCI_EVENT_ENCRYPTION_CHANGE_V2: {
            const uint8_t status = hci_event_encryption_change_get_status(packet);
            const hci_con_handle_t handle = hci_event_encryption_change_get_connection_handle(packet);
            const uint8_t enc = hci_event_encryption_change_get_encryption_enabled(packet);
            log_status("ENCRYPTION_CHANGE", status);
            OGXM_LOG("[BT HCI] ENCRYPTION_CHANGE handle=0x%04x enabled=%u\n", handle, enc);
            break;
        }
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            const uint8_t status = hci_event_disconnection_complete_get_status(packet);
            const hci_con_handle_t handle = hci_event_disconnection_complete_get_connection_handle(packet);
            const uint8_t reason = hci_event_disconnection_complete_get_reason(packet);
            log_status("DISCONNECTION_COMPLETE event_status", status);
            OGXM_LOG("[BT HCI] DISCONNECTION_COMPLETE handle=0x%04x reason=0x%02x %s\n",
                     handle, reason, hci_status_name(reason));
            break;
        }
        case HCI_EVENT_ROLE_CHANGE: {
            const uint8_t status = hci_event_role_change_get_status(packet);
            hci_event_role_change_get_bd_addr(packet, addr);
            const uint8_t role = hci_event_role_change_get_role(packet);
            log_status("ROLE_CHANGE", status);
            OGXM_LOG("[BT HCI] ROLE_CHANGE addr=%s role=%u (0=master,1=slave)\n",
                     bd_addr_to_str(addr), role);
            break;
        }
        default:
            break;
    }
}

void l2cap_security_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET)
        return;
    const uint8_t event = hci_event_packet_get_type(packet);
    switch (event) {
        case L2CAP_EVENT_INCOMING_CONNECTION: {
            const uint16_t psm = l2cap_event_incoming_connection_get_psm(packet);
            const hci_con_handle_t handle = l2cap_event_incoming_connection_get_handle(packet);
            const uint16_t cid = l2cap_event_incoming_connection_get_local_cid(packet);
            OGXM_LOG("[BT L2CAP] INCOMING_CONNECTION handle=0x%04x psm=0x%04x local_cid=0x%04x\n",
                     handle, psm, cid);
            break;
        }
        case L2CAP_EVENT_CHANNEL_OPENED: {
            const uint8_t status = l2cap_event_channel_opened_get_status(packet);
            const uint16_t psm = l2cap_event_channel_opened_get_psm(packet);
            const uint16_t cid = l2cap_event_channel_opened_get_local_cid(packet);
            const hci_con_handle_t handle = l2cap_event_channel_opened_get_handle(packet);
            log_status("L2CAP_CHANNEL_OPENED", status);
            OGXM_LOG("[BT L2CAP] CHANNEL_OPENED handle=0x%04x psm=0x%04x cid=0x%04x\n",
                     handle, psm, cid);
            break;
        }
        case L2CAP_EVENT_CHANNEL_CLOSED: {
            const uint16_t cid = l2cap_event_channel_closed_get_local_cid(packet);
            OGXM_LOG("[BT L2CAP] CHANNEL_CLOSED local_cid=0x%04x\n", cid);
            break;
        }
        default:
            break;
    }
}
#endif /* CONFIG_OGXM_DEBUG */

} // namespace

extern "C" void ogxm_bt_debug_clear_keys(void)
{
    OGXM_LOG("[BT SECURITY] Clearing all stored Bluetooth keys (BR/EDR + LE)...\n");
    uni_bt_del_keys_safe();
    OGXM_LOG("[BT SECURITY] Key clear scheduled. Re-pair controllers after this.\n");
}

extern "C" void ogxm_bt_debug_list_keys(void)
{
    OGXM_LOG("[BT SECURITY] Listing bonded keys...\n");
    uni_bt_list_keys_safe();
}

extern "C" void ogxm_classic_pairing_debug_init(void)
{
    /* Always register APEX L2CAP timing gate (Release + Debug). */
    flydigi_apex4_bt_l2cap_gate_init();

#if defined(CONFIG_OGXM_DEBUG)
    dump_versions_and_config();
    ogxm_bt_debug_list_keys();

#if defined(OGXM_BT_CLEAR_KEYS_ON_BOOT) && OGXM_BT_CLEAR_KEYS_ON_BOOT
    OGXM_LOG("[BT SECURITY] OGXM_BT_CLEAR_KEYS_ON_BOOT=1 — clearing keys once this boot\n");
    uni_bt_del_keys_unsafe();
#endif

    s_hci_cb.callback = &hci_security_packet_handler;
    hci_add_event_handler(&s_hci_cb);

    s_l2cap_cb.callback = &l2cap_security_packet_handler;
    l2cap_add_event_handler(&s_l2cap_cb);

    /* Ensure SSP Just Works path is enabled (no-op if already default). */
    gap_ssp_set_enable(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gap_ssp_set_authentication_requirement(SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING);
    gap_ssp_set_auto_accept(1);

    OGXM_LOG("[BT SECURITY] Classic pairing debug handlers registered\n");
    OGXM_LOG("[BT SECURITY] To clear bonds: rebuild with -DOGXM_BT_CLEAR_KEYS_ON_BOOT=1\n");
    OGXM_LOG("[BT SECURITY]   or call ogxm_bt_debug_clear_keys() from a debug hook\n");
#else
    (void)s_hci_cb;
    (void)s_l2cap_cb;
#endif
}
