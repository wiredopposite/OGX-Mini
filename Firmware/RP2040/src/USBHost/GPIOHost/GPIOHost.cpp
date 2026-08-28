#include "USBHost/GPIOHost/GPIOHost.h"
#include "Gamepad/Gamepad.h"
#include "USBHost/GPIOHost/psx_host/psx_host.h"
#include "USBHost/GPIOHost/joybus_host/joybus_controller.h"
#include "USBHost/GPIOHost/joybus_host/n64_host.h"
#include "USBHost/GPIOHost/dreamcast_host/MapleBus.h"
#include "USBHost/GPIOHost/dreamcast_host/MaplePacket.hpp"
#include "USBHost/GPIOHost/dreamcast_host/dreamcast_constants.h"
#include "USBHost/GPIOHost/dreamcast_host/dreamcast_structures.h"

#include <cstring>
#include <memory>

namespace GPIOHost {

/* PSX report: byte0=id, 1=buttons1 (dpad + start), 2=buttons2 (face + shoulders), 3=rx, 4=ry, 5=lx, 6=ly, 7,8 */
static constexpr uint8_t PSX_UP    = 0x10;
static constexpr uint8_t PSX_RIGHT = 0x20;
static constexpr uint8_t PSX_DOWN  = 0x40;
static constexpr uint8_t PSX_LEFT  = 0x80;
static constexpr uint8_t PSX_START = 0x08;
static constexpr uint8_t PSX_L2    = 0x01;
static constexpr uint8_t PSX_R2    = 0x02;
static constexpr uint8_t PSX_L1    = 0x04;
static constexpr uint8_t PSX_R1    = 0x08;
static constexpr uint8_t PSX_TRI   = 0x10;
static constexpr uint8_t PSX_CIR   = 0x20;
static constexpr uint8_t PSX_X     = 0x40;
static constexpr uint8_t PSX_SQU   = 0x80;

static inline int16_t psx_stick_to_int16(uint8_t b)
{
    int32_t v = (int32_t)(b & 0xFFu) - 128;
    if (v < -127) v = -127;
    if (v > 127) v = 127;
    return (int16_t)(v * 32767 / 127);
}

void psx_host_init(unsigned pio_index, unsigned gpio_cmd, unsigned gpio_data)
{
    ::psx_host_init(pio_index, gpio_cmd, gpio_data);
}

void psx_host_poll(Gamepad& gamepad)
{
    uint8_t buf[9];
    if (!::psx_host_take_data(buf))
    {
        return;
    }
    Gamepad::PadIn pad_in;
    std::memset(&pad_in, 0, sizeof(pad_in));

    uint8_t b1 = buf[1];
    uint8_t b2 = buf[2];
    if (b1 & PSX_UP)    pad_in.dpad |= Gamepad::DPAD_UP;
    if (b1 & PSX_DOWN)  pad_in.dpad |= Gamepad::DPAD_DOWN;
    if (b1 & PSX_LEFT)  pad_in.dpad |= Gamepad::DPAD_LEFT;
    if (b1 & PSX_RIGHT) pad_in.dpad |= Gamepad::DPAD_RIGHT;
    if (b1 & PSX_START) pad_in.buttons |= Gamepad::BUTTON_START;
    if (b2 & PSX_X)     pad_in.buttons |= Gamepad::BUTTON_A;
    if (b2 & PSX_CIR)   pad_in.buttons |= Gamepad::BUTTON_B;
    if (b2 & PSX_TRI)   pad_in.buttons |= Gamepad::BUTTON_X;
    if (b2 & PSX_SQU)   pad_in.buttons |= Gamepad::BUTTON_Y;
    if (b2 & PSX_L1)    pad_in.buttons |= Gamepad::BUTTON_LB;
    if (b2 & PSX_R1)    pad_in.buttons |= Gamepad::BUTTON_RB;
    pad_in.trigger_l = (b2 & PSX_L2) ? 0xFF : 0;
    pad_in.trigger_r = (b2 & PSX_R2) ? 0xFF : 0;

    pad_in.joystick_rx = psx_stick_to_int16(buf[3]);
    pad_in.joystick_ry = psx_stick_to_int16(buf[4]);
    pad_in.joystick_lx = psx_stick_to_int16(buf[5]);
    pad_in.joystick_ly = psx_stick_to_int16(buf[6]);

    gamepad.set_pad_in(pad_in);
}

/* GameCube report: buttons1, buttons2, lx, ly, rx, ry, l, r (same as GCReport). */
static constexpr uint8_t GC_A     = 0x01;
static constexpr uint8_t GC_B     = 0x02;
static constexpr uint8_t GC_X     = 0x04;
static constexpr uint8_t GC_Y     = 0x08;
static constexpr uint8_t GC_START = 0x10;
static constexpr uint8_t GC_DPAD_LEFT  = 0x01;
static constexpr uint8_t GC_DPAD_RIGHT = 0x02;
static constexpr uint8_t GC_DPAD_DOWN  = 0x04;
static constexpr uint8_t GC_DPAD_UP    = 0x08;
static constexpr uint8_t GC_Z     = 0x10;
static constexpr uint8_t GC_R     = 0x20;
static constexpr uint8_t GC_L     = 0x40;
static constexpr uint8_t STICK_CENTER = 0x80;

static int16_t gc_stick_to_int16(uint8_t b)
{
    int32_t v = (int32_t)(b & 0xFFu) - (int32_t)STICK_CENTER;
    if (v < -127) v = -127;
    if (v > 127) v = 127;
    return (int16_t)(v * 32767 / 127);
}

void joybus_host_init(unsigned pio_index, unsigned data_pin)
{
    ::joybus_host_init(pio_index, data_pin);
}

void joybus_host_poll(Gamepad& gamepad)
{
    static const uint8_t poll_cmd[] = { 0x40, 0x03 };
    ::joybus_send_data(poll_cmd, 2, 8);
    const uint8_t *r = ::joybus_get_response();

    Gamepad::PadIn pad_in;
    std::memset(&pad_in, 0, sizeof(pad_in));

    uint8_t b1 = r[0];
    uint8_t b2 = r[1];
    if (b2 & GC_DPAD_UP)    pad_in.dpad |= Gamepad::DPAD_UP;
    if (b2 & GC_DPAD_DOWN)  pad_in.dpad |= Gamepad::DPAD_DOWN;
    if (b2 & GC_DPAD_LEFT)  pad_in.dpad |= Gamepad::DPAD_LEFT;
    if (b2 & GC_DPAD_RIGHT) pad_in.dpad |= Gamepad::DPAD_RIGHT;
    if (b1 & GC_START) pad_in.buttons |= Gamepad::BUTTON_START;
    if (b1 & GC_A)    pad_in.buttons |= Gamepad::BUTTON_A;
    if (b1 & GC_B)    pad_in.buttons |= Gamepad::BUTTON_B;
    if (b1 & GC_X)    pad_in.buttons |= Gamepad::BUTTON_X;
    if (b1 & GC_Y)    pad_in.buttons |= Gamepad::BUTTON_Y;
    if (b2 & GC_L)    pad_in.buttons |= Gamepad::BUTTON_LB;
    if (b2 & GC_R)    pad_in.buttons |= Gamepad::BUTTON_RB;
    if (b2 & GC_Z)    pad_in.buttons |= Gamepad::BUTTON_MISC;
    pad_in.trigger_l = r[6];
    pad_in.trigger_r = r[7];

    pad_in.joystick_lx = gc_stick_to_int16(r[2]);
    pad_in.joystick_ly = gc_stick_to_int16(r[3]);
    pad_in.joystick_rx = gc_stick_to_int16(r[4]);
    pad_in.joystick_ry = gc_stick_to_int16(r[5]);

    gamepad.set_pad_in(pad_in);
}

/* N64 report: 4 bytes from PicoGamepadConverter layout. byte0: A=0x80, B=0x40, Z=0x20, START=0x10, D-pad low nibble. */
/* byte1: R=0x10, L=0x20, C buttons low nibble (C_UP=8, C_DOWN=4, C_LEFT=2, C_RIGHT=1). byte2/3: joy X, Y (0x80 center). */
static constexpr uint8_t N64_A     = 0x80;
static constexpr uint8_t N64_B     = 0x40;
static constexpr uint8_t N64_Z     = 0x20;
static constexpr uint8_t N64_START = 0x10;
static constexpr uint8_t N64_DPAD  = 0x0F;
static constexpr uint8_t N64_R     = 0x10;
static constexpr uint8_t N64_L     = 0x20;
static constexpr uint8_t N64_C     = 0x0F;
static constexpr uint8_t N64_C_UP   = 0x08;
static constexpr uint8_t N64_C_DOWN = 0x04;
static constexpr uint8_t N64_C_LEFT = 0x02;
static constexpr uint8_t N64_C_RIGHT = 0x01;
static constexpr uint8_t N64_JOY_CENTER = 0x80;

static int16_t n64_stick_to_int16(uint8_t b)
{
    int32_t v = (int32_t)(b & 0xFFu) - (int32_t)N64_JOY_CENTER;
    if (v < -127) v = -127;
    if (v > 127) v = 127;
    return (int16_t)(v * 32767 / 127);
}

void n64_host_init(unsigned pio_index, unsigned data_pin)
{
    ::n64_host_init(pio_index, data_pin);
}

void n64_host_poll(Gamepad& gamepad)
{
    ::n64_host_poll();
    const uint8_t* r = ::n64_host_get_response();

    Gamepad::PadIn pad_in;
    std::memset(&pad_in, 0, sizeof(pad_in));

    uint8_t b0 = r[0];
    uint8_t b1 = r[1];
    uint8_t dpad_nibble = b0 & N64_DPAD;
    if (dpad_nibble == 0x08 || dpad_nibble == 0x09 || dpad_nibble == 0x0A) pad_in.dpad |= Gamepad::DPAD_UP;
    if (dpad_nibble == 0x04 || dpad_nibble == 0x05 || dpad_nibble == 0x06) pad_in.dpad |= Gamepad::DPAD_DOWN;
    if (dpad_nibble == 0x02 || dpad_nibble == 0x06 || dpad_nibble == 0x0A) pad_in.dpad |= Gamepad::DPAD_LEFT;
    if (dpad_nibble == 0x01 || dpad_nibble == 0x05 || dpad_nibble == 0x09) pad_in.dpad |= Gamepad::DPAD_RIGHT;

    if (b0 & N64_START) pad_in.buttons |= Gamepad::BUTTON_START;
    if (b0 & N64_A)     pad_in.buttons |= Gamepad::BUTTON_A;
    if (b0 & N64_B)     pad_in.buttons |= Gamepad::BUTTON_B;
    if (b0 & N64_Z)     pad_in.buttons |= Gamepad::BUTTON_MISC;
    if (b1 & N64_L)     pad_in.buttons |= Gamepad::BUTTON_LB;
    if (b1 & N64_R)     pad_in.buttons |= Gamepad::BUTTON_RB;
    /* C buttons -> right stick (diagonals combine) */
    uint8_t c_nibble = b1 & N64_C;
    pad_in.joystick_rx = ((c_nibble & N64_C_RIGHT) ? 32767 : 0) - ((c_nibble & N64_C_LEFT) ? 32767 : 0);
    pad_in.joystick_ry = ((c_nibble & N64_C_DOWN) ? 32767 : 0) - ((c_nibble & N64_C_UP) ? 32767 : 0);

    pad_in.joystick_lx = n64_stick_to_int16(r[2]);
    pad_in.joystick_ly = n64_stick_to_int16(r[3]);

    gamepad.set_pad_in(pad_in);
}

static std::unique_ptr<MapleBus> s_dreamcast_bus;

void dreamcast_host_init(unsigned /* pio_in_index */, unsigned /* pio_out_index */, unsigned pin_a, int dir_pin) {
    s_dreamcast_bus = std::make_unique<MapleBus>(pin_a, dir_pin, true);
}

static void controller_condition_to_pad_in(const controller_condition_t& cond, Gamepad::PadIn& pad) {
    std::memset(&pad, 0, sizeof(pad));
    /* Dreamcast: 0 = pressed. PadIn: bit set = pressed */
    if (!cond.up)    pad.dpad |= Gamepad::DPAD_UP;
    if (!cond.down)  pad.dpad |= Gamepad::DPAD_DOWN;
    if (!cond.left)  pad.dpad |= Gamepad::DPAD_LEFT;
    if (!cond.right) pad.dpad |= Gamepad::DPAD_RIGHT;
    if (!cond.a)     pad.buttons |= Gamepad::BUTTON_A;
    if (!cond.b)     pad.buttons |= Gamepad::BUTTON_B;
    if (!cond.x)     pad.buttons |= Gamepad::BUTTON_X;
    if (!cond.y)     pad.buttons |= Gamepad::BUTTON_Y;
    if (!cond.start) pad.buttons |= Gamepad::BUTTON_START;
    if (!cond.c)     pad.buttons |= Gamepad::BUTTON_RB;
    if (!cond.z)     pad.buttons |= Gamepad::BUTTON_LB;
    if (!cond.d)     pad.buttons |= Gamepad::BUTTON_MISC;
    pad.trigger_l = cond.l;
    pad.trigger_r = cond.r;
    /* Sticks: 0–255, 128 center -> int16 -32767..32767 */
    auto stick8_to_16 = [](uint8_t v) -> int16_t {
        int32_t x = (int32_t)(v & 0xFFu) - 128;
        if (x < -127) x = -127;
        if (x > 127) x = 127;
        return (int16_t)(x * 32767 / 127);
    };
    pad.joystick_lx = stick8_to_16(cond.lAnalogLR);
    pad.joystick_ly = stick8_to_16(cond.lAnalogUD);
    pad.joystick_rx = stick8_to_16(cond.rAnalogLR);
    pad.joystick_ry = stick8_to_16(cond.rAnalogUD);
}

void dreamcast_host_poll(Gamepad& gamepad) {
    if (!s_dreamcast_bus) return;
    MapleBus& bus = *s_dreamcast_bus;
    if (bus.isBusy()) {
        MapleBus::Status st = bus.processEvents(time_us_64());
        (void)st;
        return;
    }
    MaplePacket req;
    req.setFrame(COMMAND_GET_CONDITION, MAPLE_CTRL_ADDR, MAPLE_HOST_ADDR, 0);
    req.payloadCount = 0;
    req.payloadByteOrder = MaplePacket::ByteOrder::HOST;
    if (!bus.write(req, true, MAPLE_RESPONSE_TIMEOUT_US, MaplePacket::ByteOrder::HOST))
        return;
    uint64_t deadline = time_us_64() + MAPLE_RESPONSE_TIMEOUT_US + 500;
    while (time_us_64() < deadline) {
        MapleBus::Status st = bus.processEvents(time_us_64());
        if (st.phase == MapleBus::Phase::READ_COMPLETE && st.readBuffer && st.readBufferLen >= 3) {
            uint32_t frame = st.readBuffer[0];
            uint8_t cmd = MaplePacket::Frame::getFrameCommand(frame, st.rxByteOrder);
            if (cmd == COMMAND_RESPONSE_DATA_XFER) {
                const uint32_t* pl = st.readBuffer + 1;
                controller_condition_t cond;
                memcpy(&cond, pl + 1, sizeof(cond));  /* skip function code word */
                Gamepad::PadIn pad;
                controller_condition_to_pad_in(cond, pad);
                gamepad.set_pad_in(pad);
            }
            return;
        }
        if (st.phase == MapleBus::Phase::READ_FAILED || st.phase == MapleBus::Phase::WRITE_FAILED)
            return;
        busy_wait_us(50);
    }
}

}  // namespace GPIOHost
