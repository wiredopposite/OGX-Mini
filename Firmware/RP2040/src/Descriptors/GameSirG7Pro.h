#pragma once

#include <cstdint>
#include <cstring>

/**
 * GameSir G7 Pro (3537:1022)
 *
 * Wired HID gamepad (captured): 9-byte report, no report-ID prefix:
 *   [0] buttons face/shoulders  [1] system/triggers digital
 *   [2] hat (0x0F = neutral)    [3..6] LX LY RX RY (center 0x80)
 *   [7] LT analog               [8] RT analog
 *
 * Classic Bluetooth uses Report 0x07 (11 B) — see GameSirG7ProBt.
 * Vendor/config HID (0xFFF0) is a separate IF — do not claim it.
 */
namespace GameSirG7Pro
{
    static constexpr uint16_t VID = 0x3537;
    static constexpr uint16_t PID = 0x1022;

    static constexpr uint8_t DPAD_MASK = 0x0F;
    static constexpr uint8_t HAT_NEUTRAL = 0x0F;
    static constexpr uint8_t AXIS_MID = 0x80;

    /** Wired USB gamepad report length (no ID byte). */
    static constexpr uint16_t USB_REPORT_LEN = 9;

    /** Bluetooth / some Android HID: Report ID + 10 payload bytes. */
    static constexpr uint8_t REPORT_ID_BT = 0x07;
    static constexpr uint16_t BT_REPORT_LEN = 11;

    /** Wired byte0 / BT byte6 — face / shoulders. */
    namespace Buttons0
    {
        static constexpr uint8_t A  = 0x01;
        static constexpr uint8_t B  = 0x02;
        static constexpr uint8_t X  = 0x08;
        static constexpr uint8_t Y  = 0x10;
        static constexpr uint8_t LB = 0x40;
        static constexpr uint8_t RB = 0x80;
    }

    /** Wired byte1 / BT byte7 — triggers digital + menu + sticks. */
    namespace Buttons1
    {
        static constexpr uint8_t LT    = 0x01;
        static constexpr uint8_t RT    = 0x02;
        static constexpr uint8_t BACK  = 0x04;
        static constexpr uint8_t START = 0x08;
        static constexpr uint8_t L3    = 0x20;
        static constexpr uint8_t R3    = 0x40;
    }

    namespace DPad
    {
        static constexpr uint8_t UP         = 0x00;
        static constexpr uint8_t UP_RIGHT   = 0x01;
        static constexpr uint8_t RIGHT      = 0x02;
        static constexpr uint8_t DOWN_RIGHT = 0x03;
        static constexpr uint8_t DOWN       = 0x04;
        static constexpr uint8_t DOWN_LEFT  = 0x05;
        static constexpr uint8_t LEFT       = 0x06;
        static constexpr uint8_t UP_LEFT    = 0x07;
    }
}
