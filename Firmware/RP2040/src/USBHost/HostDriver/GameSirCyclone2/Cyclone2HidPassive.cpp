#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/GameSirCyclone2/Cyclone2HidPassive.h"

#include <cstring>

#include "Board/ogxm_log.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Trace.h"

void Cyclone2HidPassive::reset() {
    started_ = false;
    banner_printed_ = false;
    prev_raw_len_ = 0;
    std::memset(prev_raw_, 0, sizeof(prev_raw_));
}

void Cyclone2HidPassive::dump_hid_usage_hint(const uint8_t* report_desc, uint16_t desc_len) {
    (void)report_desc;
    (void)desc_len;
}

void Cyclone2HidPassive::dump_personality(uint8_t address, uint8_t instance,
                                           const uint8_t* report_desc, uint16_t desc_len) {
    (void)report_desc;
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    /* Mount-path safe: no descriptor sync / config tree / hex dumps. */
    OGXM_LOG("[CYCLONE2 RXR] HID passive addr=%u inst=%u %04X:%04X desc_len=%u\n",
             static_cast<unsigned>(address), static_cast<unsigned>(instance),
             vid, pid, static_cast<unsigned>(desc_len));
}

void Cyclone2HidPassive::start(uint8_t address, uint8_t instance,
                                const uint8_t* report_desc, uint16_t desc_len) {
    reset();
    started_ = true;
    address_ = address;
    instance_ = instance;
    if (!banner_printed_) {
        banner_printed_ = true;
        dump_personality(address, instance, report_desc, desc_len);
    }
    (void)tuh_hid_receive_report(address, instance);
}

void Cyclone2HidPassive::process_report(uint8_t address, uint8_t instance,
                                         const uint8_t* report, uint16_t len) {
    if (!started_ || !report || len == 0) {
        tuh_hid_receive_report(address, instance);
        return;
    }

    const uint16_t n = (len < sizeof(prev_raw_)) ? len : static_cast<uint16_t>(sizeof(prev_raw_));
    const bool changed = (n != prev_raw_len_) || (std::memcmp(prev_raw_, report, n) != 0);
    if (changed) {
        prev_raw_len_ = n;
        std::memcpy(prev_raw_, report, n);
        GameSirCyclone2Trace::set_receiver_controller_online(true);
        /* Rate-limit: one short line, no hex dump during early life. */
        OGXM_LOG("[CYCLONE2 RXR] HID raw len=%u (passive, not PadIn)\n",
                 static_cast<unsigned>(len));
    }

    tuh_hid_receive_report(address, instance);
}

bool Cyclone2HidPassive::send_feedback(uint8_t address, uint8_t instance) {
    (void)address;
    (void)instance;
    return true;
}

void Cyclone2HidPassive::disconnect() {
    reset();
}
