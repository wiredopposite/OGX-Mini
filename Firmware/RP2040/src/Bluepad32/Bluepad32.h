#pragma once

#include <cstdint>
#include <array>

#include "Gamepad/Gamepad.h"
#include "Board/Config.h"

/*  NOTE: Everything bluepad32/uni needs to be wrapped
    and kept away from tinyusb due to naming conflicts */

namespace bluepad32 {
    /** True if any Bluetooth gamepad is connected (Classic or LE). */
    bool any_connected();
    void run_task(Gamepad(&gamepads)[MAX_GAMEPADS]);
    void init(Gamepad(&gamepads)[MAX_GAMEPADS]);
    /**
     * Pico W / Pico 2 W only: register a 1 ms callback from the Bluetooth run loop for
     * PIO USB host + BT mutual exclusion. Call once before run_task(); pass nullptr to disable.
     */
    void set_pico_w_pio_usb_mux_tick(void (*tick_cb)(void));
    /** Disconnect all BT gamepads and block new connections (PIO wired USB took over). */
    void wired_usb_takeover_disconnect_bt();
    /** Re-enable BT pairing after wired USB device unplugged. */
    void wired_usb_release_enable_bt_pairing();
    /** Pico W / Pico 2 W / RP2354 BT: restore pairing scans after USB host resume (e.g. Xbox 360 standby wake). */
    void on_usb_device_resume();
    /** Call from main loop to send deferred PS5 adaptive-trigger updates (keeps BT callback fast). */
    void process_pending_adaptive_triggers();
    /** Optional: when set, a timer in the BT run loop calls this every ~4 ms (for GPIO device modes: PS1/PS2, GameCube). Call before run_task(). */
    void set_gpio_device_process_callback(void (*callback)(void* ctx), void* ctx);
} 