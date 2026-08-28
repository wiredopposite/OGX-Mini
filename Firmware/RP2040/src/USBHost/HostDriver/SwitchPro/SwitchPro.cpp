#include <cstring>
#include <array>

#include "host/usbh.h"
#include "class/hid/hid_host.h"
#include "common/tusb_types.h"

#include "Board/Config.h"
#include "Board/board_api.h"
#if defined(CONFIG_EN_USB_HOST)
#include "pio_usb.h"
#endif

#include "USBHost/HostDriver/SwitchPro/SwitchPro.h"
#include "USBHost/HostDriver/SwitchPro/Switch2UsbInitPackets.h"
#include "USBHost/HostManager.h"
#include "Gamepad/MotionImu.h"

namespace {

#if defined(CONFIG_EN_USB_HOST)
void service_usb_host() {
    pio_usb_host_frame();
    tuh_task();
}
#else
void service_usb_host() {
    tuh_task();
}
#endif

bool try_hid_out_id(uint8_t address, uint8_t instance, uint8_t report_id, const void* report, uint16_t len) {
    if (!tuh_hid_send_ready(address, instance)) {
        return false;
    }
    return tuh_hid_send_report(address, instance, report_id, report, len);
}

bool try_hid_out(uint8_t address, uint8_t instance, const void* report, uint16_t len) {
    return try_hid_out_id(address, instance, 0, report, len);
}

constexpr uint8_t kInitForceAdvanceRetries = 8;

} // namespace

bool SwitchProHost::is_switch2_usb_family(uint16_t vid, uint16_t pid)
{
    if (vid != 0x057Eu)
    {
        return false;
    }
    switch (pid)
    {
    case 0x2066u:
    case 0x2067u:
    case 0x2069u:
    case 0x2073u:
        return true;
    default:
        return false;
    }
}

SwitchProHost::~SwitchProHost()
{
    switch2_cancel_bringup();
}

void SwitchProHost::disconnect_cb(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;
    (void)address;
    (void)instance;
    switch2_cancel_bringup();
}

uintptr_t SwitchProHost::switch2_make_cb_token(uint8_t daddr, uint8_t instance)
{
    return (static_cast<uintptr_t>(instance) << 8) | daddr;
}

SwitchProHost* SwitchProHost::switch2_lookup(uint8_t daddr, uint8_t instance)
{
    return HostManager::get_instance().get_switch_pro_host(daddr, instance);
}

void SwitchProHost::switch2_cancel_bringup()
{
    switch2_bringup_active_ = false;
    switch2_bringup_pending_retry_ = false;
    switch2_next_packet_ms_ = 0;
    switch2_pkt_idx_ = 0;
    switch2_bulk_keepalive_busy_ = false;

    if (switch2_dev_addr_ != 0)
    {
        if (switch2_ep_out_ != 0)
        {
            (void)tuh_edpt_abort_xfer(switch2_dev_addr_, switch2_ep_out_);
        }
        if (switch2_ep_in_ != 0)
        {
            (void)tuh_edpt_abort_xfer(switch2_dev_addr_, switch2_ep_in_);
        }
    }

    switch2_ep_out_ = 0;
    switch2_ep_in_ = 0;
}

void SwitchProHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                              const uint8_t* report_desc, uint16_t desc_len)
{
    (void)report_desc;
    (void)desc_len;

    std::memset(&out_report_, 0, sizeof(out_report_));
    std::memset(&prev_in_report_, 0, sizeof(prev_in_report_));
    sequence_counter_ = 0;
    hid_instance_ = instance;
    switch2_dev_addr_ = address;
    switch2_gamepad_ = &gamepad;
    init_state_ = InitState::USB_MAC;
    pending_usb_ack_ = 0;
    pending_subcmd_ack_ = 0;
    init_step_retries_ = 0;
    usb_timeout_sent_ = false;
    switch2_cancel_bringup();

    uint16_t vid = 0;
    uint16_t pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);

    if (is_switch2_usb_family(vid, pid))
    {
        switch2_start_bringup();
    }
    else
    {
        start_usb_wired_init(address, instance);
    }
}

void SwitchProHost::switch2_start_bringup()
{
    switch2_bringup_active_ = true;
    switch2_pkt_idx_ = 0;
    switch2_ep_out_ = 0;
    switch2_ep_in_ = 0;
    switch2_next_packet_ms_ = 0;
    switch2_start_config_read();
}

void SwitchProHost::switch2_start_config_read()
{
    if (!tuh_descriptor_get_configuration(
            switch2_dev_addr_,
            0,
            switch2_cfg_buf_.data(),
            static_cast<uint16_t>(switch2_cfg_buf_.size()),
            switch2_cfg_cb,
            switch2_make_cb_token(switch2_dev_addr_, hid_instance_)))
    {
        switch2_finish_bringup();
    }
}

bool SwitchProHost::switch2_parse_open_iface1(const uint8_t* desc_cfg, uint16_t buflen)
{
    switch2_ep_out_ = 0;
    switch2_ep_in_ = 0;

    if (buflen < sizeof(tusb_desc_configuration_t))
    {
        return false;
    }

    auto const* cfg = reinterpret_cast<tusb_desc_configuration_t const*>(desc_cfg);
    uint16_t total = tu_le16toh(cfg->wTotalLength);
    uint16_t len = TU_MIN(total, buflen);
    uint8_t const* end = desc_cfg + len;
    uint8_t const* p = tu_desc_next(desc_cfg);

    while (p < end)
    {
        if (tu_desc_type(p) != TUSB_DESC_INTERFACE)
        {
            p = tu_desc_next(p);
            continue;
        }

        auto const* itf = reinterpret_cast<tusb_desc_interface_t const*>(p);
        if (itf->bInterfaceNumber != 1 || itf->bAlternateSetting != 0)
        {
            p = tu_desc_next(p);
            continue;
        }

        p = tu_desc_next(p);
        int endpoint = 0;
        while (endpoint < itf->bNumEndpoints && p < end)
        {
            if (tu_desc_type(p) != TUSB_DESC_ENDPOINT)
            {
                p = tu_desc_next(p);
                continue;
            }

            auto const* ep = reinterpret_cast<tusb_desc_endpoint_t const*>(p);
            if (ep->bmAttributes.xfer == TUSB_XFER_BULK)
            {
                if (!tuh_edpt_open(switch2_dev_addr_, ep))
                {
                    return false;
                }
                if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_OUT && switch2_ep_out_ == 0)
                {
                    switch2_ep_out_ = ep->bEndpointAddress;
                }
                else if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN && switch2_ep_in_ == 0)
                {
                    switch2_ep_in_ = ep->bEndpointAddress;
                }
            }
            endpoint++;
            p = tu_desc_next(p);
        }

        return switch2_ep_out_ != 0;
    }

    return false;
}

void SwitchProHost::switch2_cfg_cb(tuh_xfer_s* xfer)
{
    const uint8_t daddr = xfer->daddr;
    const uint8_t instance = static_cast<uint8_t>(xfer->user_data >> 8);
    SwitchProHost* self = switch2_lookup(daddr, instance);
    if (self == nullptr || !self->switch2_bringup_active_)
    {
        return;
    }

    if (xfer->result != XFER_RESULT_SUCCESS ||
        xfer->actual_len < sizeof(tusb_desc_configuration_t))
    {
        self->switch2_finish_bringup();
        return;
    }

    uint16_t actual = static_cast<uint16_t>(xfer->actual_len);
    auto const* cfg = reinterpret_cast<tusb_desc_configuration_t const*>(self->switch2_cfg_buf_.data());
    const uint16_t total = tu_le16toh(cfg->wTotalLength);
    if (total > actual && total <= self->switch2_cfg_buf_.size())
    {
        if (!tuh_descriptor_get_configuration(
                self->switch2_dev_addr_,
                0,
                self->switch2_cfg_buf_.data(),
                total,
                switch2_cfg_cb,
                switch2_make_cb_token(self->switch2_dev_addr_, self->hid_instance_)))
        {
            self->switch2_finish_bringup();
        }
        return;
    }

    if (!self->switch2_parse_open_iface1(self->switch2_cfg_buf_.data(), actual))
    {
        self->switch2_finish_bringup();
        return;
    }

    self->switch2_pkt_idx_ = 0;
    self->switch2_submit_out_packet();
}

void SwitchProHost::switch2_out_cb(tuh_xfer_s* xfer)
{
    const uint8_t daddr = xfer->daddr;
    const uint8_t instance = static_cast<uint8_t>(xfer->user_data >> 8);
    SwitchProHost* self = switch2_lookup(daddr, instance);
    if (self == nullptr || !self->switch2_bringup_active_)
    {
        return;
    }

    if (xfer->result != XFER_RESULT_SUCCESS)
    {
        self->switch2_finish_bringup();
        return;
    }

    if (self->switch2_ep_in_ != 0)
    {
        self->switch2_submit_in_read();
    }
    else
    {
        self->switch2_after_packet_round();
    }
}

void SwitchProHost::switch2_in_cb(tuh_xfer_s* xfer)
{
    const uint8_t daddr = xfer->daddr;
    const uint8_t instance = static_cast<uint8_t>(xfer->user_data >> 8);
    SwitchProHost* self = switch2_lookup(daddr, instance);
    if (self == nullptr || !self->switch2_bringup_active_)
    {
        return;
    }
    (void)xfer;
    self->switch2_after_packet_round();
}

void SwitchProHost::switch2_keepalive_cb(tuh_xfer_s* xfer)
{
    const uint8_t daddr = xfer->daddr;
    const uint8_t instance = static_cast<uint8_t>(xfer->user_data >> 8);
    SwitchProHost* self = switch2_lookup(daddr, instance);
    if (self != nullptr)
    {
        self->switch2_bulk_keepalive_busy_ = false;
    }
    (void)daddr;
    (void)instance;
}

void SwitchProHost::switch2_send_bulk_keepalive()
{
    if (switch2_ep_out_ == 0 || switch2_bulk_keepalive_busy_)
    {
        return;
    }

    static const uint8_t kKeepalive[] = {0x03, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00};
    if (sizeof(kKeepalive) > switch2_bulk_out_buf_.size())
    {
        return;
    }

    std::memcpy(switch2_bulk_out_buf_.data(), kKeepalive, sizeof(kKeepalive));

    tuh_xfer_t xfer{};
    xfer.daddr = switch2_dev_addr_;
    xfer.ep_addr = switch2_ep_out_;
    xfer.buflen = static_cast<uint16_t>(sizeof(kKeepalive));
    xfer.buffer = switch2_bulk_out_buf_.data();
    xfer.complete_cb = switch2_keepalive_cb;
    xfer.user_data = switch2_make_cb_token(switch2_dev_addr_, hid_instance_);

    switch2_bulk_keepalive_busy_ = true;
    if (!tuh_edpt_xfer(&xfer))
    {
        switch2_bulk_keepalive_busy_ = false;
    }
}

void SwitchProHost::switch2_submit_out_packet()
{
    const Switch2UsbBulkPacket& pkt = SWITCH2_USB_BULK_PACKETS[switch2_pkt_idx_];
    if (pkt.len > switch2_bulk_out_buf_.size())
    {
        switch2_finish_bringup();
        return;
    }

    std::memcpy(switch2_bulk_out_buf_.data(), pkt.data, pkt.len);

    tuh_xfer_t xfer{};
    xfer.daddr = switch2_dev_addr_;
    xfer.ep_addr = switch2_ep_out_;
    xfer.buflen = pkt.len;
    xfer.buffer = switch2_bulk_out_buf_.data();
    xfer.complete_cb = switch2_out_cb;
    xfer.user_data = switch2_make_cb_token(switch2_dev_addr_, hid_instance_);

    if (!tuh_edpt_xfer(&xfer))
    {
        switch2_next_packet_ms_ = board_api::ms_since_boot() + 12U;
    }
}

void SwitchProHost::switch2_submit_in_read()
{
    tuh_xfer_t xfer{};
    xfer.daddr = switch2_dev_addr_;
    xfer.ep_addr = switch2_ep_in_;
    xfer.buflen = 64;
    xfer.buffer = switch2_bulk_in_buf_.data();
    xfer.complete_cb = switch2_in_cb;
    xfer.user_data = switch2_make_cb_token(switch2_dev_addr_, hid_instance_);

    if (!tuh_edpt_xfer(&xfer))
    {
        switch2_next_packet_ms_ = board_api::ms_since_boot() + 12U;
    }
}

void SwitchProHost::switch2_after_packet_round()
{
    switch2_pkt_idx_++;
    if (switch2_pkt_idx_ >= SWITCH2_USB_BULK_PACKET_COUNT)
    {
        switch2_finish_bringup();
        return;
    }
    switch2_next_packet_ms_ = board_api::ms_since_boot() + 12U;
}

void SwitchProHost::switch2_poll_bringup()
{
    if (!switch2_bringup_active_ || switch2_next_packet_ms_ == 0)
    {
        return;
    }
    if (board_api::ms_since_boot() < switch2_next_packet_ms_)
    {
        return;
    }
    switch2_next_packet_ms_ = 0;
    switch2_submit_out_packet();
}

void SwitchProHost::switch2_finish_bringup()
{
    const bool bulk_complete = switch2_pkt_idx_ >= SWITCH2_USB_BULK_PACKET_COUNT;
    switch2_bringup_active_ = false;
    switch2_next_packet_ms_ = 0;

    if (switch2_gamepad_ == nullptr)
    {
        return;
    }

    if (bulk_complete)
    {
        /* Switch 2–family: bulk bring-up replaces Switch 1 wired 0x80 HID init. */
        init_state_ = InitState::DONE;
        pending_usb_ack_ = 0;
        pending_subcmd_ack_ = 0;
        send_usb_disable_timeout(switch2_dev_addr_, hid_instance_);
        switch2_last_bulk_keepalive_ms_ = board_api::ms_since_boot();
        switch2_send_bulk_keepalive();
        tuh_hid_receive_report(switch2_dev_addr_, hid_instance_);
        return;
    }

    switch2_bringup_pending_retry_ = true;
}

bool SwitchProHost::send_usb_wired_command(uint8_t address, uint8_t instance, uint8_t subcmd)
{
    uint8_t payload[63] = {0};
    payload[0] = subcmd;
    pending_usb_ack_ = subcmd;
    return try_hid_out_id(address, instance, SwitchPro::REPORT_ID_USB_OUT, payload, sizeof(payload));
}

void SwitchProHost::start_usb_wired_init(uint8_t address, uint8_t instance)
{
    init_state_ = InitState::USB_MAC;
    pending_usb_ack_ = SwitchPro::CMD::USB_SUB_MAC;
    (void)send_usb_wired_command(address, instance, SwitchPro::CMD::USB_SUB_MAC);
    tuh_hid_receive_report(address, instance);
}

void SwitchProHost::fill_neutral_rumble(SwitchPro::OutReport& out)
{
    out.rumble_l[0] = 0x00;
    out.rumble_l[1] = 0x01;
    out.rumble_l[2] = 0x40;
    out.rumble_l[3] = 0x40;
    out.rumble_r[0] = 0x00;
    out.rumble_r[1] = 0x01;
    out.rumble_r[2] = 0x40;
    out.rumble_r[3] = 0x40;
}

void SwitchProHost::advance_after_usb_ack(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    switch (init_state_)
    {
        case InitState::USB_MAC:
            init_state_ = InitState::USB_HANDSHAKE1;
            (void)send_usb_wired_command(address, instance, SwitchPro::CMD::USB_SUB_HANDSHAKE);
            break;
        case InitState::USB_HANDSHAKE1:
            init_state_ = InitState::USB_BAUD;
            (void)send_usb_wired_command(address, instance, SwitchPro::CMD::USB_SUB_BAUD);
            break;
        case InitState::USB_BAUD:
            init_state_ = InitState::USB_HANDSHAKE2;
            (void)send_usb_wired_command(address, instance, SwitchPro::CMD::USB_SUB_HANDSHAKE);
            break;
        case InitState::USB_HANDSHAKE2:
            init_state_ = InitState::USB_NO_TIMEOUT;
            send_usb_disable_timeout_and_probe(address, instance);
            break;
        default:
            break;
    }
}

void SwitchProHost::send_usb_disable_timeout_and_probe(uint8_t address, uint8_t instance)
{
    send_usb_disable_timeout(address, instance);

    std::memset(&out_report_, 0, sizeof(out_report_));
    fill_neutral_rumble(out_report_);
    out_report_.command = SwitchPro::CMD::AND_RUMBLE;
    out_report_.sequence_counter = get_output_sequence_counter();
    out_report_.sub_command = SwitchPro::CMD::USB_PROBE;
    pending_subcmd_ack_ = SwitchPro::CMD::USB_PROBE;
    (void)try_hid_out(address, instance, &out_report_, 11);
    usb_timeout_sent_ = true;
    init_step_retries_ = 0;
}

void SwitchProHost::send_usb_disable_timeout(uint8_t address, uint8_t instance)
{
    uint8_t usb_out[63] = {0};
    usb_out[0] = SwitchPro::CMD::USB_SUB_DISABLE_TIMEOUT;
    (void)try_hid_out_id(address, instance, SwitchPro::REPORT_ID_USB_OUT, usb_out, sizeof(usb_out));
}

void SwitchProHost::init_switch_host(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;

    if (init_state_ <= InitState::USB_HANDSHAKE2)
    {
        if (pending_usb_ack_ != 0)
        {
            (void)send_usb_wired_command(address, instance, pending_usb_ack_);
        }
        return;
    }

    if (init_state_ == InitState::USB_NO_TIMEOUT)
    {
        if (!usb_timeout_sent_)
        {
            send_usb_disable_timeout_and_probe(address, instance);
        }
        return;
    }

    if (!tuh_hid_send_ready(address, instance))
    {
        return;
    }

    std::memset(&out_report_, 0, sizeof(out_report_));
    fill_neutral_rumble(out_report_);
    out_report_.command = SwitchPro::CMD::AND_RUMBLE;
    out_report_.sequence_counter = get_output_sequence_counter();

    switch (init_state_)
    {
        case InitState::LED:
            out_report_.sub_command = SwitchPro::CMD::LED;
            out_report_.sub_command_args[0] = idx_ + 1;
            if (try_hid_out(address, instance, &out_report_, 12))
            {
                init_state_ = InitState::LED_HOME;
            }
            break;
        case InitState::LED_HOME:
            out_report_.sub_command = SwitchPro::CMD::LED_HOME;
            out_report_.sub_command_args[0] = static_cast<uint8_t>((0 << 4) | 0xF);
            out_report_.sub_command_args[1] = static_cast<uint8_t>((0xF << 4) | 0x0);
            out_report_.sub_command_args[2] = static_cast<uint8_t>((0xF << 4) | 0x0);
            if (try_hid_out(address, instance, &out_report_, 14))
            {
                init_state_ = InitState::FULL_REPORT;
            }
            break;
        case InitState::FULL_REPORT:
            out_report_.sub_command = SwitchPro::CMD::MODE;
            out_report_.sub_command_args[0] = SwitchPro::CMD::FULL_REPORT_MODE;
            if (try_hid_out(address, instance, &out_report_, 12))
            {
                init_state_ = InitState::IMU;
            }
            break;
        case InitState::IMU:
            out_report_.sub_command = SwitchPro::CMD::GYRO;
            out_report_.sub_command_args[0] = 1;
            if (try_hid_out(address, instance, &out_report_, 12))
            {
                init_state_ = InitState::DONE;
                pending_subcmd_ack_ = 0;
            }
            break;
        default:
            break;
    }
}

uint8_t SwitchProHost::get_output_sequence_counter()
{
    uint8_t counter = sequence_counter_;
    sequence_counter_ = (sequence_counter_ + 1) & 0x0F;
    return counter;
}

static const uint8_t* switchpro_report_payload(const uint8_t* report, uint16_t len, uint16_t& out_payload_len)
{
    out_payload_len = len;
    if (len == sizeof(SwitchPro::InReport))
    {
        return report;
    }
    if (len >= 2U + sizeof(SwitchPro::InReport))
    {
        const uint8_t rid = report[0];
        if (rid == SwitchPro::REPORT_ID_STANDARD || rid == SwitchPro::REPORT_ID_SWITCH2_FULL ||
            rid == SwitchPro::REPORT_ID_FULL_ALT)
        {
            out_payload_len = static_cast<uint16_t>(len - 2U);
            return report + 2;
        }
    }
    return nullptr;
}

void SwitchProHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report,
                                   uint16_t len)
{
    if (switch2_bringup_active_)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    if (init_state_ != InitState::DONE)
    {
        if (len >= 2 && report[0] == SwitchPro::REPORT_ID_USB_INIT &&
            init_state_ <= InitState::USB_HANDSHAKE2)
        {
            const uint8_t ack = report[1];
            if (ack == pending_usb_ack_)
            {
                advance_after_usb_ack(gamepad, address, instance);
            }
        }
        else if (init_state_ == InitState::USB_NO_TIMEOUT &&
                 len >= 1 && report[0] == SwitchPro::REPORT_ID_SUBCMD)
        {
            init_state_ = InitState::LED;
            pending_subcmd_ack_ = 0;
            init_switch_host(gamepad, address, instance);
        }
        else if (init_state_ > InitState::USB_NO_TIMEOUT)
        {
            init_switch_host(gamepad, address, instance);
        }
        tuh_hid_receive_report(address, instance);
        return;
    }

    uint16_t payload_len = 0;
    const uint8_t* payload = switchpro_report_payload(report, len, payload_len);
    if (payload == nullptr || payload_len < sizeof(SwitchPro::InReport))
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    const SwitchPro::InReport* in_report = reinterpret_cast<const SwitchPro::InReport*>(payload);
    if (std::memcmp(&prev_in_report_.buttons, in_report->buttons, 9) == 0 &&
        MotionImu::switch_usb_imu_unchanged(prev_imu_, payload, payload_len))
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp_in;

    if (in_report->buttons[0] & SwitchPro::Buttons0::Y)
        gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->buttons[0] & SwitchPro::Buttons0::B)
        gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons[0] & SwitchPro::Buttons0::A)
        gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons[0] & SwitchPro::Buttons0::X)
        gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (in_report->buttons[2] & SwitchPro::Buttons2::L)
        gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons[0] & SwitchPro::Buttons0::R)
        gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons[1] & SwitchPro::Buttons1::L3)
        gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (in_report->buttons[1] & SwitchPro::Buttons1::R3)
        gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report->buttons[1] & SwitchPro::Buttons1::MINUS)
        gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons[1] & SwitchPro::Buttons1::PLUS)
        gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report->buttons[1] & SwitchPro::Buttons1::HOME)
        gp_in.buttons |= gamepad.MAP_BUTTON_SYS;
    if (in_report->buttons[1] & SwitchPro::Buttons1::CAPTURE)
        gp_in.buttons |= gamepad.MAP_BUTTON_MISC;

    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_UP)
        gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_DOWN)
        gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_LEFT)
        gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
    if (in_report->buttons[2] & SwitchPro::Buttons2::DPAD_RIGHT)
        gp_in.dpad |= gamepad.MAP_DPAD_LEFT;

    gp_in.trigger_l = in_report->buttons[2] & SwitchPro::Buttons2::ZL ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;
    gp_in.trigger_r = in_report->buttons[0] & SwitchPro::Buttons0::ZR ? Range::MAX<uint8_t> : Range::MIN<uint8_t>;

    uint16_t joy_lx = in_report->joysticks[0] | ((in_report->joysticks[1] & 0xF) << 8);
    uint16_t joy_ly = (in_report->joysticks[1] >> 4) | (in_report->joysticks[2] << 4);
    uint16_t joy_rx = in_report->joysticks[3] | ((in_report->joysticks[4] & 0xF) << 8);
    uint16_t joy_ry = (in_report->joysticks[4] >> 4) | (in_report->joysticks[5] << 4);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) =
        gamepad.scale_joystick_l(normalize_axis(joy_lx), normalize_axis(joy_ly), true);

    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) =
        gamepad.scale_joystick_r(normalize_axis(joy_rx), normalize_axis(joy_ry), true);

    if (payload_len >= 22) {
        gp_in.motion_source = Gamepad::PadIn::MOTION_SRC_SWITCH_USB;
        MotionImu::fill_from_switch_usb_payload(gp_in.accel, gp_in.gyro, payload, payload_len);
    }

    gamepad.set_pad_in(gp_in);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(SwitchPro::InReport));
    if (payload_len >= 22) {
        constexpr uint16_t kInReportSize = 10;
        constexpr uint16_t kImuSampleSize = sizeof(MotionImu::SwitchImuSample);
        uint16_t imu_off = kInReportSize;
        if (payload_len >= kInReportSize + (3 * kImuSampleSize)) {
            imu_off = static_cast<uint16_t>(kInReportSize + (2 * kImuSampleSize));
        }
        std::memcpy(prev_imu_, payload + imu_off, sizeof(prev_imu_));
    }
}

bool SwitchProHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    uint16_t vid = 0;
    uint16_t pid = 0;
    tuh_vid_pid_get(address, &vid, &pid);
    const bool switch2_device = is_switch2_usb_family(vid, pid);

    if (switch2_device && switch2_bringup_active_)
    {
        switch2_poll_bringup();
        service_usb_host();
        return false;
    }

    if (switch2_device && switch2_bringup_pending_retry_)
    {
        switch2_bringup_pending_retry_ = false;
        switch2_start_bringup();
        service_usb_host();
        return false;
    }

    if (init_state_ != InitState::DONE)
    {
        if (init_state_ == InitState::USB_NO_TIMEOUT)
        {
            if (++init_step_retries_ >= kInitForceAdvanceRetries)
            {
                init_state_ = InitState::LED;
                pending_subcmd_ack_ = 0;
                usb_timeout_sent_ = false;
            }
        }
        init_switch_host(gamepad, address, instance);
        tuh_hid_receive_report(address, instance);
        service_usb_host();
        return false;
    }

    if (switch2_device)
    {
        const uint32_t now_ms = board_api::ms_since_boot();
        if (now_ms - switch2_last_bulk_keepalive_ms_ >= 2000U)
        {
            switch2_last_bulk_keepalive_ms_ = now_ms;
            switch2_send_bulk_keepalive();
        }
    }

    std::memset(&out_report_, 0, sizeof(out_report_));

    out_report_.command = SwitchPro::CMD::RUMBLE_ONLY;
    out_report_.sequence_counter = get_output_sequence_counter();
    fill_neutral_rumble(out_report_);

    Gamepad::PadOut gp_out = gamepad.get_pad_out();

    if (gp_out.rumble_l > 0)
    {
        uint8_t amplitude_l = static_cast<uint8_t>(((gp_out.rumble_l / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        out_report_.rumble_l[0] = amplitude_l;
        out_report_.rumble_l[1] = 0x88;
        out_report_.rumble_l[2] = amplitude_l / 2;
        out_report_.rumble_l[3] = 0x61;
    }

    if (gp_out.rumble_r > 0)
    {
        uint8_t amplitude_r = static_cast<uint8_t>(((gp_out.rumble_r / 255.0f) * 0.8f + 0.5f) * (0xC0 - 0x40) + 0x40);

        out_report_.rumble_r[0] = amplitude_r;
        out_report_.rumble_r[1] = 0x88;
        out_report_.rumble_r[2] = amplitude_r / 2;
        out_report_.rumble_r[3] = 0x61;
    }

    if (!tuh_hid_send_ready(address, instance))
    {
        return false;
    }
    return tuh_hid_send_report(address, instance, 0, &out_report_, 10);
}
