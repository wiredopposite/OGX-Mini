#include <cstring>

#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "Board/Config.h"
#if defined(CONFIG_EN_USB_HOST)
#include "pio_usb.h"
#endif
#include "pico/time.h"

#include "USBHost/HostDriver/PS3/PS3.h"

#if defined(CONFIG_EN_BLUETOOTH)
extern "C" {
/* Do not include bt/uni_bt.h here: it pulls BTstack, whose btstack_hid.h redefines
 * HID_REPORT_TYPE_* / hid_report_type_t and breaks compilation with TinyUSB hid.h. */
typedef uint8_t uni_bt_bd_addr_t[6];
void uni_bt_get_local_bd_addr_safe(uni_bt_bd_addr_t addr);
}
#endif

namespace {

struct SyncXferCtx {
    bool done{false};
};

void sync_xfer_cb(tuh_xfer_s* xfer) {
    auto* ctx = reinterpret_cast<SyncXferCtx*>(xfer->user_data);
    ctx->done = true;
}

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

} // namespace

const tusb_control_request_t PS3Host::ENABLE_USB_REQUEST = {
    .bmRequestType = 0x21,
    .bRequest = 0x09, // SET_REPORT
    .wValue = static_cast<uint16_t>((HID_REPORT_TYPE_FEATURE << 8) | PS3::ReportID::FEATURE_F4),
    .wIndex = 0x0000,
    .wLength = 4,
};

const tusb_control_request_t PS3Host::RUMBLE_REQUEST = 
{
    .bmRequestType = 0x21,
    .bRequest = 0x09, // SET_REPORT
    .wValue = 0x0201,
    .wIndex = 0x0000, 
    .wLength = sizeof(PS3::OutReport)
};

#if defined(CONFIG_EN_BLUETOOTH)
/* Same as bluepad32/tools/sixaxispairer — DualShock 3 stores host BD_ADDR for Bluetooth. */
const tusb_control_request_t PS3Host::BT_MAC_FEATURE_REQUEST = {
    .bmRequestType = 0x21,
    .bRequest = 0x09, // SET_REPORT
    .wValue = static_cast<uint16_t>((HID_REPORT_TYPE_FEATURE << 8) | 0xF5),
    .wIndex = 0x0000,
    .wLength = 8,
};

bool PS3Host::local_bt_addr_is_nonzero(const uint8_t addr[6]) {
    for (int i = 0; i < 6; ++i) {
        if (addr[i] != 0) {
            return true;
        }
    }
    return false;
}

bool PS3Host::program_ds3_bt_host(uint8_t address) {
    uni_bt_bd_addr_t addr{};
    uni_bt_get_local_bd_addr_safe(addr);
    if (!local_bt_addr_is_nonzero(addr)) {
        return false;
    }
    bt_pair_buf_[0] = 0xF5;
    bt_pair_buf_[1] = 0;
    std::memcpy(bt_pair_buf_.data() + 2, addr, 6);
    /* Sync transfer: PIO USB needs pio_usb_host_frame() while control completes. */
    (void)send_control_xfer_wait(address, &BT_MAC_FEATURE_REQUEST, bt_pair_buf_.data(), 150u);
    return true;
}

void PS3Host::try_deferred_ds3_bt_pair(uint8_t address) {
    if (!ds3_bt_pair_deferred_) {
        return;
    }
    if (program_ds3_bt_host(address)) {
        ds3_bt_pair_deferred_ = false;
    }
}
#endif

void PS3Host::init_host_out_report(PS3::OutReport& out, uint8_t player_idx)
{
    /* USB Host Shield PS3_REPORT_BUFFER layout — not gadget DEFAULT_OUT_REPORT (leading 0x01). */
    std::memset(&out, 0, sizeof(out));
    for (int i = 0; i < 4; ++i) {
        out.leds[i].time_enabled = 0xFF;
        out.leds[i].duty_length = 0x27;
        out.leds[i].enabled = 0x10;
        out.leds[i].duty_off = 0x00;
        out.leds[i].duty_on = 0x32;
    }
    out.leds_bitmap = static_cast<uint8_t>(0x02u << player_idx);
}

bool PS3Host::send_control_xfer(uint8_t dev_addr, const tusb_control_request_t* request, uint8_t* buffer, tuh_xfer_cb_t complete_cb, uintptr_t user_data)
{
    tuh_xfer_s transfer = 
    {
        .daddr = dev_addr,
        .ep_addr = 0x00,
        .setup = request, 
        .buffer = buffer,
        .complete_cb = complete_cb, 
        .user_data = user_data
    };
    return tuh_control_xfer(&transfer);
}

bool PS3Host::send_control_xfer_wait(uint8_t dev_addr, const tusb_control_request_t* request, uint8_t* buffer,
                                      uint32_t max_attempts_ms)
{
    SyncXferCtx ctx;
    if (!send_control_xfer(dev_addr, request, buffer, sync_xfer_cb, reinterpret_cast<uintptr_t>(&ctx))) {
        return false;
    }
    for (uint32_t i = 0; !ctx.done && i < max_attempts_ms; ++i) {
        service_usb_host();
        sleep_ms(1);
    }
    return ctx.done;
}

bool PS3Host::send_control_output_sync(uint8_t address, PS3::OutReport* report)
{
    return send_control_xfer_wait(address, &RUMBLE_REQUEST, reinterpret_cast<uint8_t*>(report), 150u);
}

bool PS3Host::send_control_output_async(uint8_t address, PS3::OutReport* report)
{
    return send_control_xfer(address, &RUMBLE_REQUEST, reinterpret_cast<uint8_t*>(report), nullptr, 0);
}

void PS3Host::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) 
{
    (void)report_desc;
    (void)desc_len;

    gamepad.set_analog_host(true);

    init_host_out_report(out_report_, idx_);
    reports_enabled_ = false;
#if defined(CONFIG_EN_BLUETOOTH)
    ds3_bt_pair_deferred_ = false;
#endif

    /* DualShock 3 USB: SET feature 0xF4 {0x42,0x0c,0,0} then one LED OUT (USB Host Shield). */
    uint8_t enable_usb_data[] = {0x42, 0x0c, 0x00, 0x00};
    (void)send_control_xfer_wait(address, &ENABLE_USB_REQUEST, enable_usb_data, 150u);
    (void)send_control_output_sync(address, &out_report_);

#if defined(CONFIG_EN_BLUETOOTH)
    /* Program master BD_ADDR so the pad can reconnect wirelessly after unplug (sixaxispairer).
     * Must run here — the old GET 0xF2 async chain was skipped once wired init marked DONE. */
    if (!program_ds3_bt_host(address)) {
        ds3_bt_pair_deferred_ = true;
    }
#endif

    reports_enabled_ = true;
    last_keepalive_ms_ = time_us_32() / 1000;

    tuh_hid_receive_report(address, instance);
}

void PS3Host::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len)
{
#if defined(CONFIG_EN_BLUETOOTH)
    try_deferred_ds3_bt_pair(address);
#endif
    const PS3::InReport* in_report = reinterpret_cast<const PS3::InReport*>(report);
    if (std::memcmp(&prev_in_report_, in_report, std::min(static_cast<size_t>(len), static_cast<size_t>(26))) == 0)
    {
        tuh_hid_receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp_in;   

    if (in_report->buttons[0] & PS3::Buttons0::DPAD_UP)    gp_in.dpad |= gamepad.MAP_DPAD_UP;
    if (in_report->buttons[0] & PS3::Buttons0::DPAD_DOWN)  gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
    if (in_report->buttons[0] & PS3::Buttons0::DPAD_LEFT)  gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
    if (in_report->buttons[0] & PS3::Buttons0::DPAD_RIGHT) gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;

    if (in_report->buttons[0] & PS3::Buttons0::SELECT)   gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (in_report->buttons[0] & PS3::Buttons0::START)    gp_in.buttons |= gamepad.MAP_BUTTON_START;
    if (in_report->buttons[0] & PS3::Buttons0::L3)       gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (in_report->buttons[0] & PS3::Buttons0::R3)       gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (in_report->buttons[1] & PS3::Buttons1::L1)       gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (in_report->buttons[1] & PS3::Buttons1::R1)       gp_in.buttons |= gamepad.MAP_BUTTON_RB;
    if (in_report->buttons[1] & PS3::Buttons1::TRIANGLE) gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (in_report->buttons[1] & PS3::Buttons1::CIRCLE)   gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (in_report->buttons[1] & PS3::Buttons1::CROSS)    gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (in_report->buttons[1] & PS3::Buttons1::SQUARE)   gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (in_report->buttons[2] & PS3::Buttons2::SYS)      gp_in.buttons |= gamepad.MAP_BUTTON_SYS;

    if (gamepad.analog_enabled())
    {
        gp_in.analog[gamepad.MAP_ANALOG_OFF_UP]    = in_report->up_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_DOWN]  = in_report->down_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_LEFT]  = in_report->left_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_RIGHT] = in_report->right_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_A]  = in_report->cross_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_B]  = in_report->circle_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_X]  = in_report->square_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_Y]  = in_report->triangle_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_LB] = in_report->l1_axis;
        gp_in.analog[gamepad.MAP_ANALOG_OFF_RB] = in_report->r1_axis;
    }

    gp_in.trigger_l = gamepad.scale_trigger_l(in_report->l2_axis);
    gp_in.trigger_r = gamepad.scale_trigger_r(in_report->r2_axis);

    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(in_report->joystick_lx, in_report->joystick_ly);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(in_report->joystick_rx, in_report->joystick_ry);

    gamepad.set_pad_in(gp_in);

    tuh_hid_receive_report(address, instance);
    std::memcpy(&prev_in_report_, in_report, sizeof(PS3::InReport));
}

bool PS3Host::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)instance;

    if (!reports_enabled_) {
        return false;
    }

    const uint32_t current_ms = time_us_32() / 1000;
    Gamepad::PadOut gp_out = gamepad.get_pad_out();
    const bool has_rumble = (gp_out.rumble_l != 0 || gp_out.rumble_r != 0);
    const bool keepalive_due =
        KEEPALIVE_MS != 0u && (current_ms - last_keepalive_ms_ >= KEEPALIVE_MS);

    if (!has_rumble && !gamepad.new_pad_out() && !keepalive_due) {
        return true;
    }

    out_report_.rumble.right_duration   = (gp_out.rumble_r > 0) ? 20 : 0;
    out_report_.rumble.right_motor_on   = (gp_out.rumble_r > 0) ? 1  : 0;
    out_report_.rumble.left_duration    = (gp_out.rumble_l > 0) ? 20 : 0;
    out_report_.rumble.left_motor_force = gp_out.rumble_l;
    out_report_.leds_bitmap = static_cast<uint8_t>(0x02u << idx_);

    last_keepalive_ms_ = current_ms;
    return send_control_output_async(address, &out_report_);
}
