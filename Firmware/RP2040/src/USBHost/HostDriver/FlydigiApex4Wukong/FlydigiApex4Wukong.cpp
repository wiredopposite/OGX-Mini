#include "USBHost/HostDriver/FlydigiApex4Wukong/FlydigiApex4Wukong.h"

#include <cstdio>
#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"
#include "pico/time.h"

#include "Board/Config.h"
#include "Board/ogxm_log.h"

#if defined(CONFIG_EN_USB_HOST)
#include "pio_usb.h"
#endif

namespace {

constexpr uint16_t kFlydigiV1Vid = 0x04B4;
constexpr uint16_t kFlydigiV1Pid = 0x2412;
constexpr uint16_t kXbox360GenericVid = 0x045E;
constexpr uint16_t kXbox360GenericPid = 0x028E;

constexpr uint8_t kFlydigiVendorItfPreferred = 2;
constexpr uint8_t kGetInfoReportId = 0x05;
constexpr uint8_t kGetInfoCommand = 0xEC;
constexpr uint16_t kGetInfoReplyLen = 32;

constexpr uint8_t kDeviceTypeApex4A = 84;   // 0x54 — SDL / retail APEX 4
constexpr uint8_t kDeviceTypeApex4B = 103;  // 0x67 — ApexSenseBridge-tested APEX 4

constexpr uint32_t kUnchangedReportLogMs = 1000;
constexpr uint8_t kGetInfoMaxAttempts = 8;

#if defined(CONFIG_EN_USB_HOST)
void service_usb_host()
{
    pio_usb_host_frame();
    tuh_task();
}
#else
void service_usb_host()
{
    tuh_task();
}
#endif

void utf16le_to_ascii(const uint8_t* buf, uint16_t buflen, char* out, size_t out_len)
{
    if (!out || out_len == 0)
        return;
    out[0] = '\0';
    if (!buf || buflen < 2)
        return;

    const uint8_t str_len = buf[0];
    if (str_len < 2 || buf[1] != 0x03)
        return;

    size_t o = 0;
    for (uint8_t i = 2; i + 1 < str_len && i + 1 < buflen && o + 1 < out_len; i += 2) {
        const uint8_t lo = buf[i];
        const uint8_t hi = buf[i + 1];
        out[o++] = (hi == 0 && lo >= 0x20 && lo < 0x7F) ? static_cast<char>(lo) : '?';
    }
    out[o] = '\0';
}

const char* xfer_type_name(uint8_t attr)
{
    switch (attr & 0x03u) {
        case 0: return "CTRL";
        case 1: return "ISO";
        case 2: return "BULK";
        case 3: return "INT";
        default: return "?";
    }
}

void log_string_descriptor(uint8_t daddr, uint8_t index, const char* label)
{
    if (index == 0) {
        OGXM_LOG("[Flydigi Probe] %s: (none)\n", label);
        return;
    }

    uint8_t buf[64]{};
    const uint8_t rc = tuh_descriptor_get_string_sync(daddr, index, 0x0409, buf, sizeof(buf));
    if (rc != XFER_RESULT_SUCCESS) {
        OGXM_LOG("[Flydigi Probe] %s: string fetch failed (idx=%u rc=%u)\n", label,
                 static_cast<unsigned>(index), static_cast<unsigned>(rc));
        return;
    }

    char ascii[48]{};
    utf16le_to_ascii(buf, sizeof(buf), ascii, sizeof(ascii));
    OGXM_LOG("[Flydigi Probe] %s: \"%s\"\n", label, ascii);
}

void dump_configuration_tree(uint8_t daddr)
{
    uint8_t cfg[512]{};
    const uint8_t rc = tuh_descriptor_get_configuration_sync(daddr, 0, cfg, sizeof(cfg));
    if (rc != XFER_RESULT_SUCCESS) {
        OGXM_LOG("[Flydigi Probe] configuration descriptor fetch failed (rc=%u)\n",
                 static_cast<unsigned>(rc));
        return;
    }

    if (sizeof(tusb_desc_configuration_t) > sizeof(cfg))
        return;

    auto const* conf = reinterpret_cast<tusb_desc_configuration_t const*>(cfg);
    const uint16_t total = tu_le16toh(conf->wTotalLength);
    const uint16_t len = TU_MIN(total, static_cast<uint16_t>(sizeof(cfg)));

    OGXM_LOG("[Flydigi Probe] bConfigurationValue=%u bNumInterfaces=%u wTotalLength=%u bmAttributes=0x%02x bMaxPower=%u\n",
             conf->bConfigurationValue, conf->bNumInterfaces, total, conf->bmAttributes, conf->bMaxPower);

    uint8_t const* p = tu_desc_next(cfg);
    uint8_t const* end = cfg + len;
    int current_itf = -1;

    while (p + 2 <= end) {
        const uint8_t dlen = tu_desc_len(p);
        const uint8_t dtype = tu_desc_type(p);
        if (dlen < 2 || p + dlen > end)
            break;

        if (dtype == TUSB_DESC_INTERFACE && dlen >= sizeof(tusb_desc_interface_t)) {
            auto const* itf = reinterpret_cast<tusb_desc_interface_t const*>(p);
            current_itf = itf->bInterfaceNumber;
            OGXM_LOG("[Flydigi Probe] -- Interface %u alt=%u class=0x%02x sub=0x%02x proto=0x%02x endpoints=%u\n",
                     itf->bInterfaceNumber, itf->bAlternateSetting, itf->bInterfaceClass,
                     itf->bInterfaceSubClass, itf->bInterfaceProtocol, itf->bNumEndpoints);
            if (itf->bInterfaceNumber == kFlydigiVendorItfPreferred) {
                OGXM_LOG("[Flydigi Probe]    *** MI_%02u present (preferred Flydigi V1 vendor itf) ***\n",
                         kFlydigiVendorItfPreferred);
            }
        } else if (dtype == TUSB_DESC_ENDPOINT && dlen >= sizeof(tusb_desc_endpoint_t)) {
            auto const* ep = reinterpret_cast<tusb_desc_endpoint_t const*>(p);
            const uint8_t addr = ep->bEndpointAddress;
            const uint8_t xfer = static_cast<uint8_t>(ep->bmAttributes.xfer);
            OGXM_LOG("[Flydigi Probe]    EP itf=%d addr=0x%02x %s type=%s maxpkt=%u interval=%u\n",
                     current_itf, addr, (addr & 0x80u) ? "IN" : "OUT",
                     xfer_type_name(xfer), static_cast<unsigned>(tu_edpt_packet_size(ep)),
                     static_cast<unsigned>(ep->bInterval));
        } else if (dtype == HID_DESC_TYPE_HID && dlen >= 9) {
            const uint16_t report_len = static_cast<uint16_t>(p[7] | (static_cast<uint16_t>(p[8]) << 8));
            OGXM_LOG("[Flydigi Probe]    HID desc (itf=%d) bcdHID=0x%02x%02x country=%u numDesc=%u report_len=%u\n",
                     current_itf, p[3], p[2], p[4], p[5], report_len);
        } else if (dtype == TUSB_DESC_INTERFACE_ASSOCIATION && dlen >= 8) {
            OGXM_LOG("[Flydigi Probe] -- IAD first_itf=%u count=%u class=0x%02x sub=0x%02x proto=0x%02x\n",
                     p[2], p[3], p[4], p[5], p[6]);
        }

        p = tu_desc_next(p);
    }
}

void dump_device_descriptor(uint8_t daddr)
{
    tusb_desc_device_t dev{};
    const uint8_t rc = tuh_descriptor_get_device_sync(daddr, &dev, sizeof(dev));
    if (rc != XFER_RESULT_SUCCESS) {
        OGXM_LOG("[Flydigi Probe] device descriptor fetch failed (rc=%u)\n", static_cast<unsigned>(rc));
        return;
    }

    OGXM_LOG("[Flydigi Probe] bcdUSB=0x%04x class=0x%02x sub=0x%02x proto=0x%02x maxpkt0=%u\n",
             tu_le16toh(dev.bcdUSB), dev.bDeviceClass, dev.bDeviceSubClass, dev.bDeviceProtocol,
             dev.bMaxPacketSize0);
    OGXM_LOG("[Flydigi Probe] idVendor=0x%04x idProduct=0x%04x bcdDevice=0x%04x\n",
             tu_le16toh(dev.idVendor), tu_le16toh(dev.idProduct), tu_le16toh(dev.bcdDevice));
    OGXM_LOG("[Flydigi Probe] iManufacturer=%u iProduct=%u iSerialNumber=%u bNumConfigurations=%u\n",
             dev.iManufacturer, dev.iProduct, dev.iSerialNumber, dev.bNumConfigurations);

    log_string_descriptor(daddr, dev.iManufacturer, "Manufacturer");
    log_string_descriptor(daddr, dev.iProduct, "Product");
    log_string_descriptor(daddr, dev.iSerialNumber, "Serial");
}

bool already_dumped_device(uint8_t daddr)
{
    static uint8_t dumped[CFG_TUH_DEVICE_MAX + 1]{};
    if (daddr > CFG_TUH_DEVICE_MAX)
        return true;
    if (dumped[daddr])
        return true;
    dumped[daddr] = 1;
    return false;
}

} // namespace

void FlydigiApex4WukongHost::log_xinput_candidate(uint8_t address, uint8_t instance)
{
#if !defined(CONFIG_OGXM_DEBUG)
    (void)address;
    (void)instance;
    return;
#else
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);

    OGXM_LOG("\n");
    OGXM_LOG("================================================\n");
    OGXM_LOG("FLYDIGI APEX 4 WUKONG PROBE\n");
    OGXM_LOG("================================================\n");
    OGXM_LOG("Candidate USB device detected (XInput path)\n");
    OGXM_LOG("VID: %04X\n", vid);
    OGXM_LOG("PID: %04X\n", pid);
    OGXM_LOG("TinyUSB XInput instance: %u\n", static_cast<unsigned>(instance));
    OGXM_LOG("NOTE: 045E:028E is generic Xbox 360 identity — NOT claiming as APEX 4.\n");
    OGXM_LOG("Existing Xbox360Host continues to own this interface.\n");
    OGXM_LOG("Inspecting USB tree for accompanying Flydigi vendor interfaces...\n");

    dump_device_descriptor(address);
    dump_configuration_tree(address);

    OGXM_LOG("If MI_02 / 04B4:2412 is absent, plug in DInput/Flydigi PC mode next.\n");
    OGXM_LOG("================================================\n\n");
#endif
}

void FlydigiApex4WukongHost::dump_hid_report_descriptor(const uint8_t* report_desc, uint16_t desc_len)
{
    OGXM_LOG("[Flydigi Probe] HID report descriptor length=%u\n", static_cast<unsigned>(desc_len));
    if (!report_desc || desc_len == 0) {
        OGXM_LOG("[Flydigi Probe] (no report descriptor provided on this mount)\n");
        return;
    }
    OGXM_LOG_HEX(report_desc, desc_len);
}

void FlydigiApex4WukongHost::dump_usb_tree(uint8_t address)
{
    if (already_dumped_device(address)) {
        OGXM_LOG("[Flydigi Probe] (device tree already dumped for addr=%u)\n",
                 static_cast<unsigned>(address));
        return;
    }
    dump_device_descriptor(address);
    dump_configuration_tree(address);
}

void FlydigiApex4WukongHost::dump_device_banner(uint8_t address, uint8_t instance,
                                                 const uint8_t* report_desc, uint16_t desc_len)
{
    uint16_t vid = 0, pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);

    tuh_itf_info_t itf_info{};
    bool have_itf = tuh_hid_itf_get_info(address, instance, &itf_info);
    if (have_itf)
        itf_num_ = itf_info.desc.bInterfaceNumber;

    OGXM_LOG("\n");
    OGXM_LOG("================================================\n");
    OGXM_LOG("FLYDIGI APEX 4 WUKONG PROBE\n");
    OGXM_LOG("================================================\n");
    OGXM_LOG("Candidate USB device detected\n");
    OGXM_LOG("VID: %04X\n", vid);
    OGXM_LOG("PID: %04X\n", pid);
    OGXM_LOG("HID instance: %u\n", static_cast<unsigned>(instance));
    if (have_itf) {
        OGXM_LOG("Interface: %u (alt=%u class=0x%02x sub=0x%02x proto=0x%02x)\n",
                 itf_info.desc.bInterfaceNumber, itf_info.desc.bAlternateSetting,
                 itf_info.desc.bInterfaceClass, itf_info.desc.bInterfaceSubClass,
                 itf_info.desc.bInterfaceProtocol);
    } else {
        OGXM_LOG("Interface: (tuh_hid_itf_get_info failed)\n");
    }

    dump_usb_tree(address);
    dump_hid_report_descriptor(report_desc, desc_len);

    OGXM_LOG("Searching for Flydigi vendor interface...\n");
    if (itf_num_ == kFlydigiVendorItfPreferred) {
        OGXM_LOG("This mount IS MI_%02u — will send GET_INFO (05 EC).\n", kFlydigiVendorItfPreferred);
    } else if (have_itf) {
        OGXM_LOG("This mount is MI_%02u (not preferred MI_%02u). Will still try GET_INFO if needed.\n",
                 itf_num_, kFlydigiVendorItfPreferred);
    }
    OGXM_LOG("================================================\n");
}

void FlydigiApex4WukongHost::try_send_get_info(uint8_t address, uint8_t instance)
{
    if (get_info_ok_ || get_info_attempts_ >= kGetInfoMaxAttempts)
        return;

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (next_get_info_ms_ != 0 && now < next_get_info_ms_)
        return;

    /* Prefer MI_02. On other interfaces, wait briefly so MI_02 can mount/send first. */
    if (itf_num_ != kFlydigiVendorItfPreferred && itf_num_ != 0xFF) {
        static uint32_t first_seen_ms[CFG_TUH_DEVICE_MAX + 1]{};
        if (address <= CFG_TUH_DEVICE_MAX) {
            if (first_seen_ms[address] == 0)
                first_seen_ms[address] = now;
            if ((now - first_seen_ms[address]) < 400u && get_info_attempts_ == 0)
                return;
        }
    }

    uint8_t request[12] = {
        kGetInfoReportId, kGetInfoCommand, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    OGXM_LOG("[Flydigi Probe] GET_INFO attempt %u on itf=%u instance=%u: ",
             static_cast<unsigned>(get_info_attempts_ + 1), static_cast<unsigned>(itf_num_),
             static_cast<unsigned>(instance));
    OGXM_LOG_HEX(request, sizeof(request));

    /* Full packet includes report ID (SDL hid_write style). */
    const bool ok = tuh_hid_send_report(address, instance, 0, request, sizeof(request));
    get_info_sent_ = true;
    ++get_info_attempts_;
    next_get_info_ms_ = now + 50u;
    OGXM_LOG("[Flydigi Probe] GET_INFO send %s\n", ok ? "queued" : "FAILED");
}

void FlydigiApex4WukongHost::handle_get_info_reply(const uint8_t* report, uint16_t len)
{
    if (len != kGetInfoReplyLen || report[15] != kGetInfoCommand)
        return;

    get_info_ok_ = true;

    const uint8_t device_type = report[3];
    const uint8_t fw_lo = report[9];
    const uint8_t fw_hi = report[10];
    const uint8_t connection = report[13];
    const uint8_t battery = report[11];
    const uint8_t cpu_type = report[12];
    const uint8_t motion = report[14];

    OGXM_LOG("\n");
    OGXM_LOG("================================================\n");
    OGXM_LOG("[Flydigi Probe]\n");
    OGXM_LOG("VID: %04X\n", kFlydigiV1Vid);
    OGXM_LOG("PID: %04X\n", kFlydigiV1Pid);
    OGXM_LOG("Interface: %u\n", static_cast<unsigned>(itf_num_));
    OGXM_LOG("Flydigi GET_INFO response received\n");
    OGXM_LOG("DeviceType: %u (0x%02X)\n", device_type, device_type);
    OGXM_LOG("Firmware: %u.%u (raw lo=0x%02X hi=0x%02X packed=0x%04X)\n",
             static_cast<unsigned>(fw_hi), static_cast<unsigned>(fw_lo), fw_lo, fw_hi,
             static_cast<unsigned>((fw_hi << 8) | fw_lo));
    OGXM_LOG("Identity bytes: %02X %02X %02X %02X\n", report[5], report[6], report[7], report[8]);
    OGXM_LOG("Battery: 0x%02X  CPUType: 0x%02X  MotionSensor: 0x%02X\n", battery, cpu_type, motion);

    if (connection == 0)
        OGXM_LOG("Connection: Wireless (0)\n");
    else if (connection == 1)
        OGXM_LOG("Connection: Wired (1)\n");
    else
        OGXM_LOG("Connection: Unknown (%u)\n", connection);

    OGXM_LOG("Full GET_INFO reply:\n");
    OGXM_LOG_HEX(report, len);

    if (device_type == kDeviceTypeApex4A || device_type == kDeviceTypeApex4B) {
        OGXM_LOG("\nPossible Flydigi APEX 4 / APEX 4 Wukong detected (known DeviceType).\n");
    } else {
        OGXM_LOG("\nUNKNOWN Flydigi DeviceType: 0x%02X\n", device_type);
        OGXM_LOG("*** CAPTURE THIS DEVICE ID — POSSIBLE APEX 4 WUKONG SKU ***\n");
    }

    OGXM_LOG("Starting raw input capture...\n");
    OGXM_LOG("================================================\n\n");
}

void FlydigiApex4WukongHost::log_raw_report(uint8_t address, uint8_t instance,
                                              const uint8_t* report, uint16_t len)
{
    const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    const uint16_t n = (len < sizeof(prev_report_)) ? len : static_cast<uint16_t>(sizeof(prev_report_));
    const bool changed =
        (n != prev_len_) || (std::memcmp(prev_report_, report, n) != 0);

    if (!changed) {
        if ((now_ms - last_unchanged_log_ms_) < kUnchangedReportLogMs)
            return;
        last_unchanged_log_ms_ = now_ms;
        OGXM_LOG("[Flydigi Probe] raw unchanged sample t=%lu itf=%u inst=%u len=%u\n",
                 static_cast<unsigned long>(now_ms), static_cast<unsigned>(itf_num_),
                 static_cast<unsigned>(instance), static_cast<unsigned>(len));
        return;
    }

    last_unchanged_log_ms_ = now_ms;
    prev_len_ = n;
    std::memcpy(prev_report_, report, n);

    OGXM_LOG("[Flydigi Probe] RAW t=%lu addr=%u itf=%u inst=%u len=%u\n",
             static_cast<unsigned long>(now_ms), static_cast<unsigned>(address),
             static_cast<unsigned>(itf_num_), static_cast<unsigned>(instance),
             static_cast<unsigned>(len));
    if (len >= 2 && report[0] == 0x04 && report[1] == 0xFE)
        OGXM_LOG("[Flydigi Probe] (matches known Flydigi V1 state prefix 04 FE)\n");
    OGXM_LOG_HEX(report, len);
}

void FlydigiApex4WukongHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                          const uint8_t* report_desc, uint16_t desc_len)
{
    (void)gamepad;
    address_ = address;
    instance_ = instance;
    get_info_sent_ = false;
    get_info_ok_ = false;
    get_info_attempts_ = 0;
    next_get_info_ms_ = 0;
    prev_len_ = 0;
    std::memset(prev_report_, 0, sizeof(prev_report_));

#if defined(CONFIG_OGXM_DEBUG)
    /* Brief settle so composite siblings can finish open before config dump. */
    for (int i = 0; i < 20; ++i) {
        service_usb_host();
        sleep_ms(1);
    }

    dump_device_banner(address, instance, report_desc, desc_len);
    try_send_get_info(address, instance);
#else
    (void)report_desc;
    (void)desc_len;
#endif

    tuh_hid_receive_report(address, instance);
}

void FlydigiApex4WukongHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                              const uint8_t* report, uint16_t len)
{
    (void)gamepad;

#if defined(CONFIG_OGXM_DEBUG)
    if (!get_info_ok_)
        handle_get_info_reply(report, len);

    /* Retry GET_INFO a few times if no valid reply yet (SDL retries too). */
    if (!get_info_ok_)
        try_send_get_info(address, instance);

    log_raw_report(address, instance, report, len);
#else
    (void)address;
    (void)instance;
    (void)report;
    (void)len;
#endif

    tuh_hid_receive_report(address, instance);
}

bool FlydigiApex4WukongHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;
    (void)address;
    (void)instance;
    return true;
}
