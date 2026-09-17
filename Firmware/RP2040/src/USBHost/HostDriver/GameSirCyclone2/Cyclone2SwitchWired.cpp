#include "USBHost/HostDriver/GameSirCyclone2/Cyclone2SwitchWired.h"

#include "tusb.h"

#include "Board/ogxm_log.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Trace.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Transport.h"

void Cyclone2SwitchWired::reset() {
    started_ = false;
    GameSirCyclone2Trace::set_cyclone_switch_input_active(false);
}

void Cyclone2SwitchWired::start(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                 const uint8_t* report_desc, uint16_t desc_len) {
    started_ = true;

    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    const auto transport = GameSirCyclone2Trace::infer_usb_transport(vid, pid);

    OGXM_LOG("[CYCLONE2 RXR] Switch start transport=%s face_swap=armed\n",
             GameSirCyclone2Transport::transport_name(transport));

    GameSirCyclone2Trace::set_cyclone_switch_input_active(true);

    engine_.set_init_profile(SwitchProHost::InitProfile::MinimalReportMode);
    engine_.initialize(gamepad, address, instance, report_desc, desc_len);
}

void Cyclone2SwitchWired::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                          const uint8_t* report, uint16_t len) {
    if (!started_) {
        return;
    }
    engine_.process_report(gamepad, address, instance, report, len);
}

bool Cyclone2SwitchWired::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) {
    if (!started_) {
        return false;
    }
    return engine_.send_feedback(gamepad, address, instance);
}

void Cyclone2SwitchWired::disconnect(Gamepad& gamepad, uint8_t address, uint8_t instance) {
    if (started_) {
        engine_.disconnect_cb(gamepad, address, instance);
    }
    GameSirCyclone2Trace::set_cyclone_switch_input_active(false);
    started_ = false;
}
