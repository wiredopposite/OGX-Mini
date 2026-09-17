#include "USBHost/UsbHostLifecycle.h"

#include <cstdio>

#include <pico/time.h>
#include "tusb.h"
#include "Board/Config.h"

namespace UsbHostLifecycle {
namespace {

constexpr int32_t ATTACHED_GRACE_US = 4000 * 1000;       /* settle / mode-switch window */
constexpr int32_t ENUM_STALL_US = 3000 * 1000;           /* no progress while ENUMERATING */
constexpr int32_t RECOVERY_WAIT_US = 4000 * 1000;        /* cooldown after the one reset */
constexpr int32_t WATCH_LOG_INTERVAL_US = 500 * 1000;

enum class Progress : uint8_t {
    NONE = 0,
    ATTACH,
    DEVICE_DESCRIPTOR,
    SET_ADDRESS,
    CONFIG_DESCRIPTOR,
    INTERFACE_OPEN,
    MOUNT,
};

UsbHostState s_state = UsbHostState::DISCONNECTED;
Progress s_last_progress = Progress::NONE;
absolute_time_t s_state_entered{};
absolute_time_t s_last_progress_at{};
absolute_time_t s_last_watch_log{};
bool s_recovery_attempted = false;
bool s_resume_bt_emitted = false;
bool s_saw_enum_attach = false;
bool s_logged_enumeration_failed = false;
uint16_t s_last_vid = 0;
uint16_t s_last_pid = 0;
uint8_t s_last_daddr = 0;

static uint8_t progress_rank(Progress p) {
    return static_cast<uint8_t>(p);
}

const char* progress_name(Progress p) {
    switch (p) {
        case Progress::NONE: return "NONE";
        case Progress::ATTACH: return "ATTACH";
        case Progress::DEVICE_DESCRIPTOR: return "DEVICE_DESCRIPTOR";
        case Progress::SET_ADDRESS: return "SET_ADDRESS";
        case Progress::CONFIG_DESCRIPTOR: return "CONFIG_DESCRIPTOR";
        case Progress::INTERFACE_OPEN: return "INTERFACE_OPEN";
        case Progress::MOUNT: return "MOUNT";
        default: return "?";
    }
}

void enter_state(UsbHostState st) {
    if (s_state == st) {
        return;
    }
    s_state = st;
    s_state_entered = get_absolute_time();
}

void note_progress(Progress p, const char* detail) {
    s_last_progress = p;
    s_last_progress_at = get_absolute_time();
#if defined(CONFIG_OGXM_DEBUG)
    const uint32_t t = to_ms_since_boot(get_absolute_time());
    printf("[USB ENUM]\n");
    printf("t=%lu\n", static_cast<unsigned long>(t));
    printf("event=%s\n", detail ? detail : progress_name(p));
    if (s_last_vid || s_last_pid) {
        printf("VID=%04X\nPID=%04X\n", s_last_vid, s_last_pid);
    }
#else
    (void)detail;
#endif
    if (s_state == UsbHostState::ATTACHED_GRACE || s_state == UsbHostState::RECOVERY_WAIT ||
        s_state == UsbHostState::ENUMERATION_FAILED || s_state == UsbHostState::DISCONNECTED) {
        if (p == Progress::ATTACH || p == Progress::DEVICE_DESCRIPTOR || p == Progress::SET_ADDRESS ||
            p == Progress::CONFIG_DESCRIPTOR || p == Progress::INTERFACE_OPEN) {
            enter_state(UsbHostState::ENUMERATING);
            s_resume_bt_emitted = false;
            s_logged_enumeration_failed = false;
        }
    }
}

void clear_attach_cycle() {
    s_last_progress = Progress::NONE;
    s_last_progress_at = get_absolute_time();
    s_recovery_attempted = false;
    s_resume_bt_emitted = false;
    s_saw_enum_attach = false;
    s_logged_enumeration_failed = false;
    s_last_vid = 0;
    s_last_pid = 0;
    s_last_daddr = 0;
}

void poll_descriptor_progress() {
    const uint8_t addr_max = static_cast<uint8_t>(CFG_TUH_DEVICE_MAX + CFG_TUH_HUB);
    for (uint8_t d = 1; d <= addr_max; ++d) {
        if (tuh_mounted(d)) {
            if (s_state != UsbHostState::MOUNTED) {
                uint16_t vid = 0, pid = 0;
                (void)tuh_vid_pid_get(d, &vid, &pid);
                s_last_vid = vid;
                s_last_pid = pid;
                s_last_daddr = d;
                note_progress(Progress::MOUNT, "MOUNT");
                enter_state(UsbHostState::MOUNTED);
#if defined(CONFIG_OGXM_DEBUG)
                printf("[USB HOST]\n");
                printf("state=MOUNTED\n");
                printf("addr=%u\n", static_cast<unsigned>(d));
                printf("VID=%04X\n", vid);
                printf("PID=%04X\n", pid);
#endif
            }
            return;
        }
        uint16_t vid = 0, pid = 0;
        if (tuh_vid_pid_get(d, &vid, &pid)) {
            if (vid != s_last_vid || pid != s_last_pid ||
                progress_rank(s_last_progress) < progress_rank(Progress::DEVICE_DESCRIPTOR)) {
                s_last_vid = vid;
                s_last_pid = pid;
                s_last_daddr = d;
                /* addressed + VID means SET_ADDRESS and GET_DEVICE_DESCRIPTOR completed */
                if (progress_rank(s_last_progress) < progress_rank(Progress::SET_ADDRESS)) {
                    note_progress(Progress::SET_ADDRESS, "SET_ADDRESS");
                }
                note_progress(Progress::DEVICE_DESCRIPTOR, "DEVICE_DESCRIPTOR");
            }
            return;
        }
    }
}

void log_watch(bool root_connected, bool mounted, const char* action, const char* reason, bool force = false) {
#if defined(CONFIG_OGXM_DEBUG)
    const absolute_time_t now = get_absolute_time();
    if (!force && absolute_time_diff_us(s_last_watch_log, now) < WATCH_LOG_INTERVAL_US) {
        return;
    }
    s_last_watch_log = now;
    const int64_t since_prog = absolute_time_diff_us(s_last_progress_at, now) / 1000;
    printf("[USB WATCH]\n");
    printf("root_connected=%s\n", root_connected ? "YES" : "NO");
    printf("mounted=%s\n", mounted ? "YES" : "NO");
    printf("state=%s\n", state_name(s_state));
    if (s_state == UsbHostState::ENUMERATING || s_state == UsbHostState::RECOVERY_WAIT) {
        printf("last_progress=%s\n", progress_name(s_last_progress));
        printf("time_since_progress=%lldms\n", static_cast<long long>(since_prog));
    }
    printf("action=%s\n", action);
    if (reason && reason[0]) {
        printf("reason=%s\n", reason);
    }
#else
    (void)root_connected;
    (void)mounted;
    (void)action;
    (void)reason;
    (void)force;
#endif
}

} // namespace

void reset_for_host_stop() {
    clear_attach_cycle();
    enter_state(UsbHostState::DISCONNECTED);
}

void on_host_started(bool root_connected_at_init) {
    clear_attach_cycle();
    if (root_connected_at_init) {
        enter_state(UsbHostState::ATTACHED_GRACE);
        s_last_progress_at = get_absolute_time();
#if defined(CONFIG_OGXM_DEBUG)
        printf("[USB PREPLUG]\n");
        printf("Physical device detected at startup.\n");
        printf("Waiting %dms for normal enumeration...\n",
               static_cast<int>(ATTACHED_GRACE_US / 1000));
#endif
    } else {
        enter_state(UsbHostState::DISCONNECTED);
    }
}

void on_bus_attach(uint8_t rhport) {
    (void)rhport;
    s_saw_enum_attach = true;
    if (s_state == UsbHostState::MOUNTED) {
        return;
    }
    /* Fresh attach cycle (incl. controller mode re-enumeration). */
    if (s_state == UsbHostState::ENUMERATION_FAILED || s_state == UsbHostState::DISCONNECTED ||
        s_state == UsbHostState::ATTACHED_GRACE || s_state == UsbHostState::RECOVERY_WAIT) {
        s_recovery_attempted = false;
        s_resume_bt_emitted = false;
    }
    note_progress(Progress::ATTACH, "ATTACH");
    enter_state(UsbHostState::ENUMERATING);
}

void on_bus_remove(uint8_t rhport) {
    (void)rhport;
    clear_attach_cycle();
    enter_state(UsbHostState::DISCONNECTED);
#if defined(CONFIG_OGXM_DEBUG)
    printf("[USB ENUM]\n");
    printf("t=%lu\n", static_cast<unsigned long>(to_ms_since_boot(get_absolute_time())));
    printf("event=REMOVE\n");
#endif
}

void on_device_configured(uint8_t daddr) {
    uint16_t vid = 0, pid = 0;
    (void)tuh_vid_pid_get(daddr, &vid, &pid);
    s_last_vid = vid;
    s_last_pid = pid;
    s_last_daddr = daddr;
    note_progress(Progress::MOUNT, "MOUNT");
    enter_state(UsbHostState::MOUNTED);
    s_recovery_attempted = false;
#if defined(CONFIG_OGXM_DEBUG)
    printf("[USB HOST]\n");
    printf("state=MOUNTED\n");
    printf("addr=%u\n", static_cast<unsigned>(daddr));
    printf("VID=%04X\n", vid);
    printf("PID=%04X\n", pid);
#endif
}

void on_device_unmounted(uint8_t daddr) {
    (void)daddr;
    if (s_state == UsbHostState::MOUNTED) {
        /* Soft unmount / reconfigure — allow re-enum without treating as hard fail. */
        s_last_progress = Progress::NONE;
        s_last_progress_at = get_absolute_time();
        enter_state(UsbHostState::ATTACHED_GRACE);
        s_resume_bt_emitted = false;
    }
}

void on_interface_opened(const char* tag) {
    note_progress(Progress::INTERFACE_OPEN, tag ? tag : "INTERFACE_OPEN");
}

UsbHostState state() {
    return s_state;
}

bool recovery_attempted() {
    return s_recovery_attempted;
}

const char* state_name(UsbHostState st) {
    switch (st) {
        case UsbHostState::DISCONNECTED: return "DISCONNECTED";
        case UsbHostState::ATTACHED_GRACE: return "ATTACHED_GRACE";
        case UsbHostState::ENUMERATING: return "ENUMERATING";
        case UsbHostState::MOUNTED: return "MOUNTED";
        case UsbHostState::ENUMERATION_FAILED: return "ENUMERATION_FAILED";
        case UsbHostState::RECOVERY_WAIT: return "RECOVERY_WAIT";
        default: return "?";
    }
}

const char* last_progress_name() {
    return progress_name(s_last_progress);
}

WatchAction watch(bool root_connected, bool tuh_device_configured, bool host_pad_mounted) {
    (void)host_pad_mounted;
    const absolute_time_t now = get_absolute_time();

    if (!root_connected) {
        if (s_state != UsbHostState::DISCONNECTED) {
            clear_attach_cycle();
            enter_state(UsbHostState::DISCONNECTED);
            log_watch(false, false, "NONE", "physical detach — attach cycle cleared");
        }
        return WatchAction::NONE;
    }

    /* Physical presence after idle host — begin grace (hot-plug while BT scanning). */
    if (s_state == UsbHostState::DISCONNECTED) {
        enter_state(UsbHostState::ATTACHED_GRACE);
        s_last_progress_at = now;
        log_watch(true, false, "NONE", "device allowed to settle");
        return WatchAction::NONE;
    }

    if (tuh_device_configured) {
        if (s_state != UsbHostState::MOUNTED) {
            poll_descriptor_progress();
        }
        return WatchAction::NONE;
    }

    poll_descriptor_progress();
    if (s_state == UsbHostState::MOUNTED) {
        return WatchAction::NONE;
    }

    switch (s_state) {
        case UsbHostState::ATTACHED_GRACE: {
            log_watch(true, false, "NONE", "device allowed to settle");
            if (absolute_time_diff_us(s_state_entered, now) < ATTACHED_GRACE_US) {
                return WatchAction::NONE;
            }
            /* Grace expired: only reset if enumeration never began (missed pre-plug attach). */
            if (!s_saw_enum_attach && s_last_progress == Progress::NONE && !s_recovery_attempted) {
#if defined(CONFIG_OGXM_DEBUG)
                printf("[USB PREPLUG]\n");
                printf("No enumeration event received.\n");
                printf("Performing one startup recovery reset.\n");
#endif
                log_watch(true, false, "ONE_ROOT_RESET", "no enumeration began after grace", true);
                return WatchAction::ONE_ROOT_RESET;
            }
            /* Attach/progress happened but not mounted — treat as stall path via ENUMERATING. */
            enter_state(UsbHostState::ENUMERATING);
            return WatchAction::NONE;
        }

        case UsbHostState::ENUMERATING: {
            const int64_t since_prog_us = absolute_time_diff_us(s_last_progress_at, now);
            log_watch(true, false, "NONE", nullptr);
            if (s_last_progress == Progress::NONE) {
                /* Still waiting for first TinyUSB attach — do not reset yet (mode switch / slow pad). */
                if (since_prog_us >= ATTACHED_GRACE_US && !s_recovery_attempted) {
#if defined(CONFIG_OGXM_DEBUG)
                    printf("[USB RECOVERY]\nenumeration never started after grace\n");
#endif
                    log_watch(true, false, "ONE_ROOT_RESET", "no ATTACH after grace", true);
                    return WatchAction::ONE_ROOT_RESET;
                }
                return WatchAction::NONE;
            }
            if (since_prog_us < ENUM_STALL_US) {
                return WatchAction::NONE;
            }
            if (!s_recovery_attempted) {
#if defined(CONFIG_OGXM_DEBUG)
                printf("[USB RECOVERY]\n");
                printf("enumeration genuinely stalled\n");
                printf("last_progress:\n%s\n", progress_name(s_last_progress));
                printf("time_without_progress:\n%lldms\n", static_cast<long long>(since_prog_us / 1000));
                printf("Recovery attempt:\n1\n");
                printf("Performing one root-port reset.\n");
#endif
                log_watch(true, false, "ONE_ROOT_RESET", "ENUMERATION STALLED", true);
                return WatchAction::ONE_ROOT_RESET;
            }
            enter_state(UsbHostState::ENUMERATION_FAILED);
            [[fallthrough]];
        }

        case UsbHostState::ENUMERATION_FAILED: {
#if defined(CONFIG_OGXM_DEBUG)
            if (!s_logged_enumeration_failed) {
                s_logged_enumeration_failed = true;
                printf("[USB HOST]\n");
                printf("Device remains physically connected\n");
                printf("but could not be enumerated.\n");
                printf("Automatic resets stopped.\n");
                printf("Waiting for:\n");
                printf("- device re-enumeration\n");
                printf("- mode change\n");
                printf("- USB detach\n");
                printf("- USB attach\n");
            }
#endif
            log_watch(true, false, "NONE", "automatic resets stopped — bus left stable", true);
            if (!s_resume_bt_emitted) {
                s_resume_bt_emitted = true;
                return WatchAction::RESUME_BT_SCANS;
            }
            return WatchAction::NONE;
        }

        case UsbHostState::RECOVERY_WAIT: {
            log_watch(true, false, "NONE", "cooldown after single recovery reset");
            if (absolute_time_diff_us(s_state_entered, now) < RECOVERY_WAIT_US) {
                return WatchAction::NONE;
            }
            /* Still not mounted after cooldown — fail closed (no attempt 2/3). */
            enter_state(UsbHostState::ENUMERATION_FAILED);
            return WatchAction::NONE;
        }

        case UsbHostState::MOUNTED:
        case UsbHostState::DISCONNECTED:
        default:
            return WatchAction::NONE;
    }
}

void on_recovery_reset_done() {
    s_recovery_attempted = true;
    s_saw_enum_attach = false;
    s_last_progress = Progress::NONE;
    s_last_progress_at = get_absolute_time();
    s_last_vid = 0;
    s_last_pid = 0;
    enter_state(UsbHostState::RECOVERY_WAIT);
#if defined(CONFIG_OGXM_DEBUG)
    printf("[USB WATCH]\nRecovery cooldown %dms — bus left stable.\n",
           static_cast<int>(RECOVERY_WAIT_US / 1000));
#endif
}

} // namespace UsbHostLifecycle
