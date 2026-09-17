#include <cstdint>

#include "tusb.h"
#include "host/usbh.h"
#include "host/hcd.h"
#include "class/hid/hid_host.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostManager.h"
#include "OGXMini/OGXMini.h"
#include "Board/ogxm_log.h"
#include "USBHost/HostDriver/VictrixGambit/VictrixGambit.h"

#if defined(CONFIG_OGXM_DEBUG)
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2.h"
#include "USBHost/HostDriver/GameSirCyclone2/GameSirCyclone2Trace.h"
#endif

#if defined(CONFIG_OGXM_DEBUG)
namespace {

const char* host_type_name(HostDriverType t)
{
    switch (t) {
        case HostDriverType::UNKNOWN: return "UNKNOWN";
        case HostDriverType::SWITCH_PRO: return "SWITCH_PRO";
        case HostDriverType::SWITCH_PRO_2: return "SWITCH_PRO_2";
        case HostDriverType::SWITCH: return "SWITCH";
        case HostDriverType::PSCLASSIC: return "PSCLASSIC";
        case HostDriverType::DINPUT: return "DINPUT";
        case HostDriverType::PS3: return "PS3";
        case HostDriverType::PS4: return "PS4";
        case HostDriverType::PS5: return "PS5";
        case HostDriverType::N64: return "N64";
        case HostDriverType::FLYDIGI_APEX4_WUKONG: return "FLYDIGI_APEX4_WUKONG";
        case HostDriverType::GAMESIR_CYCLONE2: return "GAMESIR_CYCLONE2";
        case HostDriverType::GAMESIR_G7_PRO: return "GAMESIR_G7_PRO";
        case HostDriverType::VICTRIX_GAMBIT: return "VICTRIX_GAMBIT";
        case HostDriverType::XBOXOG: return "XBOXOG";
        case HostDriverType::XBOXONE: return "XBOXONE";
        case HostDriverType::XBOX360W: return "XBOX360W";
        case HostDriverType::XBOX360: return "XBOX360";
        case HostDriverType::XBOX360_CHATPAD: return "XBOX360_CHATPAD";
        case HostDriverType::HID_GENERIC: return "HID_GENERIC";
        default: return "?";
    }
}

void log_usb_driver_select(uint8_t address, uint8_t instance, HostManager::DriverClass dclass,
                           HostDriverType selected, uint16_t vid, uint16_t pid)
{
    (void)address;
    OGXM_LOG("[USB DRIVER SELECT] inst=%u %s %04X:%04X → %s sess=%s recv=%s\n",
             static_cast<unsigned>(instance),
             dclass == HostManager::DriverClass::XINPUT ? "XINPUT" : "HID",
             vid, pid, host_type_name(selected),
             GameSirCyclone2Trace::cyclone_session_active() ? "Y" : "N",
             GameSirCyclone2Trace::receiver_session_active() ? "Y" : "N");
}

} // namespace
#endif

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count) {
    *driver_count = 1;
    return tuh_xinput::class_driver();
}

#if defined(CONFIG_OGXM_DEBUG)
/* Cyclone attach/remove hooks — ISR-safe; no mutex logging / no USB sync from ISR. */
void tuh_event_hook_cb(uint8_t rhport, uint32_t eventid, bool in_isr) {
    if (eventid == HCD_EVENT_DEVICE_ATTACH) {
        GameSirCyclone2Trace::on_bus_attach(rhport, in_isr);
    } else if (eventid == HCD_EVENT_DEVICE_REMOVE) {
        GameSirCyclone2Trace::on_bus_remove(rhport, in_isr);
    }
}

void tuh_mount_cb(uint8_t daddr) {
    GameSirCyclone2Trace::on_device_configured(daddr);
}

void tuh_umount_cb(uint8_t daddr) {
    GameSirCyclone2Trace::on_device_unmounted(daddr);
}
#endif

//HID

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    HostManager& host_manager = HostManager::get_instance();
    HostDriverType host_type = HostManager::get_type({ vid, pid });

#if defined(CONFIG_OGXM_DEBUG)
    /* Session affinity + multi-presentation NS fingerprints (not global 057E:2009).
     * A: Gamepad / bcd 0x0326. B: Pro Controller / bcd 0x0116.
     * Dongle: sticky receiver affinity keeps remounts on GAMESIR_CYCLONE2. */
    if (GameSirCyclone2Host::should_claim(dev_addr, vid, pid)) {
        host_type = HostDriverType::GAMESIR_CYCLONE2;
    }
    log_usb_driver_select(dev_addr, instance, HostManager::DriverClass::HID, host_type, vid, pid);
#endif

    if (host_manager.setup_driver(host_type, HostManager::DriverClass::HID,
            dev_addr, instance, desc_report, desc_len)) {
        OGXMini::host_mounted(true);
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    HostManager& host_manager = HostManager::get_instance();
    host_manager.deinit_driver(HostManager::DriverClass::HID, dev_addr, instance);

    if (!host_manager.any_mounted()) {
        OGXMini::host_mounted(false);
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    HostManager::get_instance().process_report(HostManager::DriverClass::HID, dev_addr, instance, report, len);
}

//XINPUT

void tuh_xinput::mount_cb(uint8_t dev_addr, uint8_t instance, const tuh_xinput::Interface* interface) {
    HostManager& host_manager = HostManager::get_instance();
    HostDriverType host_type = HostManager::get_type(interface->dev_type);

    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    if (VictrixGambitHost::is_known_id(vid, pid)) {
        host_type = HostDriverType::VICTRIX_GAMBIT;
    }

#if defined(CONFIG_OGXM_DEBUG)
    GameSirCyclone2Trace::on_xinput_claimed(dev_addr, instance, interface->itf_num,
                                            interface->ep_in, interface->ep_out);
    if (GameSirCyclone2Host::is_known_id(vid, pid)) {
        host_type = HostDriverType::GAMESIR_CYCLONE2;
    }
    log_usb_driver_select(dev_addr, instance, HostManager::DriverClass::XINPUT, host_type, vid, pid);
    if (host_type == HostDriverType::VICTRIX_GAMBIT) {
        OGXM_LOG("\n[USB DRIVER SELECT]\n");
        OGXM_LOG("addr=%u\ninstance=%u\n\n", static_cast<unsigned>(dev_addr),
                 static_cast<unsigned>(instance));
        OGXM_LOG("VID=%04X\nPID=%04X\n\n", vid, pid);
        OGXM_LOG("selected=VICTRIX_GAMBIT\n\n");
        OGXM_LOG("protocol=XBOX_GIP\n\n");
    }
#endif

    if (host_manager.setup_driver(host_type, HostManager::DriverClass::XINPUT, dev_addr, instance)) {
        OGXMini::host_mounted(true, host_type);
    }
}

void tuh_xinput::unmount_cb(uint8_t dev_addr, uint8_t instance, const tuh_xinput::Interface* interface) {
    (void)interface;
    HostManager& host_manager = HostManager::get_instance();
    host_manager.deinit_driver(HostManager::DriverClass::XINPUT, dev_addr, instance);

    if (!host_manager.any_mounted()) {
        OGXMini::host_mounted(false);
    }
}

void tuh_xinput::report_received_cb(uint8_t dev_addr, uint8_t instance, const uint8_t* report, uint16_t len) {
    HostManager::get_instance().process_report(HostManager::DriverClass::XINPUT, dev_addr, instance, report, len);
}

void tuh_xinput::xbox360w_connect_cb(uint8_t dev_addr, uint8_t instance) {
    uint8_t idx = HostManager::get_instance().get_gamepad_idx(  HostManager::DriverClass::XINPUT, 
                                                                dev_addr, instance);
    OGXMini::wireless_connected(true, idx);
    HostManager::get_instance().connect_cb(HostManager::DriverClass::XINPUT, dev_addr, instance);
}

void tuh_xinput::xbox360w_disconnect_cb(uint8_t dev_addr, uint8_t instance) {
    uint8_t idx = HostManager::get_instance().get_gamepad_idx(  HostManager::DriverClass::XINPUT, 
                                                                dev_addr, instance);
    OGXMini::wireless_connected(false, idx);
    HostManager::get_instance().disconnect_cb(HostManager::DriverClass::XINPUT, dev_addr, instance);
}

void tuh_xinput::host_activity_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    HostManager::get_instance().record_usb_host_input_activity();
}
