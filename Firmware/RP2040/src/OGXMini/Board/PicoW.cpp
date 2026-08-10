#include "Board/Config.h"
#include "OGXMini/Board/PicoW.h"
#if defined(OGXM_BOARD_USES_PICO_W_FIRMWARE)

#include <cstdio>
#include <string>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <pico/multicore.h>
#include <pico/time.h>
#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "USBDevice/DeviceDriver/DeviceDriver.h"
#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"
#include "USBDevice/DeviceDriver/PS1PS2/psx_simulator.h"
#include "USBDevice/DeviceDriver/GameCube/gc_simulator.h"
#include "USBDevice/DeviceDriver/Dreamcast/Dreamcast.h"
#include "USBDevice/DeviceDriver/N64/N64.h"
#include "USBHost/GPIOHost/GPIOHost.h"
#include "UserSettings/UserSettings.h"
#include "Board/Config.h"
#include "Board/board_api.h"
#include "Board/board_api_private/board_api_private.h"
#include "Board/ogxm_log.h"
#include "Bluepad32/Bluepad32.h"
#include "BLEServer/BLEServer.h"
#include "Gamepad/Gamepad.h"
#include "TaskQueue/TaskQueue.h"

#if defined(CONFIG_EN_USB_HOST)
#include "host/hcd.h"
#include "pio_usb.h"
#include "USBHost/HostManager.h"
#include <pico/flash.h>
#include "Wii/WiiReportConverter.h"

/** Called from flash_safe_execute on Core1; only write flash. Reboot is done by caller after return. */
static void store_driver_type_callback(void* arg) {
    auto d = *static_cast<DeviceDriverType*>(arg);
    UserSettings::get_instance().store_driver_type_only(d);
}

extern "C" {
#include "Wii/wiimote-lib/wiimote.h"
#include "Wii/wiimote-lib/wiimote_btstack.h"
}

static WiimoteReport s_wiimote_report = {};  // shared: Core0 reads, Core1 writes

extern "C" void wii_led_on(void)  { board_api::set_led(true); }
extern "C" void wii_led_off(void) { board_api::set_led(false); }
#endif

Gamepad _gamepads[MAX_GAMEPADS];

#if defined(CONFIG_EN_USB_HOST)
// PIO USB host normally uses an alarm_pool SOF timer (~1 kHz). On Pico W that collides with BTstack / TaskQueue
// timer IRQs (see prior irq_set_exclusive_handler assert) and fixed alarm indices (alarm 3 "already claimed").
// Use skip_alarm_pool + pio_usb_host_frame() every ~1 ms from Core0 main loop (or Wii Core1 loop).
// Do not run this from BTstack timers / execute_on_main_thread — TinyUSB uses sleep_ms during enumeration.

// Debounce D+/D- in real time before tuh_init (not "N mux calls") so we can safely run multiple
// host frames per main-loop visit when catching up SOFs without shortening this delay.
constexpr int32_t PIO_LINE_STABLE_US = 40 * 1000;
/** Wall-time spacing for send_feedback — do not tie to SOF catch-up tick count (bursts would hammer the device). */
constexpr uint32_t USB_FEEDBACK_INTERVAL_MS = 200;

static bool s_pio_usb_tuh_inited = false;
/** When true, `pio_usb_host_frame()` runs from SDK repeating_timer at 1 kHz (not from main-loop catch-up). */
static bool s_pico_w_usb_sof_hw_timer = false;
static repeating_timer_t s_pico_w_usb_sof_timer;
static bool s_bt_new_conn_off_for_usb = false;
/** Line high continuously since this time (valid while s_usb_line_debounce_armed). */
static absolute_time_t s_usb_line_high_since;
static bool s_usb_line_debounce_armed = false;
static uint32_t s_last_usb_feedback_wall_ms = 0;

static bool pico_w_usb_sof_timer_cb(repeating_timer_t* rt) {
    (void)rt;
    pio_usb_host_frame();
    return true;
}

static void pico_w_usb_sof_timer_start() {
    if (s_pico_w_usb_sof_hw_timer) {
        return;
    }
    /* skip_alarm_pool: PIO-USB does not start its own SOF timer. Main-loop + sleep_ms(1) cannot hold 1 kHz
     * SOF under CYW43/BT load — wired input dies in ~1–2 s. Use SDK repeating_timer (separate from
     * the alarm_pool that collides on Pico W) like a dedicated Core1 loop in Wii mode. */
    if (add_repeating_timer_us(-1000, pico_w_usb_sof_timer_cb, nullptr, &s_pico_w_usb_sof_timer)) {
        s_pico_w_usb_sof_hw_timer = true;
    }
}

static void pico_w_usb_sof_timer_stop() {
    if (!s_pico_w_usb_sof_hw_timer) {
        return;
    }
    cancel_repeating_timer(&s_pico_w_usb_sof_timer);
    s_pico_w_usb_sof_hw_timer = false;
}

/** Debounce any unplug hint (line / stack) past USB bus reset glitches (10–50 ms). */
constexpr int32_t USB_UNPLUG_DEBOUNCE_US = 60 * 1000;
/** Idle-input unplug was removed for [#87](https://github.com/MegaCadeDev/OGX-Mini-2026/issues/87):
 *  report-on-change pads (8BitDo Ultimate 2 2.4 GHz dongle) stop IN while still connected;
 *  treating that as unplug tore the host down after a few quiet seconds. Physical unplug is
 *  detected via HCD disconnect and/or TinyUSB dropping the configured device. */
static bool s_usb_unplug_debounce_armed = false;
static absolute_time_t s_usb_unplug_debounce_since;

static bool pico_w_tuh_any_device_configured() {
    /* usbh.c: TOTAL_DEVICES = CFG_TUH_DEVICE_MAX + CFG_TUH_HUB (hub consumes one address slot). */
    const uint8_t addr_max = static_cast<uint8_t>(CFG_TUH_DEVICE_MAX + CFG_TUH_HUB);
    for (uint8_t d = 1; d <= addr_max; ++d) {
        if (tuh_mounted(d)) {
            return true;
        }
    }
    return false;
}

static void pico_w_usb_host_full_stop() {
    pico_w_usb_sof_timer_stop();
    (void)tuh_deinit(BOARD_TUH_RHPORT);
    s_pio_usb_tuh_inited = false;
    s_usb_line_debounce_armed = false;
    s_usb_unplug_debounce_armed = false;
    /* GPIO IRQs were suspended for PIO; restore so unplug/plug is visible again without "shorting" the port. */
    board_api_usbh::enable_host_line_irq_monitoring();
}

/**
 * Runs every ~1 ms from Core0 (normal Pico W modes) or from Core1 (GPIO modes that block Core0 in run_task).
 * - If any BT gamepad is connected: PIO USB host is not initialized (wired ignored until BT disconnects).
 * - If the PIO USB host is up (cable line stable / enumerating / mounted): disconnect BT pads and block
 *   new BT connections until the host is torn down on unplug. Do not re-enable scans between tuh_init
 *   and first mount — that window is when DualShock 4 enumeration fails under CYW43 load (#47).
 */
static void pico_w_pio_usb_bt_mux_tick() {
    HostManager& hm = HostManager::get_instance();
    const bool bt_active = bluepad32::any_connected();
    const bool wired_mounted = hm.any_mounted();

    /* Only tear down PIO USB host for BT when no wired pad is mounted. Otherwise a slow BT
     * disconnect after wired takeover (or a stray ACL/device_ready) leaves s_bt_any_connected_cached
     * true for ~seconds while USB is working — we would tuh_deinit() here and kill wired input
     * until replug. Wired wins while HostManager still has a device slot. */
    if (bt_active && !wired_mounted) {
        if (s_pio_usb_tuh_inited) {
            pico_w_usb_host_full_stop();
            if (s_bt_new_conn_off_for_usb) {
                s_bt_new_conn_off_for_usb = false;
                bluepad32::wired_usb_release_enable_bt_pairing();
            }
        }
        return;
    }

    /* Unplug: pio_usb_bus_get_line_state() uses gpio_get on D+/D− while PIO owns them — often never
     * reads SE0 when the cable is removed (floating FS-idle), so HCD "connect" can stay true. Prefer
     * TinyUSB configured-device loss; also honor HCD disconnect when it does fire.
     * Do NOT use "no process_report for N ms" — silent-but-connected pads (#87 Ultimate 2 dongle)
     * look identical to a pulled cable on that metric and were false-unplugged. */
    if (s_pio_usb_tuh_inited) {
        const bool hcd_line_connected = hcd_port_connect_status(BOARD_TUH_RHPORT);
        board_api_usbh::store_host_line_connected(hcd_line_connected);
        (void)hcd_line_connected;

        const bool tuh_still_has_device = pico_w_tuh_any_device_configured();
        /* Tear down only when TinyUSB drops the configured device. Brief HCD
         * disconnect glitches alone were killing Ultimate 2 mid-play (#87 rare
         * drop + freeze). Real cable pull clears tuh_mounted; do not trust HCD alone. */
        const bool unplug_hint = wired_mounted && !tuh_still_has_device;

        if (unplug_hint) {
            if (!s_usb_unplug_debounce_armed) {
                s_usb_unplug_debounce_armed = true;
                s_usb_unplug_debounce_since = get_absolute_time();
            } else if (absolute_time_diff_us(s_usb_unplug_debounce_since, get_absolute_time()) >= USB_UNPLUG_DEBOUNCE_US) {
                pico_w_usb_host_full_stop();
                s_bt_new_conn_off_for_usb = false;
                bluepad32::wired_usb_release_enable_bt_pairing();
                return;
            }
        } else {
            s_usb_unplug_debounce_armed = false;
        }
    }

    if (!s_pio_usb_tuh_inited) {
        // Sample D+/D− directly: host_connected() is IRQ-driven and can stay false if an edge was
        // missed (Core0 load, CYW43), or the ISR left IRQs disabled while lines were high.
        // Pins are still GPIO inputs here; PIO has not taken over yet.
        const bool line = (gpio_get(PIO_USB_DP_PIN) != 0) || (gpio_get(PIO_USB_DP_PIN + 1) != 0);
        if (!line) {
            s_usb_line_debounce_armed = false;
            return;
        }
        if (!s_usb_line_debounce_armed) {
            s_usb_line_high_since = get_absolute_time();
            s_usb_line_debounce_armed = true;
        }
        if (absolute_time_diff_us(s_usb_line_high_since, get_absolute_time()) < PIO_LINE_STABLE_US) {
            return;
        }
        /* Quiet CYW43 BT *before* tuh_init. BR/LE inquiry during DS4/XBO enumeration starves PIO SOF
         * on Pico W / 2 W (#47). Keep quiet for the whole host-up window, not only after mount —
         * the old !wired_mounted branch re-enabled scans mid-enumeration and broke first-party DS4. */
        if (!s_bt_new_conn_off_for_usb) {
            bluepad32::wired_usb_takeover_disconnect_bt();
            s_bt_new_conn_off_for_usb = true;
        }
        // main() already called flash_safe_execute_core_init() on Core0; do not call from Core1 BT timer (deadlock risk).
        hm.initialize(_gamepads);
        // PIO USB takes over D+/D−; GPIO edge IRQs on those pins must be off or IO_IRQ floods and BT stalls.
        board_api_usbh::suspend_line_irq();
        pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
        pio_cfg.alarm_pool = nullptr;
        pio_cfg.skip_alarm_pool = true;
        pio_cfg.tx_ch = 2;
        tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
        if (!tuh_init(BOARD_TUH_RHPORT)) {
            s_usb_line_debounce_armed = false;
            s_bt_new_conn_off_for_usb = false;
            bluepad32::wired_usb_release_enable_bt_pairing();
            return;
        }
        s_pio_usb_tuh_inited = true;
        pico_w_usb_sof_timer_start();
    }

    if (!s_pico_w_usb_sof_hw_timer) {
        pio_usb_host_frame();
    }

    // Host drivers (e.g. Xbox360W chatpad) queue Core1 delayed tasks; safe to drain from Core0 (spinlocks).
    TaskQueue::Core1::process_tasks();

    tuh_task();

    {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if ((now_ms - s_last_usb_feedback_wall_ms) >= USB_FEEDBACK_INTERVAL_MS) {
            s_last_usb_feedback_wall_ms = now_ms;
            hm.send_feedback();
        }
    }

    /* While PIO USB host is up (enumerating or mounted), keep BT scans off. Release only in
     * pico_w_usb_host_full_stop() after unplug / BT-priority teardown. */
    if (s_pio_usb_tuh_inited && !s_bt_new_conn_off_for_usb) {
        bluepad32::wired_usb_takeover_disconnect_bt();
        s_bt_new_conn_off_for_usb = true;
    }
}

void pico_w::poll_usb_host_mux_from_core0() {
    static absolute_time_t next_usb_mux;
    static bool usb_mux_time_inited = false;
    if (!usb_mux_time_inited) {
        next_usb_mux = get_absolute_time();
        usb_mux_time_inited = true;
    }
    absolute_time_t now = get_absolute_time();

    if (!s_pio_usb_tuh_inited) {
        if (absolute_time_diff_us(next_usb_mux, now) < 0) {
            return;
        }
        next_usb_mux = delayed_by_us(next_usb_mux, 1000);
        pico_w_pio_usb_bt_mux_tick();
        return;
    }

    if (s_pico_w_usb_sof_hw_timer) {
        /* SOF is 1 kHz from repeating_timer; run TinyUSB + mux every visit (no 1 ms gate). */
        pico_w_pio_usb_bt_mux_tick();
        return;
    }

    if (absolute_time_diff_us(next_usb_mux, now) < 0) {
        return;
    }
    // Fallback if hardware SOF timer failed: manual frames + catch-up (Wii Core1–style burst).
    constexpr int k_max_catchup_frames = 500;
    for (int n = 0; n < k_max_catchup_frames && absolute_time_diff_us(next_usb_mux, now) >= 0; ++n) {
        next_usb_mux = delayed_by_us(next_usb_mux, 1000);
        pico_w_pio_usb_bt_mux_tick();
        now = get_absolute_time();
    }
}
#endif // CONFIG_EN_USB_HOST

static DeviceDriver* s_gpio_device_driver = nullptr;
static void gpio_device_process_cb(void* ctx) {
    (void)ctx;
    if (s_gpio_device_driver) {
        HostInputSource input_src = UserSettings::get_instance().get_input_source();
        if (input_src == HostInputSource::PSX_GPIO) {
            GPIOHost::psx_host_poll(_gamepads[0]);
        } else if (input_src == HostInputSource::GAMECUBE_GPIO) {
            GPIOHost::joybus_host_poll(_gamepads[0]);
        } else if (input_src == HostInputSource::DREAMCAST_GPIO) {
            GPIOHost::dreamcast_host_poll(_gamepads[0]);
        } else if (input_src == HostInputSource::N64_GPIO) {
            GPIOHost::n64_host_poll(_gamepads[0]);
        }
        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
            s_gpio_device_driver->process(i, _gamepads[i]);
        }
        bluepad32::process_pending_adaptive_triggers();
    }
}

#if defined(CONFIG_EN_USB_HOST)
constexpr uint32_t FEEDBACK_DELAY_MS = 200;

void core1_task_wii_usb_host() {
    OGXM_LOG("WII Core1: entry\n");
    flash_safe_execute_core_init();  // so Core0 can flash_safe_execute when saving Wii address (TLV)
    HostManager& host_manager = HostManager::get_instance();
    host_manager.initialize(_gamepads);
    OGXM_LOG("WII Core1: HostManager init done\n");

    OGXM_LOG("WII Core1: waiting for host_connected() (plug USB controller into external port GP0/GP1)\n");
    uint32_t wait_count = 0;
    while (!board_api::usb::host_connected()) {
        sleep_ms(100);
        if ((++wait_count % 20) == 0) {
            OGXM_LOG("WII Core1: still waiting for host_connected()...\n");
        }
    }
    OGXM_LOG("WII Core1: host_connected\n");

    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    pio_cfg.alarm_pool = nullptr;
    pio_cfg.skip_alarm_pool = true;
    // CYW43 (BT on Core0) claims DMA 0 and 1; use channel 2 for PIO USB TX to avoid "DMA channel 0 is already claimed"
    pio_cfg.tx_ch = 2;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    OGXM_LOG("WII Core1: tuh_configure done\n");
    tuh_init(BOARD_TUH_RHPORT);
    OGXM_LOG("WII Core1: tuh_init done\n");

    uint32_t tid_feedback = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(tid_feedback, FEEDBACK_DELAY_MS, true,
        [&host_manager]() { host_manager.send_feedback(); });
    OGXM_LOG("WII Core1: feedback task queued, entering loop\n");

    s_wiimote_report.mode = (uint8_t)NO_EXTENSION;  // Wiimote only; home+down cycles to Nunchuk, then Classic

#if !defined(CONFIG_OGXM_FIXED_DRIVER) || defined(CONFIG_OGXM_FIXED_DRIVER_ALLOW_COMBOS)
    uint32_t last_gp_check_ms = 0;  // throttle: same 600 ms / 3 s hold as other modes
    uint32_t gp_check_ticks = 0;
#endif

    absolute_time_t next_pio_host_frame = get_absolute_time();
    while (true) {
        absolute_time_t now = get_absolute_time();
        while (absolute_time_diff_us(next_pio_host_frame, now) >= 0) {
            pio_usb_host_frame();
            next_pio_host_frame = delayed_by_us(next_pio_host_frame, 1000);
            now = get_absolute_time();
        }
        TaskQueue::Core1::process_tasks();
        tuh_task();
        Wii::gamepad_to_wiimote_report(_gamepads[0], &s_wiimote_report);

#if !defined(CONFIG_OGXM_FIXED_DRIVER) || defined(CONFIG_OGXM_FIXED_DRIVER_ALLOW_COMBOS)
        const uint32_t now_ms = board_api::ms_since_boot();
        if (now_ms - last_gp_check_ms >= static_cast<uint32_t>(UserSettings::GP_CHECK_DELAY_MS)) {
            last_gp_check_ms = now_ms;
            UserSettings& user_settings = UserSettings::get_instance();
            bool changed = user_settings.check_for_driver_change(_gamepads[0]);
#if defined(CONFIG_OGXM_DEBUG)
            if ((++gp_check_ticks % 10) == 0)
                OGXM_LOG("WII Core1: gp check tick " + std::to_string(gp_check_ticks) + "\n");
#endif
            if (changed) {
                static DeviceDriverType new_driver;
                new_driver = user_settings.get_current_driver();
                OGXM_LOG("WII Core1: driver change detected, new_driver=" + OGXM_TO_STRING(new_driver) + ", calling flash_safe_execute\n");
                int fs_ret = flash_safe_execute(store_driver_type_callback, &new_driver, 200);
                OGXM_LOG("WII Core1: flash_safe_execute returned " + std::to_string(fs_ret) + " (0=ok)\n");
                if (fs_ret == 0) {
                    OGXM_LOG("WII Core1: calling reboot\n");
                    board_api::reboot();
                }
            }
        }
#endif
    }
}
#endif

void core1_task() {
    OGXM_LOG("PicoW Core1: entry\n");
    board_api::init_bluetooth();
    OGXM_LOG("PicoW Core1: init_bluetooth done\n");
    board_api::set_led(true);
    OGXM_LOG("PicoW Core1: set_led(true) done\n");
    BLEServer::init_server(_gamepads);
    OGXM_LOG("PicoW Core1: BLEServer init done, entering run_task\n");
    /* PIO USB mux runs on Core0 main loop (poll_usb_host_mux_from_core0), not here — avoids sleep_ms panic. */
    bluepad32::run_task(_gamepads);
}

void set_gp_check_timer(uint32_t task_id) {
#if !defined(CONFIG_OGXM_FIXED_DRIVER) || defined(CONFIG_OGXM_FIXED_DRIVER_ALLOW_COMBOS)
    UserSettings& user_settings = UserSettings::get_instance();
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true,
    [&user_settings] {
        //Check gamepad inputs for button combo to change usb device driver
        if (user_settings.check_for_driver_change(_gamepads[0])) {
            //This will store the new mode and reboot the pico
            user_settings.store_driver_type(user_settings.get_current_driver());
        }
    });
#else
    (void)task_id;  // Fixed output, combos disabled
#endif
}

void pico_w::initialize() {
    OGXM_LOG("PicoW init: start\n");
    board_api::init_board();
    OGXM_LOG("PicoW init: board inited\n");

    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();
#if defined(CONFIG_OGXM_DEBUG)
    OGXM_LOG("PicoW init: flash inited, driver=" + OGXM_TO_STRING(user_settings.get_current_driver()) + "\n");
#else
    OGXM_LOG("PicoW init: flash inited\n");
#endif

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
        _gamepads[i].set_profile(user_settings.get_profile_by_index(i), user_settings.get_current_driver());
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver(), _gamepads);
#if defined(CONFIG_EN_USB_HOST)
    HostManager::get_instance().initialize(_gamepads);
#endif
    OGXM_LOG("PicoW init: driver inited\n");
}

void pico_w::run() {
    OGXM_LOG("PicoW run: start\n");
    multicore_reset_core1();

    UserSettings& user_settings = UserSettings::get_instance();
    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();
    DeviceDriverType current_driver = user_settings.get_current_driver();
    const bool wii_mode = (current_driver == DeviceDriverType::WII);
    const bool gpio_device_mode = (current_driver == DeviceDriverType::PS1PS2 || current_driver == DeviceDriverType::GAMECUBE || current_driver == DeviceDriverType::DREAMCAST);
    OGXM_LOG("PicoW run: wii_mode=" + std::string(wii_mode ? "1" : "0") + " gpio_device_mode=" + std::string(gpio_device_mode ? "1" : "0") + "\n");

#if defined(CONFIG_EN_USB_HOST)
    if (wii_mode) {
        OGXM_LOG("PicoW run: launching Core1 (Wii USB host)\n");
        multicore_launch_core1(core1_task_wii_usb_host);
        OGXM_LOG("PicoW run: Core1 launched, starting Wiimote BT (discoverable as Nintendo RVL-CNT-01)\n");
        board_api::init_bluetooth();
        wiimote_emulator_set_led(wii_led_on, wii_led_off);
        wiimote_emulator(&s_wiimote_report);  // never returns: BT run loop, LED blinks until connected
    } else
#endif
    if (gpio_device_mode) {
        /* PS2 on Pico W: same as Switch/PS3 – Core1 = BT, Core0 = main loop. Core1 writes _gamepads, Core0 reads and calls process() + psx_device_poll(). */
        const bool ps2_poll_mode = (current_driver == DeviceDriverType::PS1PS2);
        if (ps2_poll_mode) {
            psx_device_set_poll_mode(true);
            OGXM_LOG("PicoW run: PS2 poll mode (BT on Core1, protocol on Core0)\n");
        }
        OGXM_LOG("PicoW run: GPIO device mode\n");
        HostInputSource input_src = user_settings.get_input_source();
        s_gpio_device_driver = device_driver;
        if (input_src == HostInputSource::PSX_GPIO) {
            GPIOHost::psx_host_init(0, 20, 19);
        } else if (input_src == HostInputSource::GAMECUBE_GPIO && current_driver != DeviceDriverType::GAMECUBE) {
            /* Skip joybus host when in GameCube device mode: Core1 uses GPIO 19 for console output. */
            GPIOHost::joybus_host_init(0, 19);
        } else if (input_src == HostInputSource::DREAMCAST_GPIO) {
            GPIOHost::dreamcast_host_init(1, 0, 10, -1);
        } else if (input_src == HostInputSource::N64_GPIO) {
            GPIOHost::n64_host_init(0, 19);
        }
        bluepad32::set_gpio_device_process_callback(gpio_device_process_cb, nullptr);
        /* Present as USB device to console immediately so PS2/GC/Dreamcast see the adapter at boot. */
        tud_init(BOARD_TUD_RHPORT);
        OGXM_LOG("PicoW run: tud_init done (GPIO device mode)\n");
        if (ps2_poll_mode) {
            /* PS2: BT on Core1 (like Switch/PS3); Core0 main loop does process() + psx_device_poll(). */
            OGXM_LOG("PicoW run: launching Core1 (BT)\n");
            multicore_launch_core1(core1_task);
        } else {
            OGXM_LOG("PicoW run: Core0 calling init_bluetooth/set_led/BLEServer\n");
            board_api::init_bluetooth();
            board_api::set_led(true);
            BLEServer::init_server(_gamepads);
            OGXM_LOG("PicoW run: Core0 BT/LED/BLEServer done, launching Core1\n");
            if (current_driver == DeviceDriverType::DREAMCAST) {
                dreamcast_set_core1_device_mode(input_src != HostInputSource::DREAMCAST_GPIO);
            }
            if (current_driver == DeviceDriverType::GAMECUBE) {
                multicore_launch_core1(gamecube_core1_entry);
            } else if (current_driver == DeviceDriverType::N64) {
                multicore_launch_core1(n64_core1_entry);
            } else {
                multicore_launch_core1(dreamcast_core1_entry);
            }
            /* Let Core1 reach flash_safe_execute_core_init() before we enter run_task (BT stack may touch flash). */
            sleep_ms(100);
#if defined(CONFIG_EN_USB_HOST)
            bluepad32::set_pico_w_pio_usb_mux_tick(pico_w_pio_usb_bt_mux_tick);
#endif
            bluepad32::run_task(_gamepads);  /* never returns */
        }
    } else {
        tud_init(BOARD_TUD_RHPORT);
        OGXM_LOG("PicoW run: tud_init done, launching Core1 (BT)\n");
        multicore_launch_core1(core1_task);
        OGXM_LOG("PicoW run: Core1 (BT) launched\n");

        HostInputSource input_src = user_settings.get_input_source();
        if (input_src == HostInputSource::PSX_GPIO) {
            GPIOHost::psx_host_init(0, 20, 19);
        } else if (input_src == HostInputSource::GAMECUBE_GPIO) {
            GPIOHost::joybus_host_init(0, 19);
        } else if (input_src == HostInputSource::DREAMCAST_GPIO) {
            GPIOHost::dreamcast_host_init(1, 0, 10, -1);
        } else if (input_src == HostInputSource::N64_GPIO) {
            GPIOHost::n64_host_init(0, 19);
        }
    }

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check);
    OGXM_LOG("PicoW run: gp check timer set, entering main loop\n");

    static bool mounted_logged = false;
    static uint32_t loop_count = 0;
    const bool ps2_poll_mode = gpio_device_mode && (current_driver == DeviceDriverType::PS1PS2);
    while (true) {
        /* PS2 first: drain before any other work so we respond within byte time (reduces OPL pad-init hang). */
        if (ps2_poll_mode) {
            while (psx_device_poll() != 0)
                ;
            /* Short burst: update report and drain again to catch OPL's rapid init. */
            for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
                device_driver->process(i, _gamepads[i]);
            while (psx_device_poll() != 0)
                ;
        }
        TaskQueue::Core0::process_tasks();
        if (!wii_mode) {
            tud_task();
            HostInputSource input_src = UserSettings::get_instance().get_input_source();
            if (input_src == HostInputSource::PSX_GPIO) {
                GPIOHost::psx_host_poll(_gamepads[0]);
            } else if (input_src == HostInputSource::GAMECUBE_GPIO && UserSettings::get_instance().get_current_driver() != DeviceDriverType::GAMECUBE) {
                GPIOHost::joybus_host_poll(_gamepads[0]);
            } else if (input_src == HostInputSource::DREAMCAST_GPIO) {
                GPIOHost::dreamcast_host_poll(_gamepads[0]);
            } else if (input_src == HostInputSource::N64_GPIO) {
                GPIOHost::n64_host_poll(_gamepads[0]);
            }
        }
        if (ps2_poll_mode) {
            while (psx_device_poll() != 0)
                ;
            if (loop_count != 0 && (loop_count % 10000u) == 0)
                OGXM_LOG("PicoW run: main loop psx_device_poll tick " + std::to_string(loop_count) + "\n");
        }
        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
            device_driver->process(i, _gamepads[i]);
        }
        bluepad32::process_pending_adaptive_triggers();
        if (!wii_mode) {
            if (tud_mounted() && !mounted_logged) {
                mounted_logged = true;
                OGXM_LOG("PicoW run: USB configured\n");
            }
        } else if (loop_count == 0 || loop_count == 5000) {
            OGXM_LOG("PicoW run: Wii main loop tick\n");
        }
        loop_count++;
        /* Run USB host mux after all heavy work (device_driver->process, etc.) so pio_usb_host_frame
         * catch-up covers the whole iteration; otherwise SOFs can lag for tens of ms and wired input dies. */
#if defined(CONFIG_EN_USB_HOST)
        if (!wii_mode) {
            pico_w::poll_usb_host_mux_from_core0();
        }
#endif
        /* Main-loop yield (#54 / wired host):
         * - sleep_ms(1) while tud_mounted() starves OG Xbox Duke USB → MC3 / Half-Life slowdown.
         * - While USB device configured: never sleep_ms(1). BT pad connected → sleep_us(250) so
         *   Core1/CYW43 still run; unpaired → tight_loop_contents().
         * - Wired PIO host pad mounted: short sleep_us so tuh_task keeps up under BT load.
         * - USB not configured: sleep_ms(1) OK (pairing / idle). */
        if (!ps2_poll_mode) {
#if defined(CONFIG_EN_USB_HOST)
            if (!wii_mode && s_pio_usb_tuh_inited && HostManager::get_instance().any_mounted()) {
                sleep_us(400);
            } else
#endif
            if (!wii_mode && tud_mounted()) {
                if (bluepad32::any_connected()) {
                    sleep_us(250);
                } else {
                    tight_loop_contents();
                }
            } else {
                sleep_ms(1);
            }
        }
    }
}

// #else // OGXM_BOARD_USES_PICO_W_FIRMWARE

#endif // OGXM_BOARD_USES_PICO_W_FIRMWARE
