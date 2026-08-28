#ifndef _I2C_WIRE_PAD_H_
#define _I2C_WIRE_PAD_H_

#include <cstring>

#include "Gamepad/Gamepad.h"

/** Fixed-size pad payload for I2C master/slave links (32-byte PacketIn). Excludes IMU
 *  fields added to Gamepad::PadIn for DS4/DS5/Switch Pro — not used between 4ch boards. */
#pragma pack(push, 1)
struct I2CWirePadIn {
    uint8_t  dpad;
    uint16_t buttons;
    uint8_t  trigger_l;
    uint8_t  trigger_r;
    int16_t  joystick_lx;
    int16_t  joystick_ly;
    int16_t  joystick_rx;
    int16_t  joystick_ry;
    uint8_t  analog[10];
};
#pragma pack(pop)
static_assert(sizeof(I2CWirePadIn) == 23, "I2CWirePadIn size mismatch");

inline I2CWirePadIn i2c_wire_pad_from(const Gamepad::PadIn& pad_in)
{
    I2CWirePadIn wire{};
    wire.dpad = pad_in.dpad;
    wire.buttons = pad_in.buttons;
    wire.trigger_l = pad_in.trigger_l;
    wire.trigger_r = pad_in.trigger_r;
    wire.joystick_lx = pad_in.joystick_lx;
    wire.joystick_ly = pad_in.joystick_ly;
    wire.joystick_rx = pad_in.joystick_rx;
    wire.joystick_ry = pad_in.joystick_ry;
    std::memcpy(wire.analog, pad_in.analog, sizeof(wire.analog));
    return wire;
}

inline Gamepad::PadIn i2c_pad_from_wire(const I2CWirePadIn& wire)
{
    Gamepad::PadIn pad_in;
    pad_in.dpad = wire.dpad;
    pad_in.buttons = wire.buttons;
    pad_in.trigger_l = wire.trigger_l;
    pad_in.trigger_r = wire.trigger_r;
    pad_in.joystick_lx = wire.joystick_lx;
    pad_in.joystick_ly = wire.joystick_ly;
    pad_in.joystick_rx = wire.joystick_rx;
    pad_in.joystick_ry = wire.joystick_ry;
    std::memcpy(pad_in.analog, wire.analog, sizeof(pad_in.analog));
    return pad_in;
}

#endif
