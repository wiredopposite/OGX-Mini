/**
 * Converts OGX-Mini Gamepad state to WiimoteReport for Wii mode BT emulation.
 * Mappings match PicoGamepadConverter convert_data.c WII case (XInput layout).
 */
#include "Wii/WiiReportConverter.h"
#include "Gamepad/Gamepad.h"
#include "Wii/wiimote-lib/wiimote.h"
#include <cstring>
#include <algorithm>

#if defined(OGXM_BOARD_USES_PICO_W_FIRMWARE) && defined(CONFIG_EN_USB_HOST)
#include "Board/board_api.h"
#include "pico/time.h"

namespace Wii {

/** Flash LED n times to indicate mode: 1 = NO_EXTENSION, 2 = Nunchuk, 3 = Classic */
static void flash_led_times(unsigned n) {
    for (unsigned i = 0; i < n; ++i) {
        board_api::set_led(false);
        sleep_ms(80);
        board_api::set_led(true);
        if (i + 1 < n)
            sleep_ms(100);
    }
}

// Require Home+Down held this many consecutive frames before switching mode (avoids stuck/noisy Home)
static constexpr unsigned MODE_SWITCH_HOLD_FRAMES = 10;
static unsigned s_home_down_hold_count = 0;

// Same as PicoGamepadConverter: int16 stick -> int8 for IR (-128..127)
static inline int8_t stick_to_wiimote_ir(int16_t val) {
    return (int8_t)(val >> 8);
}
// int16 stick -> uint8 for nunchuk stick (0-255, center 128). val>>8 is -128..127; +128 -> 0..255
static inline uint8_t stick_to_nunchuk_xy(int16_t val) {
    int v = (val >> 8) + 128;
    return (uint8_t)std::clamp(v, 0, 255);
}
// int16 stick -> uint8 for classic stick (32..63 center 32). PicoGamepadConverter: (sThumbLX/32768.0)*31+32
static inline uint8_t stick_to_classic_xy(int16_t val) {
    int v = (int)((float)val / 32768.f * 31.f + 32.f);
    return (uint8_t)std::clamp(v, 0, 63);
}

// Classic right stick: center 15, range 0..30. Deadzone to avoid pointer drift at rest.
static constexpr int16_t CLASSIC_RS_DEADZONE = 2048;
static inline int16_t classic_rs_apply_deadzone(int16_t val) {
    return (val > -CLASSIC_RS_DEADZONE && val < CLASSIC_RS_DEADZONE) ? 0 : val;
}

void gamepad_to_wiimote_report(Gamepad& gamepad, void* out_wiimote_report)
{
    WiimoteReport* out = static_cast<WiimoteReport*>(out_wiimote_report);
    if (!out) return;

    Gamepad::PadIn pad = gamepad.get_pad_in();

    // Preserve console_info so the Wiimote stack keeps the saved Wii address across frames
    // (we only leave it unchanged; first boot it's zero so we stay discoverable)

    // Default mode to NO_EXTENSION if invalid (first frame or corrupted)
    if (out->mode != NO_EXTENSION && out->mode != WIIMOTE_AND_NUNCHUCK && out->mode != CLASSIC_CONTROLLER)
        out->mode = NO_EXTENSION;

    // --- Wiimote buttons (match PicoGamepadConverter NO_EXTENSION / WIIMOTE_AND_NUNCHUCK) ---
    // XInput: A=0x1000 B=0x2000 X=0x4000 Y=0x8000 BACK=0x0020 START=0x0010 GUIDE=0x0400 DPAD_*=0x0001/2/4/8 LB=0x0100 RB=0x0200
    // Our Gamepad: BUTTON_A=0x0001 B=0x0002 X=0x0004 Y=0x0008 BACK=0x0040 START=0x0080 SYS=0x0400 LB=0x0100 RB=0x0200
    out->wiimote.a     = (pad.buttons & Gamepad::BUTTON_A) != 0;
    out->wiimote.b     = (pad.buttons & Gamepad::BUTTON_B) != 0;
    out->wiimote.minus = (pad.buttons & Gamepad::BUTTON_BACK) != 0;
    out->wiimote.plus  = (pad.buttons & Gamepad::BUTTON_START) != 0;
    out->wiimote.home  = (pad.buttons & Gamepad::BUTTON_SYS) != 0;
    out->wiimote.one   = (pad.buttons & Gamepad::BUTTON_X) != 0;
    out->wiimote.two   = (pad.buttons & Gamepad::BUTTON_Y) != 0;
    out->wiimote.up    = (pad.dpad & Gamepad::DPAD_UP) != 0;
    out->wiimote.down  = (pad.dpad & Gamepad::DPAD_DOWN) != 0;
    out->wiimote.left  = (pad.dpad & Gamepad::DPAD_LEFT) != 0;
    out->wiimote.right = (pad.dpad & Gamepad::DPAD_RIGHT) != 0;
    out->wiimote.sync  = false;
    out->wiimote.power = false;

    // Accelerometer: neutral ~0x200 (10-bit). Wii expects this for pointer/motion
    out->wiimote.accel_x = 0x200;
    out->wiimote.accel_y = 0x200;
    out->wiimote.accel_z = 0x200;

    // IR pointer: left stick for NO_EXTENSION (right→right, up→up; deadzone to prevent drift)
    out->wiimote.ir_x = stick_to_wiimote_ir(classic_rs_apply_deadzone(pad.joystick_lx));
    out->wiimote.ir_y = stick_to_wiimote_ir(classic_rs_apply_deadzone(-pad.joystick_ly));

    // Mode and extension data (match PicoGamepadConverter)
    switch (out->mode) {
        case NO_EXTENSION:
            // Sideway: home+LB toggles (we don't change sideway here; leave as-is or 0)
            if ((out->wiimote.home) && (pad.buttons & Gamepad::BUTTON_LB)) {
                out->wiimote.home = false;
                out->sideway = (0xF << 1) | (out->sideway & 1);
            } else if ((out->sideway >> 1) == 0xF) {
                out->sideway = (out->sideway & 1) ? 0 : 1;
            }
            // Mode switch: home+down held for MODE_SWITCH_HOLD_FRAMES -> next mode (NO_EXTENSION -> WIIMOTE_AND_NUNCHUCK)
            if (out->wiimote.home && out->wiimote.down) {
                if (++s_home_down_hold_count >= MODE_SWITCH_HOLD_FRAMES) {
                    s_home_down_hold_count = 0;
                    out->wiimote.home = false;
                    out->wiimote.down = false;
                    out->switch_mode = 1;
                }
            } else {
                s_home_down_hold_count = 0;
            }
            if (out->switch_mode) {
                out->mode = WIIMOTE_AND_NUNCHUCK;
                out->switch_mode = 0;
                out->reset_ir = 1;
                flash_led_times(2);
            }
            break;

        case WIIMOTE_AND_NUNCHUCK:
            // Wiimote IR from right stick (right→right, up→up; deadzone to prevent drift)
            out->wiimote.ir_x = stick_to_wiimote_ir(classic_rs_apply_deadzone(pad.joystick_rx));
            out->wiimote.ir_y = stick_to_wiimote_ir(classic_rs_apply_deadzone(-pad.joystick_ry));
            // Nunchuk stick from left stick (right→right, up→forward); deadzone so center is true 128 (stops drift in aiming)
            out->nunchuk.x = stick_to_nunchuk_xy(classic_rs_apply_deadzone(pad.joystick_lx));
            out->nunchuk.y = stick_to_nunchuk_xy(classic_rs_apply_deadzone(-pad.joystick_ly));
            out->nunchuk.c = (pad.trigger_l > 127);
            out->nunchuk.z = (pad.buttons & Gamepad::BUTTON_RB) != 0;
            out->nunchuk.accel_x = 0x200;
            out->nunchuk.accel_y = 0x200;
            out->nunchuk.accel_z = 0x200;
            // Fake motion: LB held
            if (pad.buttons & Gamepad::BUTTON_LB) {
                out->fake_motion = 1;
            } else if (out->fake_motion) {
                out->wiimote.accel_x = 0x82 << 2;
                out->nunchuk.accel_x = 0x82 << 2;
                out->wiimote.accel_y = 0x82 << 2;
                out->nunchuk.accel_y = 0x82 << 2;
                out->wiimote.accel_z = 0x9f << 2;
                out->nunchuk.accel_z = 0x9f << 2;
                out->fake_motion = 0;
                out->center_accel = 1;
            }
            // Mode switch: home+down held -> CLASSIC
            if (out->wiimote.home && out->wiimote.down) {
                if (++s_home_down_hold_count >= MODE_SWITCH_HOLD_FRAMES) {
                    s_home_down_hold_count = 0;
                    out->wiimote.home = false;
                    out->wiimote.down = false;
                    out->switch_mode = 1;
                }
            } else {
                s_home_down_hold_count = 0;
            }
            if (out->switch_mode) {
                out->mode = CLASSIC_CONTROLLER;
                out->switch_mode = 0;
                out->reset_ir = 1;
                flash_led_times(3);
            }
            break;

        case CLASSIC_CONTROLLER: {
            // IR pointer from right stick (correct direction: +rx = right, -ry = up; deadzone avoids drift)
            out->wiimote.ir_x = stick_to_wiimote_ir(classic_rs_apply_deadzone(pad.joystick_rx));
            out->wiimote.ir_y = stick_to_wiimote_ir(classic_rs_apply_deadzone(-pad.joystick_ry));
            out->classic.a = (pad.buttons & Gamepad::BUTTON_A) != 0;
            out->classic.b = (pad.buttons & Gamepad::BUTTON_B) != 0;
            out->classic.x = (pad.buttons & Gamepad::BUTTON_Y) != 0;  // PicoGamepadConverter: x=Y, y=X
            out->classic.y = (pad.buttons & Gamepad::BUTTON_X) != 0;
            out->classic.minus = (pad.buttons & Gamepad::BUTTON_BACK) != 0;
            out->classic.plus  = (pad.buttons & Gamepad::BUTTON_START) != 0;
            out->classic.home  = (pad.buttons & Gamepad::BUTTON_SYS) != 0;
            out->classic.up    = (pad.dpad & Gamepad::DPAD_UP) != 0;
            out->classic.down  = (pad.dpad & Gamepad::DPAD_DOWN) != 0;
            out->classic.left  = (pad.dpad & Gamepad::DPAD_LEFT) != 0;
            out->classic.right = (pad.dpad & Gamepad::DPAD_RIGHT) != 0;
            out->classic.lz    = (pad.buttons & Gamepad::BUTTON_LB) != 0;
            out->classic.rz    = (pad.buttons & Gamepad::BUTTON_RB) != 0;
            out->classic.lt    = pad.trigger_l;
            out->classic.rt    = pad.trigger_r;
            out->classic.ltrigger = (pad.trigger_l > 32);
            out->classic.rtrigger = (pad.trigger_r > 32);
            // Left stick: correct direction (right→right, up→forward)
            out->classic.ls_x = stick_to_classic_xy(pad.joystick_lx);
            out->classic.ls_y = stick_to_classic_xy(-pad.joystick_ly);
            // Right stick → classic RS (used for pointer): fix X/Y direction, deadzone to prevent drift up
            int16_t rs_x = classic_rs_apply_deadzone(pad.joystick_rx);
            int16_t rs_y = classic_rs_apply_deadzone(-pad.joystick_ry);
            out->classic.rs_x = (uint8_t)std::clamp((int)((float)rs_x / 32768.f * 15.f + 15.f), 0, 30);
            out->classic.rs_y = (uint8_t)std::clamp((int)((float)rs_y / 32768.f * 15.f + 15.f), 0, 30);
            // Mode switch: home+down held -> NO_EXTENSION
            if (out->classic.home && out->classic.down) {
                if (++s_home_down_hold_count >= MODE_SWITCH_HOLD_FRAMES) {
                    s_home_down_hold_count = 0;
                    out->classic.home = false;
                    out->classic.down = false;
                    out->switch_mode = 1;
                }
            } else {
                s_home_down_hold_count = 0;
            }
            if (out->switch_mode) {
                out->mode = NO_EXTENSION;
                out->switch_mode = 0;
                out->reset_ir = 1;
                flash_led_times(1);
            }
            break;
        }

        default:
            out->mode = NO_EXTENSION;
            out->switch_mode = 0;
            out->reset_ir = 0;
            break;
    }
}

} // namespace Wii

#endif
