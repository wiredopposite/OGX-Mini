#include <array>
#include <cstring>
#include <pico/i2c_slave.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "Gamepad.h"
#include "board_config.h"
#include "Board/board_api.h"
#include "I2CDriver/ESP32/I2CDriver.h"
#include "TaskQueue/TaskQueue.h"

namespace I2CDriver {

//May expand commands in the future
enum class PacketID : uint8_t { UNKNOWN = 0, SET_PAD, GET_PAD };

#pragma pack(push, 1)
struct PacketIn
{
    uint8_t packet_len;
    uint8_t packet_id;
    uint8_t index;
    uint8_t gp_data[sizeof(Gamepad::PadIn) - sizeof(Gamepad::PadIn::analog) - sizeof(Gamepad::PadIn::chatpad)];

    PacketIn()
    {
        std::memset(this, 0, sizeof(PacketIn));
        packet_len = sizeof(PacketIn);
        packet_id = static_cast<uint8_t>(PacketID::SET_PAD);
    }
};
static_assert(sizeof(PacketIn) == 16, "i2c_driver_esp::PacketIn size mismatch");

struct PacketOut
{
    uint8_t packet_len;
    uint8_t packet_id;
    uint8_t index;
    Gamepad::PadOut pad_out;
    uint8_t reserved[3];

    PacketOut()
    {
        std::memset(this, 0, sizeof(PacketOut));
        packet_len = sizeof(PacketOut);
        packet_id = static_cast<uint8_t>(PacketID::GET_PAD);
    }
};
static_assert(sizeof(PacketOut) == 8, "i2c_driver_esp::PacketOut size mismatch");
#pragma pack(pop)

static constexpr size_t MAX_BUFFER_SIZE = std::max(sizeof(PacketOut), sizeof(PacketIn));
static constexpr uint8_t I2C_ADDR = 0x01;

Gamepad* gamepads_[MAX_GAMEPADS];

inline void process_in_packet(PacketIn* packet_in)
{
    Gamepad::PadIn gp_in;
    std::memcpy(&gp_in, packet_in->gp_data, sizeof(packet_in->gp_data));

    //This is a bandaid since the ESP32 doesn't have access to user profiles atm
    //Will update this once I write a BLE server for interfacing with the webapp

    Gamepad* gamepad = gamepads_[packet_in->index];
    Gamepad::PadIn mapped_gp_in;

    if (gp_in.dpad & Gamepad::DPAD_UP)    mapped_gp_in.dpad |= gamepad->MAP_DPAD_UP;
    if (gp_in.dpad & Gamepad::DPAD_DOWN)  mapped_gp_in.dpad |= gamepad->MAP_DPAD_DOWN;
    if (gp_in.dpad & Gamepad::DPAD_LEFT)  mapped_gp_in.dpad |= gamepad->MAP_DPAD_LEFT;
    if (gp_in.dpad & Gamepad::DPAD_RIGHT) mapped_gp_in.dpad |= gamepad->MAP_DPAD_RIGHT;

    if (gp_in.buttons & Gamepad::BUTTON_START)  mapped_gp_in.buttons |= gamepad->MAP_BUTTON_START;
    if (gp_in.buttons & Gamepad::BUTTON_BACK)   mapped_gp_in.buttons |= gamepad->MAP_BUTTON_BACK;
    if (gp_in.buttons & Gamepad::BUTTON_L3)     mapped_gp_in.buttons |= gamepad->MAP_BUTTON_L3;
    if (gp_in.buttons & Gamepad::BUTTON_R3)     mapped_gp_in.buttons |= gamepad->MAP_BUTTON_R3;
    if (gp_in.buttons & Gamepad::BUTTON_LB)     mapped_gp_in.buttons |= gamepad->MAP_BUTTON_LB;
    if (gp_in.buttons & Gamepad::BUTTON_RB)     mapped_gp_in.buttons |= gamepad->MAP_BUTTON_RB;
    if (gp_in.buttons & Gamepad::BUTTON_SYS)    mapped_gp_in.buttons |= gamepad->MAP_BUTTON_SYS;
    if (gp_in.buttons & Gamepad::BUTTON_A)      mapped_gp_in.buttons |= gamepad->MAP_BUTTON_A;
    if (gp_in.buttons & Gamepad::BUTTON_B)      mapped_gp_in.buttons |= gamepad->MAP_BUTTON_B;
    if (gp_in.buttons & Gamepad::BUTTON_X)      mapped_gp_in.buttons |= gamepad->MAP_BUTTON_X;
    if (gp_in.buttons & Gamepad::BUTTON_Y)      mapped_gp_in.buttons |= gamepad->MAP_BUTTON_Y;

    mapped_gp_in.trigger_l = gp_in.trigger_l;
    mapped_gp_in.trigger_r = gp_in.trigger_r;
    mapped_gp_in.joystick_lx = gp_in.joystick_lx;
    mapped_gp_in.joystick_ly = gp_in.joystick_ly;
    mapped_gp_in.joystick_rx = gp_in.joystick_rx;
    mapped_gp_in.joystick_ry = gp_in.joystick_ry;

    gamepad->set_pad_in(mapped_gp_in);
}

inline void fill_out_report(uint8_t index, PacketOut* report_out)
{
    if (index >= MAX_GAMEPADS)
    {
        return;
    }

    report_out->packet_len = sizeof(PacketOut);
    report_out->packet_id = static_cast<uint8_t>(PacketID::GET_PAD);
    report_out->index = index;
    report_out->pad_out = gamepads_[index]->get_pad_out();
}

PacketID get_packet_id(uint8_t* buffer_in)
{
    switch (static_cast<PacketID>(buffer_in[1]))
    {
        case PacketID::SET_PAD:
            if (buffer_in[0] == sizeof(PacketIn))
            {
                return PacketID::SET_PAD;
            }
            break;
        //Unused ATM
        // case PacketID::GET_PAD:
        //     if (buffer_in[0] == sizeof(PacketOut))
        //     {
        //         return PacketID::GET_PAD;
        //     }
        //     break;
        default:
            break;
    }
    return PacketID::UNKNOWN;
}

static inline void slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    static int count = 0;
    static std::array<uint8_t, MAX_BUFFER_SIZE> buffer_in{0};
    static std::array<uint8_t, MAX_BUFFER_SIZE> buffer_out{0};

    switch (event) 
    {
        case I2C_SLAVE_RECEIVE: // master has written
            if (count < sizeof(PacketIn))
            {
                buffer_in.data()[count] = i2c_read_byte_raw(i2c);
                ++count;
            }
            break;

        case I2C_SLAVE_FINISH: // master signalled Stop / Restart
            if (get_packet_id(buffer_in.data()) == PacketID::SET_PAD)
            {
                process_in_packet(reinterpret_cast<PacketIn*>(buffer_in.data()));
            }
            count = 0;
            break;

        case I2C_SLAVE_REQUEST: // master requesting data
            fill_out_report(reinterpret_cast<PacketIn*>(buffer_in.data())->index, reinterpret_cast<PacketOut*>(buffer_out.data()));
            i2c_write_raw_blocking(i2c, buffer_out.data(), buffer_out.data()[0]);
            buffer_in.fill(0);
            break;

        default:
            break;
    }
}

void initialize(Gamepad (&gamepads)[MAX_GAMEPADS])
{
    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads_[i] = &gamepads[i];
    }

    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_slave_init(I2C_PORT, I2C_ADDR, &slave_handler);
}

} // namespace i2c_driver_esp32