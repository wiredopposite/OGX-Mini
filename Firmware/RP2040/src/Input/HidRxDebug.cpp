#include "Input/HidRxDebug.h"

#include <cstdio>
#include <cstring>

#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "Board/Config.h"
#include "Board/ogxm_log.h"
#include "pico/time.h"

namespace HidRxDebug {
namespace {

uint8_t s_prev[64]{};
uint16_t s_prev_len{0};
uint8_t s_prev_addr{0};
uint8_t s_prev_inst{0};
uint32_t s_rearm_ok_logged{0};
bool s_arm_logged{false};

} // namespace

void log_initial_arm(uint8_t addr, uint8_t instance, uint8_t ep_in_hint, bool already_pending,
                     bool arm_attempted, bool arm_ok) {
#if defined(CONFIG_OGXM_DEBUG)
    (void)s_arm_logged;
    printf("\n[USB HID RX ARM]\n");
    printf("addr=%u\ninstance=%u\nendpoint=0x%02X\n",
           static_cast<unsigned>(addr), static_cast<unsigned>(instance),
           static_cast<unsigned>(ep_in_hint));
    if (already_pending) {
        printf("initial tuh_hid_receive_report:\nSUCCESS (driver already armed / transfer pending)\n");
    } else if (arm_attempted) {
        printf("initial tuh_hid_receive_report:\n%s\n", arm_ok ? "SUCCESS" : "FAILED");
        if (!arm_ok) {
            printf("reason:\ninterface not ready / claim failed / xfer failed\n");
        }
    } else {
        printf("initial tuh_hid_receive_report:\nFAILED\nreason:\nnot attempted\n");
    }
#else
    (void)addr;
    (void)instance;
    (void)ep_in_hint;
    (void)already_pending;
    (void)arm_attempted;
    (void)arm_ok;
#endif
}

void ensure_armed_after_mount(uint8_t addr, uint8_t instance) {
#if defined(CONFIG_OGXM_DEBUG)
    uint8_t ep_in = 0x82; /* Cyclone/Switch typical; actual EP may differ */
    const bool ready = tuh_hid_receive_ready(addr, instance);
    if (!ready) {
        /* Endpoint busy ⇒ a receive is already pending. */
        log_initial_arm(addr, instance, ep_in, true, false, true);
        return;
    }
    const bool ok = tuh_hid_receive_report(addr, instance);
    log_initial_arm(addr, instance, ep_in, false, true, ok);
#else
    if (tuh_hid_receive_ready(addr, instance)) {
        (void)tuh_hid_receive_report(addr, instance);
    }
#endif
}

void log_raw_rx_if_changed(uint8_t addr, uint8_t instance, const uint8_t* report, uint16_t len) {
#if defined(CONFIG_OGXM_DEBUG)
    if (!report || len == 0) {
        return;
    }
    const uint16_t n = (len < sizeof(s_prev)) ? len : static_cast<uint16_t>(sizeof(s_prev));
    const bool changed =
        (addr != s_prev_addr) || (instance != s_prev_inst) || (n != s_prev_len) ||
        (std::memcmp(s_prev, report, n) != 0);
    if (!changed) {
        return;
    }
    s_prev_addr = addr;
    s_prev_inst = instance;
    s_prev_len = n;
    std::memcpy(s_prev, report, n);

    printf("\n[USB HID RAW RX]\n");
    printf("addr=%u\ninstance=%u\nlen=%u\n", static_cast<unsigned>(addr),
           static_cast<unsigned>(instance), static_cast<unsigned>(len));
    printf("DATA:\n");
    OGXM_LOG_HEX(report, n > 32 ? 32 : n);
#else
    (void)addr;
    (void)instance;
    (void)report;
    (void)len;
#endif
}

void ensure_rearmed_after_rx(uint8_t addr, uint8_t instance) {
    const bool idle = tuh_hid_receive_ready(addr, instance);
    if (!idle) {
#if defined(CONFIG_OGXM_DEBUG)
        if (s_rearm_ok_logged < 4) {
            ++s_rearm_ok_logged;
            printf("\n[USB HID RX REARM]\naddr=%u\ninstance=%u\nresult:\nSUCCESS (pending from driver)\n",
                   static_cast<unsigned>(addr), static_cast<unsigned>(instance));
        }
#endif
        return;
    }

    /* Driver forgot to re-arm — fix so input keeps flowing. */
    const bool ok = tuh_hid_receive_report(addr, instance);
#if defined(CONFIG_OGXM_DEBUG)
    printf("\n[USB HID RX REARM]\naddr=%u\ninstance=%u\nresult:\n%s\n",
           static_cast<unsigned>(addr), static_cast<unsigned>(instance),
           ok ? "SUCCESS (safety re-arm — driver left EP idle)" : "FAILED");
#else
    (void)ok;
#endif
}

} // namespace HidRxDebug
