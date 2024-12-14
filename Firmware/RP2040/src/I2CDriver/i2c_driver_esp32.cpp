#include <array>
#include <cstring>
#include <pico/i2c_slave.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "Gamepad.h"
#include "board_config.h"
#include "Board/board_api.h"
#include "I2CDriver/i2c_driver_esp32.h"

namespace i2c_driver_esp32 {

//May expand commands in the future
enum class ReportID : uint8_t { UNKNOWN = 0, SET_PAD, GET_PAD };

#pragma pack(push, 1)
struct ReportIn
{
    uint8_t report_len;
    uint8_t report_id;
    uint8_t index;
    uint8_t dpad;
    uint16_t buttons;
    uint8_t trigger_l;
    uint8_t trigger_r;
    int16_t joystick_lx;
    int16_t joystick_ly;
    int16_t joystick_rx;
    int16_t joystick_ry;

    ReportIn()
    {
        std::memset(this, 0, sizeof(ReportIn));
        report_len = sizeof(ReportIn);
        report_id = static_cast<uint8_t>(ReportID::SET_PAD);
    }
};
static_assert(sizeof(ReportIn) == 16, "i2c_driver_esp::ReportIn size mismatch");

struct ReportOut
{
    uint8_t report_len;
    uint8_t report_id;
    uint8_t index;
    uint8_t rumble_l;
    uint8_t rumble_r;

    ReportOut()
    {
        std::memset(this, 0, sizeof(ReportOut));
        report_len = sizeof(ReportOut);
        report_id = static_cast<uint8_t>(ReportID::GET_PAD);
    }
};
static_assert(sizeof(ReportOut) == 5, "i2c_driver_esp::ReportOut size mismatch");
#pragma pack(pop)

static constexpr size_t MAX_BUFFER_SIZE = std::max(sizeof(ReportOut), sizeof(ReportIn));
static constexpr uint8_t I2C_ADDR = 0x01;

std::array<Gamepad*, MAX_GAMEPADS> gamepads_{nullptr};

inline void process_in_report(ReportIn* report_in)
{
    if (report_in->index >= MAX_GAMEPADS)
    {
        return;
    }

    Gamepad* gamepad = gamepads_[report_in->index];

    gamepad->set_buttons(report_in->buttons);
    gamepad->set_dpad(report_in->dpad);
    gamepad->set_trigger_l(report_in->trigger_l);
    gamepad->set_trigger_r(report_in->trigger_r);
    gamepad->set_joystick_lx_int10(static_cast<int32_t>(report_in->joystick_lx));
    gamepad->set_joystick_ly_int10(static_cast<int32_t>(report_in->joystick_ly));
    gamepad->set_joystick_rx_int10(static_cast<int32_t>(report_in->joystick_rx));
    gamepad->set_joystick_ry_int10(static_cast<int32_t>(report_in->joystick_ry));
    gamepad->set_new_report(true);
}

inline void fill_out_report(uint8_t index, ReportOut* report_out)
{
    if (index >= MAX_GAMEPADS)
    {
        return;
    }

    Gamepad* gamepad = gamepads_[index];
    
    report_out->report_len = sizeof(ReportOut);
    report_out->report_id = static_cast<uint8_t>(ReportID::GET_PAD);
    report_out->rumble_l = gamepad->get_rumble_l().uint8();
    report_out->rumble_r = gamepad->get_rumble_r().uint8();
}

ReportID get_report_id(uint8_t* buffer_in)
{
    switch (static_cast<ReportID>(buffer_in[1]))
    {
        case ReportID::SET_PAD:
            if (buffer_in[0] == sizeof(ReportIn))
            {
                return ReportID::SET_PAD;
            }
            break;
        //Unused ATM
        // case ReportID::GET_PAD:
        //     if (buffer_in[0] == sizeof(ReportOut))
        //     {
        //         return ReportID::GET_PAD;
        //     }
        //     break;
        default:
            break;
    }
    return ReportID::UNKNOWN;
}

static inline void slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    static int count = 0;
    static std::array<uint8_t, MAX_BUFFER_SIZE> buffer_in{0};
    static std::array<uint8_t, MAX_BUFFER_SIZE> buffer_out{0};

    switch (event) 
    {
        case I2C_SLAVE_RECEIVE: // master has written
            if (count < MAX_BUFFER_SIZE)
            {
                buffer_in.data()[count] = i2c_read_byte_raw(i2c);
                ++count;
            }
            break;

        case I2C_SLAVE_FINISH: // master signalled Stop / Restart
            if (get_report_id(buffer_in.data()) == ReportID::SET_PAD)
            {
                process_in_report(reinterpret_cast<ReportIn*>(buffer_in.data()));
            }
            count = 0;
            break;

        case I2C_SLAVE_REQUEST: // master requesting data
            fill_out_report(reinterpret_cast<ReportIn*>(buffer_in.data())->index, reinterpret_cast<ReportOut*>(buffer_out.data()));
            i2c_write_raw_blocking(i2c, buffer_out.data(), buffer_out.data()[0]);
            buffer_in.fill(0);
            break;

        default:
            break;
    }
}

void initialize(std::array<Gamepad, MAX_GAMEPADS>& gamepad)
{
    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads_[i] = &gamepad[i];
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