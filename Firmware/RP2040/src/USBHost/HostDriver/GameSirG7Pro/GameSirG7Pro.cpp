#include <cstring>
#include <tuple>

#include "tusb.h"
#include "host/usbh.h"
#include "class/hid/hid_host.h"

#include "Board/ogxm_log.h"
#include "USBHost/HostDriver/GameSirG7Pro/GameSirG7Pro.h"

namespace {

void map_dpad(Gamepad& gamepad, uint8_t hat_raw, Gamepad::PadIn& gp_in)
{
    const uint8_t hat = hat_raw & GameSirG7Pro::DPAD_MASK;
    if (hat > 7) {
        return; /* 0x0F (and other) = neutral */
    }
    switch (hat) {
        case GameSirG7Pro::DPad::UP:
            gp_in.dpad |= gamepad.MAP_DPAD_UP;
            break;
        case GameSirG7Pro::DPad::DOWN:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN;
            break;
        case GameSirG7Pro::DPad::LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_LEFT;
            break;
        case GameSirG7Pro::DPad::RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_RIGHT;
            break;
        case GameSirG7Pro::DPad::UP_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_RIGHT;
            break;
        case GameSirG7Pro::DPad::DOWN_RIGHT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_RIGHT;
            break;
        case GameSirG7Pro::DPad::DOWN_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_DOWN_LEFT;
            break;
        case GameSirG7Pro::DPad::UP_LEFT:
            gp_in.dpad |= gamepad.MAP_DPAD_UP_LEFT;
            break;
        default:
            break;
    }
}

void map_face_system(Gamepad& gamepad, uint8_t b0, uint8_t b1, Gamepad::PadIn& gp_in)
{
    if (b0 & GameSirG7Pro::Buttons0::A)
        gp_in.buttons |= gamepad.MAP_BUTTON_A;
    if (b0 & GameSirG7Pro::Buttons0::B)
        gp_in.buttons |= gamepad.MAP_BUTTON_B;
    if (b0 & GameSirG7Pro::Buttons0::X)
        gp_in.buttons |= gamepad.MAP_BUTTON_X;
    if (b0 & GameSirG7Pro::Buttons0::Y)
        gp_in.buttons |= gamepad.MAP_BUTTON_Y;
    if (b0 & GameSirG7Pro::Buttons0::LB)
        gp_in.buttons |= gamepad.MAP_BUTTON_LB;
    if (b0 & GameSirG7Pro::Buttons0::RB)
        gp_in.buttons |= gamepad.MAP_BUTTON_RB;

    if (b1 & GameSirG7Pro::Buttons1::L3)
        gp_in.buttons |= gamepad.MAP_BUTTON_L3;
    if (b1 & GameSirG7Pro::Buttons1::R3)
        gp_in.buttons |= gamepad.MAP_BUTTON_R3;
    if (b1 & GameSirG7Pro::Buttons1::BACK)
        gp_in.buttons |= gamepad.MAP_BUTTON_BACK;
    if (b1 & GameSirG7Pro::Buttons1::START)
        gp_in.buttons |= gamepad.MAP_BUTTON_START;
}

void map_triggers(Gamepad& gamepad, uint8_t b1, uint8_t lt, uint8_t rt, Gamepad::PadIn& gp_in)
{
    gp_in.trigger_l = gamepad.scale_trigger_l(lt);
    gp_in.trigger_r = gamepad.scale_trigger_r(rt);
    if ((b1 & GameSirG7Pro::Buttons1::LT) && lt == 0)
        gp_in.trigger_l = Range::MAX<uint8_t>;
    if ((b1 & GameSirG7Pro::Buttons1::RT) && rt == 0)
        gp_in.trigger_r = Range::MAX<uint8_t>;
}

/** Wired USB: 9 bytes, buttons first (captured on 3537:1022). */
bool parse_usb_9(Gamepad& gamepad, const uint8_t* r, uint16_t len, Gamepad::PadIn& gp_in)
{
    if (!r || len < GameSirG7Pro::USB_REPORT_LEN) {
        return false;
    }
    /* Reject BT-style Report 0x07 frames that happen to be longer. */
    if (len >= GameSirG7Pro::BT_REPORT_LEN && r[0] == GameSirG7Pro::REPORT_ID_BT) {
        return false;
    }

    map_face_system(gamepad, r[0], r[1], gp_in);
    map_dpad(gamepad, r[2], gp_in);
    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(r[3], r[4]);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(r[5], r[6]);
    map_triggers(gamepad, r[1], r[7], r[8], gp_in);
    return true;
}

/** Classic BT / Report 0x07: sticks first (working over Bluetooth). */
bool parse_bt_07(Gamepad& gamepad, const uint8_t* r, uint16_t len, Gamepad::PadIn& gp_in)
{
    if (!r || r[0] != GameSirG7Pro::REPORT_ID_BT || len < 10) {
        return false;
    }
    map_dpad(gamepad, r[5], gp_in);
    map_face_system(gamepad, r[6], r[7], gp_in);
    std::tie(gp_in.joystick_lx, gp_in.joystick_ly) = gamepad.scale_joystick_l(r[1], r[2]);
    std::tie(gp_in.joystick_rx, gp_in.joystick_ry) = gamepad.scale_joystick_r(r[3], r[4]);
    const uint8_t rt = (len >= 9) ? r[8] : 0;
    const uint8_t lt = (len >= 10) ? r[9] : 0;
    map_triggers(gamepad, r[7], lt, rt, gp_in);
    return true;
}

} // namespace

void GameSirG7ProHost::initialize(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                  const uint8_t* report_desc, uint16_t desc_len)
{
    (void)report_desc;
    (void)desc_len;
    gamepad.set_analog_host(true);
    prev_len_ = 0;
    std::memset(prev_report_, 0, sizeof(prev_report_));
    OGXM_LOG("GameSir G7 Pro: HID host ready (3537:1022) addr=%u inst=%u desc_len=%u\n",
             static_cast<unsigned>(address), static_cast<unsigned>(instance),
             static_cast<unsigned>(desc_len));
    tuh_hid_receive_report(address, instance);
}

void GameSirG7ProHost::process_report(Gamepad& gamepad, uint8_t address, uint8_t instance,
                                      const uint8_t* report, uint16_t len)
{
    if (report == nullptr || len < GameSirG7Pro::USB_REPORT_LEN) {
        tuh_hid_receive_report(address, instance);
        return;
    }

#if defined(CONFIG_OGXM_DEBUG)
    /* Log a few unique fingerprints (len + first 3 bytes) — not every button edge. */
    static uint8_t s_fp_n = 0;
    static uint8_t s_fp_key[8][4]{};
    const uint8_t key[4] = {
        static_cast<uint8_t>(len & 0xFF),
        report[0],
        report[1],
        report[2],
    };
    bool seen = false;
    for (uint8_t i = 0; i < s_fp_n; ++i) {
        if (std::memcmp(s_fp_key[i], key, 4) == 0) {
            seen = true;
            break;
        }
    }
    if (!seen && s_fp_n < 8) {
        std::memcpy(s_fp_key[s_fp_n], key, 4);
        ++s_fp_n;
        OGXM_LOG("[G7PRO USB RX] len=%u:", static_cast<unsigned>(len));
        const uint16_t n = (len < 12) ? len : 12;
        for (uint16_t i = 0; i < n; ++i)
            OGXM_LOG(" %02x", report[i]);
        OGXM_LOG("\n");
    }
#endif

    const uint16_t cmp_len =
        (len < GameSirG7Pro::USB_REPORT_LEN) ? len : GameSirG7Pro::USB_REPORT_LEN;
    if (prev_len_ == cmp_len && std::memcmp(prev_report_, report, cmp_len) == 0) {
        tuh_hid_receive_report(address, instance);
        return;
    }

    Gamepad::PadIn gp_in{};
    bool ok = parse_usb_9(gamepad, report, len, gp_in);
    if (!ok) {
        ok = parse_bt_07(gamepad, report, len, gp_in);
    }

    if (ok) {
        gamepad.set_pad_in(gp_in);
        std::memcpy(prev_report_, report, cmp_len);
        prev_len_ = cmp_len;
    }

    tuh_hid_receive_report(address, instance);
}

bool GameSirG7ProHost::send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance)
{
    (void)gamepad;
    (void)address;
    (void)instance;
    return true;
}
