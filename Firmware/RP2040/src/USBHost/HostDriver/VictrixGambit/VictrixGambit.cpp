#include <algorithm>
#include <cstdio>
#include <cstring>
#include <tuple>

#include "host/usbh.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"
#include "TaskQueue/TaskQueue.h"

#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput.h"
#include "USBHost/HostDriver/XInput/tuh_xinput/tuh_xinput_cmd.h"
#include "USBHost/HostDriver/VictrixGambit/VictrixGambit.h"
#include "Descriptors/XboxOne.h"

namespace {

VictrixGambitHost* g_active_gambit = nullptr;

constexpr uint32_t kGuideStaleMs = 400u;
constexpr uint32_t kGuideOrphanMs = 5000u;
constexpr uint32_t kSubmitRetryMs = 15u;
constexpr uint32_t kStatusPeriodMs = 1000u;
constexpr uint8_t kMaxSubmitRetries = 40;

constexpr uint16_t kShareAbsOffsetDoc = 32;
constexpr uint8_t kShareBit = 0x01;

/** xone gip_pkt_announce payload size (28). */
constexpr uint16_t kAnnouncePayloadLen = 28;

uint16_t gip_logical_len(const uint8_t* report, uint16_t usb_len)
{
    if (usb_len < 4)
    {
        return usb_len;
    }
    const uint16_t declared = static_cast<uint16_t>(4u + report[3]);
    return (declared < usb_len) ? declared : usb_len;
}

bool read_share_documented_02d6(const uint8_t* report, uint16_t usb_len)
{
    const uint16_t logical = gip_logical_len(report, usb_len);
    if (logical >= (kShareAbsOffsetDoc + 1) && report[3] >= 46)
    {
        return (report[kShareAbsOffsetDoc] & kShareBit) != 0;
    }
    return false;
}

void log_hex_line(const uint8_t* data, uint16_t len)
{
    char line[196]{};
    size_t o = 0;
    const uint16_t n = (len > 64) ? 64 : len;
    for (uint16_t i = 0; i < n && o + 4 < sizeof(line); ++i)
    {
        o += static_cast<size_t>(snprintf(line + o, sizeof(line) - o, "%02X ", data[i]));
    }
    OGXM_LOG("%s%s\n", line, (len > 64) ? "..." : "");
}

uint16_t read_le16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

const char* gip_cmd_label(uint8_t cmd)
{
    using namespace tuh_xinput::XboxOne;
    switch (cmd)
    {
        case GIP_CMD_ACK: return "ACK";
        case GIP_CMD_ANNOUNCE: return "ANNOUNCE";
        case GIP_CMD_IDENTIFY: return "IDENTIFY";
        case GIP_CMD_POWER: return "POWER";
        case GIP_CMD_AUTHENTICATE: return "AUTH";
        case GIP_CMD_VIRTUAL_KEY: return "VIRTUAL_KEY";
        case GIP_CMD_LED: return "LED";
        case GIP_CMD_INPUT: return "INPUT";
        default: return "OTHER";
    }
}

void log_string_index(uint8_t address, uint8_t index, const char* label)
{
    if (index == 0)
    {
        OGXM_LOG("%s=(none)\n", label);
        return;
    }
    uint8_t buf[64]{};
    if (tuh_descriptor_get_string_sync(address, index, 0x0409, buf, sizeof(buf)) != XFER_RESULT_SUCCESS)
    {
        OGXM_LOG("%s=(fetch failed idx=%u)\n", label, static_cast<unsigned>(index));
        return;
    }
    char ascii[48]{};
    size_t o = 0;
    const uint8_t blen = buf[0];
    for (uint8_t i = 2; i + 1 < blen && i < sizeof(buf) && o + 1 < sizeof(ascii); i += 2)
    {
        ascii[o++] = (buf[i] >= 32 && buf[i] < 127) ? static_cast<char>(buf[i]) : '?';
    }
    ascii[o] = '\0';
    OGXM_LOG("%s=%s\n", label, ascii);
}

const char* endpoint_type_name(uint8_t xfer)
{
    switch (xfer)
    {
        case TUSB_XFER_CONTROL: return "CONTROL";
        case TUSB_XFER_ISOCHRONOUS: return "ISOCHRONOUS";
        case TUSB_XFER_BULK: return "BULK";
        case TUSB_XFER_INTERRUPT: return "INTERRUPT";
        default: return "UNKNOWN";
    }
}

void dump_config_interfaces(uint8_t address)
{
    uint8_t cfg_buf[512]{};
    if (tuh_descriptor_get_configuration_sync(address, 0, cfg_buf, sizeof(cfg_buf)) != XFER_RESULT_SUCCESS)
    {
        OGXM_LOG("configuration descriptor=(fetch failed)\n");
        return;
    }

    auto const* conf = reinterpret_cast<tusb_desc_configuration_t const*>(cfg_buf);
    uint16_t total = tu_le16toh(conf->wTotalLength);
    if (total > sizeof(cfg_buf))
    {
        OGXM_LOG("configuration wTotalLength=%u (truncated to %u)\n",
                 static_cast<unsigned>(total), static_cast<unsigned>(sizeof(cfg_buf)));
        total = sizeof(cfg_buf);
    }
    else
    {
        OGXM_LOG("configuration wTotalLength=%u\n", static_cast<unsigned>(total));
    }

    uint16_t offset = conf->bLength;
    while (offset + sizeof(tusb_desc_interface_t) <= total)
    {
        auto const* d = reinterpret_cast<tusb_desc_interface_t const*>(cfg_buf + offset);
        if (d->bDescriptorType == TUSB_DESC_INTERFACE)
        {
            OGXM_LOG("\nInterface %u\n", d->bInterfaceNumber);
            OGXM_LOG("Alt %u\n", d->bAlternateSetting);
            OGXM_LOG("num_endpoints=%u\n", d->bNumEndpoints);
            OGXM_LOG("class=0x%02X\n", d->bInterfaceClass);
            OGXM_LOG("subclass=0x%02X\n", d->bInterfaceSubClass);
            OGXM_LOG("protocol=0x%02X\n", d->bInterfaceProtocol);

            uint16_t ep_off = offset + d->bLength;
            uint8_t eps_seen = 0;
            while (ep_off + sizeof(tusb_desc_endpoint_t) <= total && eps_seen < d->bNumEndpoints)
            {
                auto const* ed = reinterpret_cast<tusb_desc_endpoint_t const*>(cfg_buf + ep_off);
                if (ed->bDescriptorType == TUSB_DESC_ENDPOINT)
                {
                    const bool is_in = (ed->bEndpointAddress & TUSB_DIR_IN_MASK) != 0;
                    OGXM_LOG("endpoint_type=%s\n", endpoint_type_name(ed->bmAttributes.xfer));
                    OGXM_LOG("%s endpoint=0x%02X\n", is_in ? "IN" : "OUT", ed->bEndpointAddress);
                    OGXM_LOG("max packet=%u\n", tu_edpt_packet_size(ed));
                    OGXM_LOG("interval=%u\n", ed->bInterval);
                    ++eps_seen;
                }
                if (ed->bLength == 0)
                {
                    break;
                }
                ep_off = static_cast<uint16_t>(ep_off + ed->bLength);
            }
        }
        if (d->bLength == 0)
        {
            break;
        }
        offset = static_cast<uint16_t>(offset + d->bLength);
    }
}

} // namespace

void VictrixGambitHost::on_out_xfer_complete(uint8_t address, uint8_t instance, bool success,
                                             const uint8_t* data, uint16_t len)
{
    if (!g_active_gambit || g_active_gambit->address_ != address ||
        g_active_gambit->instance_ != instance)
    {
        return;
    }
    g_active_gambit->handle_out_complete(success, data, len);
}

void VictrixGambitHost::on_in_xfer_result(uint8_t address, uint8_t instance, bool success,
                                          uint16_t len)
{
    if (!g_active_gambit || g_active_gambit->address_ != address ||
        g_active_gambit->instance_ != instance)
    {
        return;
    }
    if (success)
    {
        ++g_active_gambit->rx_complete_;
    }
    (void)len;
}

void VictrixGambitHost::on_set_interface_complete(uint8_t daddr, bool success, xfer_result_t result,
                                                  uintptr_t user_data)
{
    (void)user_data;
    if (!g_active_gambit || g_active_gambit->address_ != daddr)
    {
        return;
    }
    g_active_gambit->handle_set_interface_complete(success, result);
}

const char* VictrixGambitHost::state_name(InitState s) const
{
    switch (s)
    {
        case InitState::Idle: return "IDLE";
        case InitState::DisableAudioInterface: return "DISABLE_AUDIO_INTERFACE";
        case InitState::WaitAudioInterfaceComplete: return "WAIT_AUDIO_INTERFACE_COMPLETE";
        case InitState::WaitAnnounce: return "WAIT_ANNOUNCE";
        case InitState::SendIdentify: return "SEND_IDENTIFY";
        case InitState::WaitIdentifyTxComplete: return "WAIT_IDENTIFY_TX_COMPLETE";
        case InitState::WaitIdentifyResponse: return "WAIT_IDENTIFY_RESPONSE";
        case InitState::GamepadIdentified: return "GAMEPAD_IDENTIFIED";
        case InitState::SendPower: return "SEND_POWER";
        case InitState::WaitPowerComplete: return "WAIT_POWER_COMPLETE";
        case InitState::SendLed: return "SEND_LED";
        case InitState::WaitLedComplete: return "WAIT_LED_COMPLETE";
        case InitState::SendAuth: return "SEND_AUTH";
        case InitState::WaitAuthComplete: return "WAIT_AUTH_COMPLETE";
        case InitState::WaitForInput: return "WAIT_FOR_INPUT";
        case InitState::Ready: return "READY";
        case InitState::InitFailed: return "INIT_FAILED";
        default: return "?";
    }
}

const char* VictrixGambitHost::cmd_name(PendingCmd c) const
{
    switch (c)
    {
        case PendingCmd::PowerOn: return "POWER_ON";
        case PendingCmd::LedEnable: return "LED_ENABLE";
        case PendingCmd::AuthDone: return "AUTH_DONE";
        case PendingCmd::Identify: return "IDENTIFY";
        default: return "NONE";
    }
}

void VictrixGambitHost::set_state(InitState s)
{
    state_ = s;
    OGXM_LOG("[GAMBIT INIT]\nstate=%s\n\n", state_name(s));
}

bool VictrixGambitHost::arm_rx(const char* why, bool quiet)
{
    ++rx_arm_attempts_;
    const char* reason = nullptr;
    const bool ok = tuh_xinput::arm_gip_in(address_, instance_, &reason);
    uint8_t ep_in = 0x81;
    tuh_xinput::get_endpoint_info(address_, instance_, nullptr, &ep_in, nullptr, nullptr, nullptr);

    if (ok)
    {
        ++rx_arm_ok_;
    }
    else
    {
        ++rx_arm_fail_;
    }

    /* Log mount/post-auth/keepalive always; hot-path rearm only on failure or first few. */
    const bool log_it = !quiet || !ok || rx_arm_attempts_ <= 8;
    if (log_it)
    {
        OGXM_LOG("[GAMBIT GIP RX ARM]\n");
        OGXM_LOG("why=%s\n", why ? why : "?");
        OGXM_LOG("addr=%u\ninstance=%u\n", static_cast<unsigned>(address_),
                 static_cast<unsigned>(instance_));
        OGXM_LOG("endpoint=0x%02X\n", ep_in);
        OGXM_LOG("result=%s\n\n", ok ? (reason ? reason : "SUCCESS") : (reason ? reason : "FAILED"));
    }
    return ok;
}

void VictrixGambitHost::begin_disable_audio_interface()
{
    constexpr uint8_t kAudioItf = 1;
    constexpr uint8_t kTargetAlt = 0;

    uint8_t current_alt = 0xFF;
    const bool got_alt = tuh_xinput::get_interface_alt(address_, kAudioItf, &current_alt);

    OGXM_LOG("[GAMBIT USB AUDIO]\n");
    OGXM_LOG("interface=%u\n", static_cast<unsigned>(kAudioItf));
    if (got_alt)
    {
        OGXM_LOG("current_alt=%u\n", static_cast<unsigned>(current_alt));
    }
    else
    {
        OGXM_LOG("current_alt=(GET_INTERFACE failed)\n");
    }
    OGXM_LOG("target_alt=%u\n\n", static_cast<unsigned>(kTargetAlt));

    set_state(InitState::WaitAudioInterfaceComplete);

    OGXM_LOG("[GAMBIT USB AUDIO]\n");
    OGXM_LOG("SET_INTERFACE request\n");
    OGXM_LOG("interface=%u\n", static_cast<unsigned>(kAudioItf));
    OGXM_LOG("alt=%u\n\n", static_cast<unsigned>(kTargetAlt));

    if (!tuh_xinput::disable_gip_audio_interface(address_, kAudioItf, kTargetAlt,
                                                 &VictrixGambitHost::on_set_interface_complete,
                                                 0))
    {
        OGXM_LOG("[GAMBIT USB AUDIO]\n");
        OGXM_LOG("SET_INTERFACE failed\n");
        OGXM_LOG("status=SUBMIT_FAILED\n\n");
        set_state(InitState::InitFailed);
    }
}

void VictrixGambitHost::handle_set_interface_complete(bool success, xfer_result_t result)
{
    if (state_ != InitState::WaitAudioInterfaceComplete)
    {
        return;
    }

    OGXM_LOG("[GAMBIT USB AUDIO]\n");
    OGXM_LOG("SET_INTERFACE request\n");
    OGXM_LOG("interface=1\n");
    OGXM_LOG("alt=0\n");
    if (success)
    {
        OGXM_LOG("status=SUCCESS\n\n");
        enter_wait_announce();
        return;
    }

    OGXM_LOG("SET_INTERFACE failed\n");
    OGXM_LOG("status=%u\n\n", static_cast<unsigned>(result));
    set_state(InitState::InitFailed);
}

void VictrixGambitHost::enter_wait_announce()
{
    arm_rx("post_audio_disable");
    wait_announce_start_ms_ = board_api::ms_since_boot();
    set_state(InitState::WaitAnnounce);
    OGXM_LOG("[GAMBIT INIT]\nWAIT_ANNOUNCE — no POWER/LED/AUTH until device announces\n\n");
}

void VictrixGambitHost::log_enumeration(uint8_t address, uint8_t instance)
{
    if (enum_log_done_)
    {
        return;
    }
    enum_log_done_ = true;

    uint16_t vid = 0;
    uint16_t pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    pid_ = pid;

    uint8_t itf = 0xFF;
    uint8_t ep_in = 0xFF;
    uint8_t ep_out = 0xFF;
    uint16_t ep_in_sz = 0;
    uint16_t ep_out_sz = 0;
    tuh_xinput::get_endpoint_info(address, instance, &itf, &ep_in, &ep_out, &ep_in_sz, &ep_out_sz);

    tusb_desc_device_t desc{};
    uint16_t bcd = 0;
    uint8_t num_cfg = 0;
    uint8_t dev_class = 0;
    if (tuh_descriptor_get_device_sync(address, &desc, sizeof(desc)) == XFER_RESULT_SUCCESS)
    {
        bcd = desc.bcdDevice;
        num_cfg = desc.bNumConfigurations;
        dev_class = desc.bDeviceClass;
    }

    OGXM_LOG("\n================================================\n");
    OGXM_LOG("VICTRIX GAMBIT ENUMERATION\n");
    OGXM_LOG("================================================\n\n");
    OGXM_LOG("VID=%04X\nPID=%04X\nbcdDevice=%04X\n", vid, pid, bcd);
    log_string_index(address, desc.iManufacturer, "Manufacturer");
    log_string_index(address, desc.iProduct, "Product");
    log_string_index(address, desc.iSerialNumber, "Serial");
    OGXM_LOG("Device class=0x%02X\nConfiguration count=%u\n", dev_class,
             static_cast<unsigned>(num_cfg));
    dump_config_interfaces(address);
    OGXM_LOG("\nclaimed gamepad interface=%u\n", static_cast<unsigned>(itf));
    OGXM_LOG("claimed IN endpoint=0x%02X (max=%u)\n", ep_in, ep_in_sz);
    OGXM_LOG("claimed OUT endpoint=0x%02X (max=%u)\n", ep_out, ep_out_sz);
    OGXM_LOG("physical_driver=VICTRIX_GAMBIT\n");
    OGXM_LOG("================================================\n\n");
}

void VictrixGambitHost::log_perf()
{
    if (state_ == InitState::WaitAnnounce)
    {
        const uint32_t elapsed = board_api::ms_since_boot() - wait_announce_start_ms_;
        uint8_t ep_in = 0x81;
        tuh_xinput::get_endpoint_info(address_, instance_, nullptr, &ep_in, nullptr, nullptr, nullptr);
        const char* arm_reason = nullptr;
        const bool armed = tuh_xinput::arm_gip_in(address_, instance_, &arm_reason);
        OGXM_LOG("[GAMBIT INIT]\n");
        OGXM_LOG("state=WAIT_ANNOUNCE\n");
        OGXM_LOG("rx_armed=%s\n", armed ? "YES" : "NO");
        if (arm_reason)
        {
            OGXM_LOG("rx_arm_detail=%s\n", arm_reason);
        }
        OGXM_LOG("endpoint=0x%02X\n", ep_in);
        OGXM_LOG("rx_packets=%lu\n", static_cast<unsigned long>(rx_packets_));
        OGXM_LOG("elapsed_ms=%lu\n\n", static_cast<unsigned long>(elapsed));
        return;
    }

    OGXM_LOG("[GAMBIT PERF]\n");
    OGXM_LOG("rx_arm_attempts=%lu\n", static_cast<unsigned long>(rx_arm_attempts_));
    OGXM_LOG("rx_arm_ok=%lu\n", static_cast<unsigned long>(rx_arm_ok_));
    OGXM_LOG("rx_arm_fail=%lu\n", static_cast<unsigned long>(rx_arm_fail_));
    OGXM_LOG("rx_complete=%lu\n", static_cast<unsigned long>(rx_complete_));
    OGXM_LOG("rx_packets=%lu\n", static_cast<unsigned long>(rx_packets_));
    OGXM_LOG("tx_complete=%lu\n", static_cast<unsigned long>(tx_completed_));
    OGXM_LOG("identify_requested=%u\n", identify_requested_ ? 1u : 0u);
    OGXM_LOG("state=%s\n\n", state_name(state_));

    if (!gip_traffic_seen_ && state_ != InitState::InitFailed && state_ != InitState::Idle)
    {
        arm_rx("perf_keepalive");
    }
}

void VictrixGambitHost::schedule_status_tick()
{
    if (status_task_id_ != 0)
    {
        TaskQueue::Core1::cancel_delayed_task(status_task_id_);
        status_task_id_ = 0;
    }
    if (state_ == InitState::Ready || state_ == InitState::InitFailed || state_ == InitState::Idle)
    {
        return;
    }
    const uint8_t gen = init_gen_;
    status_task_id_ = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(
        status_task_id_, kStatusPeriodMs, false,
        [this, gen]() {
            status_task_id_ = 0;
            if (gen != init_gen_)
            {
                return;
            }
            log_perf();
            schedule_status_tick();
        });
}

void VictrixGambitHost::schedule_retry_submit(uint32_t delay_ms)
{
    if (retry_task_id_ != 0)
    {
        TaskQueue::Core1::cancel_delayed_task(retry_task_id_);
        retry_task_id_ = 0;
    }
    const uint8_t gen = init_gen_;
    retry_task_id_ = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(
        retry_task_id_, delay_ms, false,
        [this, gen]() {
            retry_task_id_ = 0;
            if (gen != init_gen_)
            {
                return;
            }
            try_submit_current();
        });
}

void VictrixGambitHost::try_submit_current()
{
    using namespace tuh_xinput::XboxOne;

    const uint8_t* src = nullptr;
    uint16_t src_len = 0;
    PendingCmd cmd = PendingCmd::None;
    InitState wait_state = InitState::Idle;

    switch (state_)
    {
        case InitState::SendPower:
            src = POWER_ON;
            src_len = sizeof(POWER_ON);
            cmd = PendingCmd::PowerOn;
            wait_state = InitState::WaitPowerComplete;
            break;
        case InitState::SendLed:
            src = S_LED_INIT;
            src_len = sizeof(S_LED_INIT);
            cmd = PendingCmd::LedEnable;
            wait_state = InitState::WaitLedComplete;
            break;
        case InitState::SendAuth:
            src = PDP_AUTH;
            src_len = sizeof(PDP_AUTH);
            cmd = PendingCmd::AuthDone;
            wait_state = InitState::WaitAuthComplete;
            break;
        case InitState::SendIdentify:
            src = IDENTIFY_REQ;
            src_len = sizeof(IDENTIFY_REQ);
            cmd = PendingCmd::Identify;
            wait_state = InitState::WaitIdentifyTxComplete;
            break;
        default:
            return;
    }

    if (!tuh_xinput::out_endpoint_ready(address_, instance_))
    {
        if (++submit_retries_ > kMaxSubmitRetries)
        {
            OGXM_LOG("[GAMBIT GIP TX FAILED]\ncommand=%s\nreason=endpoint_busy_timeout\n\n",
                     cmd_name(cmd));
            ++tx_failed_;
            set_state(InitState::InitFailed);
            return;
        }
        schedule_retry_submit(kSubmitRetryMs);
        return;
    }

    pending_wire_.fill(0);
    std::memcpy(pending_wire_.data(), src, src_len);
    pending_seq_ = tuh_xinput::current_gip_seq(address_, instance_);
    pending_len_ = src_len;
    pending_cmd_ = cmd;
    pending_wire_[2] = pending_seq_;

    uint8_t ep_out = 0x01;
    tuh_xinput::get_endpoint_info(address_, instance_, nullptr, nullptr, &ep_out, nullptr, nullptr);

    OGXM_LOG("[GAMBIT GIP TX REQUEST]\n");
    OGXM_LOG("command=%s\n", cmd_name(cmd));
    OGXM_LOG("addr=%u\ninstance=%u\nendpoint=0x%02X\ngip_seq=%u\nlen=%u\ndata:\n",
             static_cast<unsigned>(address_), static_cast<unsigned>(instance_), ep_out,
             static_cast<unsigned>(pending_seq_), static_cast<unsigned>(src_len));
    log_hex_line(pending_wire_.data(), src_len);
    OGXM_LOG("\n");

    ++tx_requested_;
    if (!tuh_xinput::send_gip_out(address_, instance_, src, src_len, true))
    {
        OGXM_LOG("[GAMBIT GIP TX FAILED]\ncommand=%s\nreason=submit_failed\n\n", cmd_name(cmd));
        ++tx_failed_;
        pending_cmd_ = PendingCmd::None;
        if (++submit_retries_ > kMaxSubmitRetries)
        {
            set_state(InitState::InitFailed);
            return;
        }
        schedule_retry_submit(kSubmitRetryMs);
        return;
    }

    if (cmd == PendingCmd::PowerOn)
    {
        tuh_xinput::mark_gip_power_sent(address_, instance_);
    }

    submit_retries_ = 0;
    set_state(wait_state);
}

void VictrixGambitHost::handle_out_complete(bool success, const uint8_t* data, uint16_t len)
{
    if (pending_cmd_ == PendingCmd::None)
    {
        return;
    }

    const PendingCmd done = pending_cmd_;
    pending_cmd_ = PendingCmd::None;

    if (!success)
    {
        ++tx_failed_;
        OGXM_LOG("[GAMBIT GIP TX FAILED]\ncommand=%s\nreason=xfer_failed\nbytes=%u\n\n",
                 cmd_name(done), static_cast<unsigned>(len));
        switch (done)
        {
            case PendingCmd::PowerOn: set_state(InitState::SendPower); break;
            case PendingCmd::LedEnable: set_state(InitState::SendLed); break;
            case PendingCmd::AuthDone: set_state(InitState::SendAuth); break;
            case PendingCmd::Identify: set_state(InitState::SendIdentify); break;
            default: set_state(InitState::InitFailed); return;
        }
        if (++submit_retries_ > kMaxSubmitRetries)
        {
            set_state(InitState::InitFailed);
            return;
        }
        schedule_retry_submit(kSubmitRetryMs);
        return;
    }

    ++tx_completed_;
    OGXM_LOG("[GAMBIT GIP TX COMPLETE]\ncommand=%s\nstatus=SUCCESS\nbytes=%u\n",
             cmd_name(done), static_cast<unsigned>(len));
    if (data && len)
    {
        OGXM_LOG("data:\n");
        log_hex_line(data, len);
    }
    if (done == PendingCmd::LedEnable)
    {
        OGXM_LOG("LED command transfer complete: YES\n");
    }
    OGXM_LOG("\n");

    submit_retries_ = 0;

    switch (done)
    {
        case PendingCmd::PowerOn:
            set_state(InitState::SendLed);
            try_submit_current();
            break;
        case PendingCmd::LedEnable:
            set_state(InitState::SendAuth);
            try_submit_current();
            break;
        case PendingCmd::AuthDone:
            set_state(InitState::WaitForInput);
            OGXM_LOG("[GAMBIT INIT]\nWAIT_FOR_INPUT — listening on EP 0x81\n\n");
            arm_rx("post_auth");
            break;
        case PendingCmd::Identify:
            set_state(InitState::WaitIdentifyResponse);
            OGXM_LOG("[GAMBIT INIT]\nWAIT_IDENTIFY_RESPONSE — listening for descriptor\n\n");
            arm_rx("post_identify");
            break;
        default:
            break;
    }
}

bool VictrixGambitHost::decode_gip_header(const uint8_t* data, uint16_t len, GipHdr& out) const
{
    using namespace tuh_xinput::XboxOne;
    if (!data || len < 4)
    {
        return false;
    }

    out.command = data[0];
    out.options = data[1];
    out.sequence = data[2];
    out.packet_length = 0;
    out.chunk_offset = 0;

    auto decode_varint = [](const uint8_t* p, uint16_t avail, uint32_t* val) -> int {
        *val = 0;
        int i = 0;
        for (; i < 4 && i < static_cast<int>(avail); ++i)
        {
            *val |= static_cast<uint32_t>(p[i] & 0x7Fu) << (i * 7);
            if ((p[i] & 0x80u) == 0)
            {
                return i + 1;
            }
        }
        return i;
    };

    uint16_t i = 3;
    int n = decode_varint(data + i, static_cast<uint16_t>(len - i), &out.packet_length);
    if (n <= 0)
    {
        return false;
    }
    i = static_cast<uint16_t>(i + n);

    if (out.options & GIP_OPT_CHUNK)
    {
        n = decode_varint(data + i, static_cast<uint16_t>(len - i), &out.chunk_offset);
        if (n <= 0)
        {
            return false;
        }
        i = static_cast<uint16_t>(i + n);
    }

    out.hdr_len = static_cast<uint8_t>(i);
    return out.hdr_len >= 4 && out.hdr_len <= len;
}

void VictrixGambitHost::maybe_ack(const uint8_t* report, uint16_t len, const GipHdr& hdr,
                                  uint16_t remaining)
{
    using namespace tuh_xinput::XboxOne;
    if ((hdr.options & GIP_OPT_ACK) == 0)
    {
        return;
    }
    const uint16_t bytes_received =
        static_cast<uint16_t>(hdr.chunk_offset + hdr.packet_length);
    tuh_xinput::send_gip_ack_if_requested(address_, instance_, report, len, bytes_received,
                                          remaining);
}

void VictrixGambitHost::maybe_log_rx_packet(const uint8_t* report, uint16_t len, const GipHdr* hdr)
{
    if (!report || len == 0)
    {
        return;
    }

    using namespace tuh_xinput::XboxOne;
    const uint8_t cmd = report[0];
    const bool always =
        (cmd == GIP_CMD_IDENTIFY) || (cmd == GIP_CMD_ANNOUNCE && !announce_logged_) ||
        (state_ == InitState::WaitIdentifyResponse && !identify_rx_logged_);
    if (!always)
    {
        if (rx_log_remaining_ == 0)
        {
            return;
        }
        --rx_log_remaining_;
    }

    uint8_t ep_in = 0x81;
    tuh_xinput::get_endpoint_info(address_, instance_, nullptr, &ep_in, nullptr, nullptr, nullptr);

    const uint8_t options = hdr ? hdr->options : ((len >= 2) ? report[1] : 0);

    OGXM_LOG("[GAMBIT GIP RX]\n");
    OGXM_LOG("count=%lu\n", static_cast<unsigned long>(rx_packets_));
    OGXM_LOG("addr=%u\ninstance=%u\nendpoint=0x%02X\nxfer=SUCCESS\nlen=%u\n",
             static_cast<unsigned>(address_), static_cast<unsigned>(instance_), ep_in,
             static_cast<unsigned>(len));
    OGXM_LOG("command=0x%02X (%s)\n", cmd, gip_cmd_label(cmd));
    OGXM_LOG("options=0x%02X\n", options);
    OGXM_LOG("ACK REQUIRED=%s\n", (options & GIP_OPT_ACK) ? "YES" : "NO");
    if (options & GIP_OPT_CHUNK_START)
    {
        OGXM_LOG("CHUNK_START=YES\n");
    }
    if (options & GIP_OPT_CHUNK)
    {
        OGXM_LOG("CHUNK=YES\n");
    }
    if (hdr)
    {
        OGXM_LOG("sequence=%u\n", hdr->sequence);
        OGXM_LOG("payload_len=%lu\n", static_cast<unsigned long>(hdr->packet_length));
        OGXM_LOG("chunk_offset=%lu\n", static_cast<unsigned long>(hdr->chunk_offset));
        OGXM_LOG("hdr_len=%u\n", hdr->hdr_len);
    }
    OGXM_LOG("data:\n");
    log_hex_line(report, len);
    OGXM_LOG("\n");
}

void VictrixGambitHost::log_announce_once(const uint8_t* report, uint16_t len, const GipHdr& hdr)
{
    if (announce_logged_ || !report || len < 4)
    {
        return;
    }
    announce_logged_ = true;

    const uint8_t* p = report + hdr.hdr_len;
    const uint16_t payload = static_cast<uint16_t>(hdr.packet_length);

    OGXM_LOG("[GAMBIT GIP]\nANNOUNCE received\n");
    OGXM_LOG("sequence=%u\n", hdr.sequence);

    if (payload >= kAnnouncePayloadLen &&
        len >= static_cast<uint16_t>(hdr.hdr_len + kAnnouncePayloadLen))
    {
        const uint16_t vid = read_le16(p + 8);
        const uint16_t pid = read_le16(p + 10);
        OGXM_LOG("VID=%04X\nPID=%04X\n", vid, pid);
        OGXM_LOG("firmware: %u.%u.%u.%u\n", read_le16(p + 12), read_le16(p + 14),
                 read_le16(p + 16), read_le16(p + 18));
        OGXM_LOG("hardware: %u.%u.%u.%u\n\n", read_le16(p + 20), read_le16(p + 22),
                 read_le16(p + 24), read_le16(p + 26));
    }
    else
    {
        OGXM_LOG("payload_len=%u (expected %u)\n\n", payload, kAnnouncePayloadLen);
    }
}

void VictrixGambitHost::handle_announce(const uint8_t* report, uint16_t len, const GipHdr& hdr)
{
    log_announce_once(report, len, hdr);

    if (identify_requested_ || pending_cmd_ == PendingCmd::Identify ||
        state_ == InitState::SendIdentify || state_ == InitState::WaitIdentifyTxComplete ||
        state_ == InitState::WaitIdentifyResponse ||
        state_ == InitState::GamepadIdentified || state_ == InitState::Ready ||
        state_ == InitState::SendPower || state_ == InitState::WaitPowerComplete ||
        state_ == InitState::SendLed || state_ == InitState::WaitLedComplete ||
        state_ == InitState::SendAuth || state_ == InitState::WaitAuthComplete ||
        state_ == InitState::WaitForInput)
    {
        return;
    }

    if (state_ != InitState::WaitAnnounce)
    {
        return;
    }

    identify_requested_ = true;
    OGXM_LOG("[GAMBIT INIT]\nANNOUNCE received — requesting IDENTIFY\n\n");
    set_state(InitState::SendIdentify);
    try_submit_current();
}

bool VictrixGambitHost::descriptor_has_gamepad_class(const uint8_t* data, uint16_t len) const
{
    /* xone gip_pkt_identify: unknown[16] + 8x le16 offsets; classes_offset at +16+10 = +26. */
    if (!data || len < 32)
    {
        return false;
    }
    const uint16_t classes_off = read_le16(data + 26);
    if (classes_off >= len)
    {
        return false;
    }
    uint16_t off = classes_off;
    const uint8_t count = data[off++];
    static constexpr char kGamepadClass[] = "Windows.Xbox.Input.Gamepad";
    for (uint8_t i = 0; i < count; ++i)
    {
        if (off + 2 > len)
        {
            return false;
        }
        const uint16_t str_len = read_le16(data + off);
        off = static_cast<uint16_t>(off + 2);
        if (str_len == 0 || static_cast<uint32_t>(off) + str_len > len)
        {
            return false;
        }
        if (str_len == (sizeof(kGamepadClass) - 1) &&
            std::memcmp(data + off, kGamepadClass, sizeof(kGamepadClass) - 1) == 0)
        {
            return true;
        }
        /* Also accept shorter "Gamepad" substring used by some descriptors. */
        if (str_len >= 7)
        {
            for (uint16_t s = 0; s + 7 <= str_len; ++s)
            {
                if (std::memcmp(data + off + s, "Gamepad", 7) == 0)
                {
                    return true;
                }
            }
        }
        off = static_cast<uint16_t>(off + str_len);
    }
    return false;
}

void VictrixGambitHost::begin_post_identify_bringup()
{
    set_state(InitState::GamepadIdentified);
    OGXM_LOG("[GAMBIT INIT]\nGAMEPAD_IDENTIFIED — starting POWER/LED/AUTH\n\n");
    set_state(InitState::SendPower);
    try_submit_current();
}

void VictrixGambitHost::on_identify_descriptor_complete()
{
    OGXM_LOG("[GAMBIT IDENTIFY]\ndescriptor complete\nlen=%u\n",
             static_cast<unsigned>(identify_total_));
    log_hex_line(identify_buf_.data(),
                 identify_total_ > 64 ? 64 : identify_total_);
    OGXM_LOG("\n");

    if (!descriptor_has_gamepad_class(identify_buf_.data(), identify_total_))
    {
        OGXM_LOG("[GAMBIT IDENTIFY]\ngamepad class NOT found — scanning raw buffer\n");
        /* Physical Gambit is known gamepad; allow bring-up if "Gamepad" appears anywhere. */
        bool found = false;
        for (uint16_t i = 0; i + 7 <= identify_total_; ++i)
        {
            if (std::memcmp(identify_buf_.data() + i, "Gamepad", 7) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            OGXM_LOG("[GAMBIT IDENTIFY]\nno Gamepad class string — InitFailed\n\n");
            set_state(InitState::InitFailed);
            return;
        }
        OGXM_LOG("[GAMBIT IDENTIFY]\nGamepad substring found\n\n");
    }
    else
    {
        OGXM_LOG("[GAMBIT IDENTIFY]\nclass=Windows.Xbox.Input.Gamepad\n\n");
    }

    identify_assembling_ = false;
    begin_post_identify_bringup();
}

void VictrixGambitHost::handle_identify_packet(const uint8_t* report, uint16_t len,
                                               const GipHdr& hdr)
{
    using namespace tuh_xinput::XboxOne;

    if (!identify_rx_logged_)
    {
        identify_rx_logged_ = true;
        OGXM_LOG("[GAMBIT IDENTIFY]\n");
        OGXM_LOG("command=0x%02X\noptions=0x%02X\nsequence=%u\n", hdr.command, hdr.options,
                 hdr.sequence);
        OGXM_LOG("payload_len=%lu\nchunk_offset=%lu\n",
                 static_cast<unsigned long>(hdr.packet_length),
                 static_cast<unsigned long>(hdr.chunk_offset));
        OGXM_LOG("data:\n");
        log_hex_line(report, len);
        OGXM_LOG("\n");
    }
    else
    {
        OGXM_LOG("[GAMBIT IDENTIFY]\nchunk continue options=0x%02X seq=%u plen=%lu coff=%lu\n",
                 hdr.options, hdr.sequence, static_cast<unsigned long>(hdr.packet_length),
                 static_cast<unsigned long>(hdr.chunk_offset));
        log_hex_line(report, len);
        OGXM_LOG("\n");
    }

    const uint8_t* payload = report + hdr.hdr_len;
    const uint16_t payload_avail =
        (len > hdr.hdr_len) ? static_cast<uint16_t>(len - hdr.hdr_len) : 0;
    const uint16_t copy_len =
        static_cast<uint16_t>((hdr.packet_length < payload_avail) ? hdr.packet_length
                                                                 : payload_avail);

    if (hdr.options & GIP_OPT_CHUNK_START)
    {
        /* First chunk: chunk_offset is total assembled length. */
        if (hdr.chunk_offset == 0 || hdr.chunk_offset > kIdentifyBufMax)
        {
            OGXM_LOG("[GAMBIT IDENTIFY]\ninvalid total length %lu\n\n",
                     static_cast<unsigned long>(hdr.chunk_offset));
            set_state(InitState::InitFailed);
            return;
        }
        identify_total_ = static_cast<uint16_t>(hdr.chunk_offset);
        identify_buf_.fill(0);
        identify_assembling_ = true;

        GipHdr ack_hdr = hdr;
        const uint16_t remaining =
            (identify_total_ > copy_len) ? static_cast<uint16_t>(identify_total_ - copy_len) : 0;
        /* xone zeros chunk_offset before ACK after allocating buffer. */
        ack_hdr.chunk_offset = 0;
        maybe_ack(report, len, ack_hdr, remaining);

        if (copy_len > 0 && copy_len <= identify_total_)
        {
            std::memcpy(identify_buf_.data(), payload, copy_len);
        }
        return;
    }

    if (hdr.options & GIP_OPT_CHUNK)
    {
        uint16_t remaining = 0;
        if (identify_assembling_ && identify_total_ > (hdr.chunk_offset + hdr.packet_length))
        {
            remaining = static_cast<uint16_t>(identify_total_ - (hdr.chunk_offset + hdr.packet_length));
        }
        maybe_ack(report, len, hdr, remaining);

        /* Empty chunk payload signals completion (xone). */
        if (hdr.packet_length == 0)
        {
            if (identify_assembling_)
            {
                on_identify_descriptor_complete();
            }
            return;
        }

        if (!identify_assembling_)
        {
            return;
        }
        if (hdr.chunk_offset + copy_len > identify_total_ ||
            hdr.chunk_offset + copy_len > kIdentifyBufMax)
        {
            OGXM_LOG("[GAMBIT IDENTIFY]\nchunk overflow\n\n");
            set_state(InitState::InitFailed);
            return;
        }
        std::memcpy(identify_buf_.data() + hdr.chunk_offset, payload, copy_len);
        return;
    }

    /* Non-chunked IDENTIFY descriptor in one packet. */
    maybe_ack(report, len, hdr, 0);
    if (copy_len == 0 || copy_len > kIdentifyBufMax)
    {
        OGXM_LOG("[GAMBIT IDENTIFY]\nempty/oversized single packet\n\n");
        set_state(InitState::InitFailed);
        return;
    }
    identify_total_ = copy_len;
    identify_buf_.fill(0);
    std::memcpy(identify_buf_.data(), payload, copy_len);
    identify_assembling_ = true;
    on_identify_descriptor_complete();
}

void VictrixGambitHost::on_any_gip_rx(const uint8_t* report, uint16_t len)
{
    using namespace tuh_xinput::XboxOne;

    ++rx_packets_;

    GipHdr hdr{};
    const bool decoded = decode_gip_header(report, len, hdr);
    maybe_log_rx_packet(report, len, decoded ? &hdr : nullptr);

    if (!gip_traffic_seen_)
    {
        gip_traffic_seen_ = true;
        OGXM_LOG("[GAMBIT INIT]\nGIP traffic detected\n\n");
    }

    if (!decoded)
    {
        return;
    }

    if (hdr.command == GIP_CMD_ANNOUNCE)
    {
        maybe_ack(report, len, hdr, 0); /* no-op unless ACK bit set */
        handle_announce(report, len, hdr);
        return;
    }

    if (hdr.command == GIP_CMD_IDENTIFY &&
        (state_ == InitState::WaitIdentifyResponse || state_ == InitState::WaitIdentifyTxComplete ||
         identify_requested_))
    {
        handle_identify_packet(report, len, hdr);
        return;
    }

    /* ACK any other ACK-required traffic (e.g. future auth). */
    if ((hdr.options & GIP_OPT_ACK) != 0 && hdr.command != GIP_CMD_IDENTIFY)
    {
        maybe_ack(report, len, hdr, 0);
    }

    if (state_ != InitState::Ready && hdr.command == GIP_CMD_INPUT &&
        gip_logical_len(report, len) >= sizeof(::XboxOne::InReport))
    {
        controller_active_ = true;
        set_state(InitState::Ready);
        OGXM_LOG("[GAMBIT INIT]\nREADY\n\n");
        log_perf();
    }
}

void VictrixGambitHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                   const uint8_t* report_desc, uint16_t desc_len)
{
    (void)gamepad;
    (void)report_desc;
    (void)desc_len;

    address_ = address;
    instance_ = instance;
    ++init_gen_;
    g_active_gambit = this;

    state_ = InitState::Idle;
    pending_cmd_ = PendingCmd::None;
    submit_retries_ = 0;
    tx_requested_ = 0;
    tx_completed_ = 0;
    tx_failed_ = 0;
    rx_arm_attempts_ = 0;
    rx_arm_ok_ = 0;
    rx_arm_fail_ = 0;
    rx_complete_ = 0;
    rx_packets_ = 0;
    rx_log_remaining_ = 40;
    gip_traffic_seen_ = false;
    controller_active_ = false;
    identify_requested_ = false;
    announce_logged_ = false;
    identify_rx_logged_ = false;
    wait_announce_start_ms_ = 0;
    identify_buf_.fill(0);
    identify_total_ = 0;
    identify_assembling_ = false;
    prev_report_.fill(0);
    prev_report_len_ = 0;
    guide_pressed_ = 0;
    last_guide_07_ms_ = 0;
    cancel_guide_orphan();
    prev_buttons_ = 0;
    prev_dpad_ = 0;
    enum_log_done_ = false;

    if (retry_task_id_ != 0)
    {
        TaskQueue::Core1::cancel_delayed_task(retry_task_id_);
        retry_task_id_ = 0;
    }

    log_enumeration(address, instance);

    tuh_xinput::prepare_gip_session(address, instance);

    /* Arm IN only after SET_INTERFACE — then wait for ANNOUNCE (xone discovery). */
    schedule_status_tick();
    set_state(InitState::DisableAudioInterface);
    begin_disable_audio_interface();
}

void VictrixGambitHost::disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;
    (void)address;
    (void)instance;
    ++init_gen_;
    if (g_active_gambit == this)
    {
        g_active_gambit = nullptr;
    }
    if (retry_task_id_ != 0)
    {
        TaskQueue::Core1::cancel_delayed_task(retry_task_id_);
        retry_task_id_ = 0;
    }
    if (status_task_id_ != 0)
    {
        TaskQueue::Core1::cancel_delayed_task(status_task_id_);
        status_task_id_ = 0;
    }
    cancel_guide_orphan();
    guide_pressed_ = 0;
    state_ = InitState::Idle;
    pending_cmd_ = PendingCmd::None;
    controller_active_ = false;
    gip_traffic_seen_ = false;
    identify_requested_ = false;
    announce_logged_ = false;
    identify_rx_logged_ = false;
    identify_assembling_ = false;
    identify_total_ = 0;
}

void VictrixGambitHost::map_input(Gamepad& gamepad, const uint8_t* report, uint16_t len,
                                  uint8_t guide_pressed, Gamepad::PadIn& gp_in)
{
    const uint16_t logical = gip_logical_len(report, len);
    if (logical < sizeof(XboxOne::InReport))
    {
        return;
    }

    const auto* in_report = reinterpret_cast<const XboxOne::InReport*>(report);
    const uint16_t b = in_report->buttons;

    if (b & XboxOne::GipWireButtons::DPAD_UP)    gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (b & XboxOne::GipWireButtons::DPAD_DOWN)  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (b & XboxOne::GipWireButtons::DPAD_LEFT)  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (b & XboxOne::GipWireButtons::DPAD_RIGHT) gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;

    if (b & XboxOne::GipWireButtons::LEFT_THUMB)     gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (b & XboxOne::GipWireButtons::RIGHT_THUMB)    gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (b & XboxOne::GipWireButtons::LEFT_SHOULDER)  gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (b & XboxOne::GipWireButtons::RIGHT_SHOULDER) gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (b & XboxOne::GipWireButtons::BACK)  gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (b & XboxOne::GipWireButtons::START) gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (b & XboxOne::GipWireButtons::SYNC)  gp_in.buttons |= gamepad.MAP_BUTTON_MISC;
    if (b & XboxOne::Buttons0::GUIDE)       gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (guide_pressed)                     gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (b & XboxOne::GipWireButtons::A)     gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (b & XboxOne::GipWireButtons::B)     gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (b & XboxOne::GipWireButtons::X)     gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (b & XboxOne::GipWireButtons::Y)     gp_in.buttons |= gamepad.MAP_BUTTON_Y;

    if (pid_ == kPidDocumented && read_share_documented_02d6(report, len))
    {
        gp_in.buttons |= gamepad.MAP_BUTTON_MISC;
    }

    gp_in.trigger_l = gamepad.scale_trigger_l(static_cast<uint8_t>(in_report->trigger_l >> 2));
    gp_in.trigger_r = gamepad.scale_trigger_r(static_cast<uint8_t>(in_report->trigger_r >> 2));

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) =
        gamepad.scale_joystick_l(in_report->joystick_lx, in_report->joystick_ly, true);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) =
        gamepad.scale_joystick_r(in_report->joystick_rx, in_report->joystick_ry, true);
}

void VictrixGambitHost::emit_pad_in(Gamepad& gamepad, uint8_t guide_pressed)
{
    Gamepad::PadIn gp_in{};
    if (prev_report_len_ >= sizeof(XboxOne::InReport))
    {
        map_input(gamepad, prev_report_.data(), prev_report_len_, guide_pressed, gp_in);
    }
    else if (guide_pressed)
    {
        gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    }
    gamepad.set_pad_in(gp_in);
}

void VictrixGambitHost::cancel_guide_orphan()
{
    if (guide_orphan_task_id_ != 0)
    {
        TaskQueue::Core1::cancel_delayed_task(guide_orphan_task_id_);
        guide_orphan_task_id_ = 0;
    }
    ++guide_orphan_gen_;
}

void VictrixGambitHost::schedule_guide_orphan_clear(Gamepad& gamepad)
{
    cancel_guide_orphan();
    const uint8_t gen = guide_orphan_gen_;
    Gamepad* gp = &gamepad;
    guide_orphan_task_id_ = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(
        guide_orphan_task_id_, kGuideOrphanMs, false,
        [this, gp, gen]() {
            guide_orphan_task_id_ = 0;
            if (gen != guide_orphan_gen_ || !guide_pressed_)
            {
                return;
            }
            guide_pressed_ = 0;
            emit_pad_in(*gp, 0);
        });
}

void VictrixGambitHost::log_input_change(const Gamepad::PadIn& gp_in)
{
    const uint16_t buttons = gp_in.buttons;
    if (buttons == prev_buttons_ && gp_in.dpad == prev_dpad_)
    {
        return;
    }
    OGXM_LOG("[GAMBIT INPUT] A=%u B=%u X=%u Y=%u SHARE=%u\n",
             (buttons & Gamepad::BUTTON_A) ? 1u : 0u,
             (buttons & Gamepad::BUTTON_B) ? 1u : 0u,
             (buttons & Gamepad::BUTTON_X) ? 1u : 0u,
             (buttons & Gamepad::BUTTON_Y) ? 1u : 0u,
             (buttons & Gamepad::BUTTON_MISC) ? 1u : 0u);
    prev_buttons_ = buttons;
    prev_dpad_ = gp_in.dpad;
}

void VictrixGambitHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                       const uint8_t* report, uint16_t len)
{
    if (len < 1)
    {
        arm_rx("rearm_empty", true);
        return;
    }

    on_any_gip_rx(report, len);

    const uint8_t cmd = report[0];

    /* Do not parse PadIn until READY (first valid INPUT). Always re-arm IN. */
    if (state_ != InitState::Ready)
    {
        arm_rx("rearm_pre_ready", true);
        return;
    }

    if (cmd == XboxOne::GIP_CMD_VIRTUAL_KEY)
    {
        last_guide_07_ms_ = board_api::ms_since_boot();
        uint8_t pressed = 0;
        if (len >= 5)
        {
            if (report[4] == 0x5B && len >= 6)
            {
                pressed = (report[5] & 0x01) ? 1 : 0;
            }
            else
            {
                pressed = (report[4] & 0x01) ? 1 : 0;
            }
        }
        guide_pressed_ = pressed;
        emit_pad_in(gamepad, guide_pressed_);
        if (pressed)
        {
            schedule_guide_orphan_clear(gamepad);
        }
        else
        {
            cancel_guide_orphan();
        }
        arm_rx("rearm_vkey", true);
        return;
    }

    if (cmd != XboxOne::GIP_CMD_INPUT)
    {
        arm_rx("rearm_non_input", true);
        return;
    }

    const uint16_t logical = gip_logical_len(report, len);
    if (logical < sizeof(XboxOne::InReport))
    {
        arm_rx("rearm_short_input", true);
        return;
    }

    if (guide_pressed_ && (board_api::ms_since_boot() - last_guide_07_ms_) > kGuideStaleMs)
    {
        guide_pressed_ = 0;
        cancel_guide_orphan();
    }

    Gamepad::PadIn gp_in{};
    map_input(gamepad, report, len, guide_pressed_, gp_in);
    gamepad.set_pad_in(gp_in);
    log_input_change(gp_in);

    const uint16_t copy_len = std::min<uint16_t>(len, static_cast<uint16_t>(prev_report_.size()));
    std::memcpy(prev_report_.data(), report, copy_len);
    prev_report_len_ = copy_len;

    arm_rx("rearm_input", true);
}

bool VictrixGambitHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    if (state_ != InitState::Ready)
    {
        return true;
    }
    Gamepad::PadOut gp_out = gamepad.get_pad_out();
    return tuh_xinput::set_rumble(address, instance, gp_out.rumble_l, gp_out.rumble_r, false);
}

namespace tuh_xinput {

void out_xfer_complete_cb(uint8_t dev_addr, uint8_t instance, bool success,
                          uint8_t const* data, uint16_t len)
{
    VictrixGambitHost::on_out_xfer_complete(dev_addr, instance, success, data, len);
}

} // namespace tuh_xinput
