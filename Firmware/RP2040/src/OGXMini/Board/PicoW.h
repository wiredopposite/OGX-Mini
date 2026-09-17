#pragma once

#include <cstdint>

namespace pico_w {
    void initialize();
    void run();
#if defined(CONFIG_EN_USB_HOST)
    /** Core0: ~1 kHz PIO USB host + BT/wired mux (must not run from BT timer / execute_on_main_thread). */
    void poll_usb_host_mux_from_core0();
#endif
} // namespace pico_w