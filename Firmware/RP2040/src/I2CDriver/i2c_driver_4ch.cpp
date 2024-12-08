#include <cstdint>
#include <array>
#include <cstring>
#include <atomic>

#include <pico/i2c_slave.h>
#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "board_config.h"
#include "I2CDriver/i2c_driver_4ch.h"

namespace i2c_driver_4ch {

enum class Mode { MASTER = 0, SLAVE };
enum class ReportID : uint8_t { UNKNOWN = 0, PAD, STATUS, CONNECT, DISCONNECT };
enum class SlaveStatus : uint8_t { NC = 0, NOT_READY, READY, RESP_OK };

#pragma pack(push, 1)
struct ReportIn
{
    uint8_t report_len;
    uint8_t report_id;
    uint16_t buttons;
    uint8_t dpad;
    uint8_t trigger_l;
    uint8_t trigger_r;
    int16_t joystick_lx;
    int16_t joystick_ly;
    int16_t joystick_rx;
    int16_t joystick_ry;
    std::array<uint8_t, 3> chatpad;

    ReportIn()
    {
        std::memset(this, 0, sizeof(ReportIn));
        report_len = sizeof(ReportIn);
        report_id = static_cast<uint8_t>(ReportID::PAD);
    }
};
static_assert(sizeof(ReportIn) == 18, "I2CDriver::ReportIn size mismatch");

struct ReportOut
{
    uint8_t report_len;
    uint8_t report_id;
    uint8_t rumble_l;
    uint8_t rumble_r;

    ReportOut()
    {
        std::memset(this, 0, sizeof(ReportOut));
        report_len = sizeof(ReportOut);
        report_id = static_cast<uint8_t>(ReportID::PAD);
    }
};
static_assert(sizeof(ReportOut) == 4, "I2CDriver::ReportOut size mismatch");

struct ReportStatus
{
    uint8_t report_len;
    uint8_t report_id;
    uint8_t status;

    ReportStatus()
    {
        std::memset(this, 0, sizeof(ReportStatus));
        report_len = sizeof(ReportStatus);
        report_id = static_cast<uint8_t>(ReportID::STATUS);
        status = static_cast<uint8_t>(SlaveStatus::NC);
    }
};
static_assert(sizeof(ReportStatus) == 3, "I2CDriver::ReportStatus size mismatch");
#pragma pack(pop)

static constexpr size_t MAX_BUFFER_SIZE = std::max(sizeof(ReportStatus), std::max(sizeof(ReportIn), sizeof(ReportOut)));

namespace i2c_master
{
    struct Master
    {
        std::atomic<bool> enabled{false};

        struct Slave
        {
            uint8_t address{0xFF};
            Gamepad* gamepad{nullptr};

            SlaveStatus status{SlaveStatus::NC};
            std::atomic<bool> enabled{false};
            uint32_t last_update{0};
        };

        std::array<Slave, MAX_GAMEPADS - 1> slaves;
    };

    Master master_;

    bool slave_detected(uint8_t address);
    bool send_in_report(Master::Slave& slave);
    bool get_out_report(Master::Slave& slave);
    bool update_slave_enabled(uint8_t address, bool connected);
    void notify_tud_deinit();
    void update_slave_status(Master::Slave& slave);
    
    void notify_tuh_mounted(HostDriver::Type host_type);
    void notify_tuh_unmounted(HostDriver::Type host_type);
    void notify_xbox360w_connected(uint8_t idx);
    void notify_xbox360w_disconnected(uint8_t idx);
    bool is_active();
    void process();
    void init(std::array<Gamepad, MAX_GAMEPADS>& gamepads);
}

namespace i2c_slave
{
    struct Slave
    {
        uint8_t address{0xFF};
        Gamepad* gamepad{nullptr};
        std::atomic<bool> i2c_enabled{false};
        std::atomic<bool> tuh_enabled{false};
    };

    Slave slave_;

    void process_in_report(ReportIn* report_in);
    void fill_out_report(ReportOut* report_out);
    ReportID get_report_id(uint8_t* buffer_in);
    void slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event);

    bool is_active();
    void notify_tuh_mounted();
    void notify_tuh_unmounted();
    void init(uint8_t address, Gamepad& gamepad);
}

namespace i2c_master {

bool is_active()
{
    return master_.enabled.load();
}

bool slave_detected(uint8_t address)
{
    uint8_t dummy_data = 0;
    int result = i2c_write_timeout_us(I2C_PORT, address, &dummy_data, 0, false, 1000);
    return (result >= 0);
}

void notify_tuh_mounted(HostDriver::Type host_type)
{
    if (host_type == HostDriver::Type::XBOX360W)
    {
        master_.enabled.store(true);
    }
}

void notify_tuh_unmounted(HostDriver::Type host_type)
{
    master_.enabled.store(false);
}

void notify_xbox360w_connected(uint8_t idx)
{
    if (idx < 1 || idx >= MAX_GAMEPADS)
    {
        return;
    }
    master_.slaves[idx - 1].enabled.store(true);
}

void notify_xbox360w_disconnected(uint8_t idx)
{
    if (idx < 1 || idx >= MAX_GAMEPADS)
    {
        return;
    }
    master_.slaves[idx - 1].enabled.store(false);
    // bool any_connected = false;
    // for (auto& slave : master_.slaves)
    // {
    //     if (slave.enabled.load())
    //     {
    //         any_connected = true;
    //         break;
    //     }
    // }
    // if (!any_connected)
    // {
    //     master_.enabled.store(false);
    // }
}

bool send_in_report(Master::Slave& slave)
{
    static ReportIn report_in = ReportIn();
    
    report_in.buttons       = slave.gamepad->get_buttons();
    report_in.dpad          = slave.gamepad->get_dpad_buttons();
    report_in.trigger_l     = slave.gamepad->get_trigger_l().uint8();
    report_in.trigger_r     = slave.gamepad->get_trigger_r().uint8();
    report_in.joystick_lx   = slave.gamepad->get_joystick_lx().int16();
    report_in.joystick_ly   = slave.gamepad->get_joystick_ly().int16();
    report_in.joystick_rx   = slave.gamepad->get_joystick_rx().int16();
    report_in.joystick_ry   = slave.gamepad->get_joystick_ry().int16();
    report_in.chatpad       = slave.gamepad->get_chatpad();

    int count = i2c_write_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&report_in), sizeof(report_in), false);
    return (count == sizeof(ReportIn));
}

bool get_out_report(Master::Slave& slave)
{
    static ReportOut report_out = ReportOut();

    int count = i2c_read_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&report_out), sizeof(ReportOut), false);
    if (count != sizeof(ReportOut))
    {
        return false;
    }

    slave.gamepad->set_rumble_l(report_out.rumble_l);
    slave.gamepad->set_rumble_r(report_out.rumble_r);

    return true;
}

bool update_slave_enabled(uint8_t address, bool connected)
{
    ReportStatus report;
    report.report_id = connected ? static_cast<uint8_t>(ReportID::CONNECT) : static_cast<uint8_t>(ReportID::DISCONNECT);
    int count = i2c_write_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(&report), sizeof(ReportStatus), false);
    if (count == sizeof(ReportStatus))
    {
        count = i2c_read_blocking(I2C_PORT, address, reinterpret_cast<uint8_t*>(&report), sizeof(ReportStatus), false);
        return (static_cast<SlaveStatus>(report.status) == SlaveStatus::RESP_OK);
    }
    return false;
}

void notify_tud_deinit()
{
    for (auto& slave : master_.slaves)
    {
        if (slave.status != SlaveStatus::NC)
        {
            int retries = 0;
            while (!update_slave_enabled(slave.address, false) && retries < 6)
            {
                sleep_ms(1);
                ++retries;
            }
        }
    }
}

void update_slave_status(Master::Slave& slave)
{
    bool slave_enabled = slave.enabled.load();

    if (!slave_detected(slave.address))
    {
        slave.status = SlaveStatus::NC;
        return;
    }
    if (!update_slave_enabled(slave.address, slave_enabled))
    {
        slave.status = SlaveStatus::NOT_READY;
        return;
    }
    if (slave_enabled)
    {
        ReportStatus report = ReportStatus();
        int count = 0;

        report.report_id = static_cast<uint8_t>(ReportID::STATUS);
        count = i2c_write_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&report), sizeof(ReportStatus), false);
        if (count == sizeof(ReportStatus))
        {
            count = i2c_read_blocking(I2C_PORT, slave.address, reinterpret_cast<uint8_t*>(&report), sizeof(ReportStatus), false);
            slave.status = (count == sizeof(ReportStatus)) ? static_cast<SlaveStatus>(report.status) : SlaveStatus::NC;
        }
        else
        {
            slave.status = SlaveStatus::NOT_READY;
        }
    }
    else
    {
        slave.status = SlaveStatus::NOT_READY;
    }
}

void process()
{
    for (auto& slave : master_.slaves)
    {
        if ((time_us_32() / 1000) - slave.last_update > 1000)
        {
            update_slave_status(slave);
            slave.last_update = time_us_32() / 1000;
        }
        sleep_us(100);
    }
    if (!master_.enabled.load())
    {
        return;
    }
    for (auto& slave : master_.slaves)
    {
        if (slave.status == SlaveStatus::READY)
        {
            if (send_in_report(slave))
            {
                get_out_report(slave);
            }
            sleep_us(100);
        }
    }
}

void init(std::array<Gamepad, MAX_GAMEPADS>& gamepads)
{
    for (uint8_t i = 0; i < master_.slaves.size(); ++i)
    {
        master_.slaves[i].address = i + 1;
        master_.slaves[i].gamepad = &gamepads[i + 1];
    }
}

} // namespace i2c_master

namespace i2c_slave {

bool is_active()
{
    return slave_.i2c_enabled.load();
}

void notify_tuh_mounted()
{
    slave_.tuh_enabled.store(true);
}

void notify_tuh_unmounted()
{
    slave_.tuh_enabled.store(false);
}

void process_in_report(ReportIn* report_in)
{
    Gamepad& gamepad = *slave_.gamepad;
    
    gamepad.set_buttons(report_in->buttons);
    gamepad.set_dpad(report_in->dpad);
    gamepad.set_trigger_l(report_in->trigger_l);
    gamepad.set_trigger_r(report_in->trigger_r);
    gamepad.set_joystick_lx(report_in->joystick_lx);
    gamepad.set_joystick_ly(report_in->joystick_ly);
    gamepad.set_joystick_rx(report_in->joystick_rx);
    gamepad.set_joystick_ry(report_in->joystick_ry);
    gamepad.set_chatpad(report_in->chatpad);
}

void fill_out_report(ReportOut* report_out)
{
    Gamepad& gamepad = *slave_.gamepad;

    report_out->report_len = sizeof(ReportOut);
    report_out->report_id = static_cast<uint8_t>(ReportID::PAD);
    report_out->rumble_l = gamepad.get_rumble_l().uint8();
    report_out->rumble_r = gamepad.get_rumble_r().uint8();
}

ReportID get_report_id(uint8_t* buffer_in)
{
    switch (static_cast<ReportID>(buffer_in[1]))
    {
        case ReportID::PAD:
            if (buffer_in[0] == sizeof(ReportIn))
            {
                return ReportID::PAD;
            }
            break;
        case ReportID::DISCONNECT:
            if (buffer_in[0] == sizeof(ReportStatus))
            {
                return ReportID::DISCONNECT;
            }
            break;
        case ReportID::CONNECT:
            if (buffer_in[0] == sizeof(ReportStatus))
            {
                return ReportID::CONNECT;
            }
            break;
        case ReportID::STATUS:
            if (buffer_in[0] == sizeof(ReportStatus))
            {
                return ReportID::STATUS;
            }
            break;
        default:
            break;
    }
    return ReportID::UNKNOWN;
}

void slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
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
            // Each master write has an ID indicating the type of data to send back on the next request
            // Every write has an associated read
            switch (get_report_id(buffer_in.data())) 
            {
                case ReportID::PAD:
                    process_in_report(reinterpret_cast<ReportIn*>(buffer_in.data()));
                    fill_out_report(reinterpret_cast<ReportOut*>(buffer_out.data()));
                    break;
                case ReportID::STATUS:
                    buffer_out.data()[0] = sizeof(ReportStatus);
                    buffer_out.data()[1] = static_cast<uint8_t>(ReportID::STATUS);
                    // if something is mounted by tuh, signal to not send gamepad data
                    buffer_out.data()[2] = slave_.tuh_enabled.load() ? static_cast<uint8_t>(SlaveStatus::NOT_READY) : static_cast<uint8_t>(SlaveStatus::READY);
                    break;
                case ReportID::CONNECT:
                    slave_.i2c_enabled.store(true);
                    buffer_out.data()[0] = sizeof(ReportStatus);
                    buffer_out.data()[1] = static_cast<uint8_t>(ReportID::STATUS);
                    buffer_out.data()[2] = static_cast<uint8_t>(SlaveStatus::RESP_OK);
                    break;
                case ReportID::DISCONNECT:
                    slave_.i2c_enabled.store(false);
                    buffer_out.data()[0] = sizeof(ReportStatus);
                    buffer_out.data()[1] = static_cast<uint8_t>(ReportID::STATUS);
                    buffer_out.data()[2] = static_cast<uint8_t>(SlaveStatus::RESP_OK);
                    break;
                default:
                    break;
            }
            count = 0;
            break;

        case I2C_SLAVE_REQUEST: // master requesting data
            i2c_write_raw_blocking(i2c, buffer_out.data(), buffer_out.data()[0]);
            buffer_in.fill(0);
            break;

        default:
            break;

    }
}

void init(uint8_t address, Gamepad& gamepad)
{
    slave_.address = address;
    slave_.gamepad = &gamepad;
    i2c_slave_init(I2C_PORT, slave_.address, &slave_handler);
}

} // namespace i2c_slave

Mode mode_{Mode::MASTER};

uint8_t get_address()
{
    gpio_init(SLAVE_ADDR_PIN_1);
    gpio_init(SLAVE_ADDR_PIN_2);
    gpio_pull_up(SLAVE_ADDR_PIN_1);
    gpio_pull_up(SLAVE_ADDR_PIN_2);

    if (gpio_get(SLAVE_ADDR_PIN_1) == 1 && gpio_get(SLAVE_ADDR_PIN_2) == 1)
    {
        return 0x00;
    }
    else if (gpio_get(SLAVE_ADDR_PIN_1) == 1 && gpio_get(SLAVE_ADDR_PIN_2) == 0)
    {
        return 0x01;
    }
    else if (gpio_get(SLAVE_ADDR_PIN_1) == 0 && gpio_get(SLAVE_ADDR_PIN_2) == 1)
    {
        return 0x02;
    }
    else if (gpio_get(SLAVE_ADDR_PIN_1) == 0 && gpio_get(SLAVE_ADDR_PIN_2) == 0)
    {
        return 0x03;
    }

    return 0xFF;
}

//Public methods

//Call before core1 starts
void initialize(std::array<Gamepad, MAX_GAMEPADS>& gamepads)
{
    uint8_t address = get_address();
    if (address == 0xFF)
    {
        return;
    }

    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    if (address < 1)
    {
        mode_ = Mode::MASTER;
        i2c_master::init(gamepads);
    }
    else
    {
        mode_ = Mode::SLAVE;
        i2c_slave::init(address, gamepads.front());
    }
}

void process()
{
    switch (mode_)
    {
        case Mode::MASTER:
            i2c_master::process();
            break;
        case Mode::SLAVE:
            return;
    }
}

bool is_active()
{
    switch (mode_)
    {
        case Mode::MASTER:
            return i2c_master::is_active();
        case Mode::SLAVE:
            return i2c_slave::is_active();
    }
    return false;
}

//Called from core1
void notify_tuh_mounted(HostDriver::Type host_type)
{
    switch (mode_)
    {
        case Mode::MASTER:
            i2c_master::notify_tuh_mounted(host_type);
            break;
        case Mode::SLAVE:
            i2c_slave::notify_tuh_mounted();
            break;
    }
}
//Called from core1
void notify_tuh_unmounted(HostDriver::Type host_type)
{
    switch (mode_)
    {
        case Mode::MASTER:
            i2c_master::notify_tuh_unmounted(host_type);
            break;
        case Mode::SLAVE:
            i2c_slave::notify_tuh_unmounted();
            break;
    }
}
//Called from core1
void notify_xbox360w_connected(uint8_t idx)
{
    switch (mode_)
    {
        case Mode::MASTER:
            i2c_master::notify_xbox360w_connected(idx);
            break;
        case Mode::SLAVE:
            return;
    }
}
//Called from core1
void notify_xbox360w_disconnected(uint8_t idx)
{
    switch (mode_)
    {
        case Mode::MASTER:
            i2c_master::notify_xbox360w_disconnected(idx);
            break;
        case Mode::SLAVE:
            return;
    }
}

void notify_tud_deinit()
{
    switch (mode_)
    {
        case Mode::MASTER:
            i2c_master::notify_tud_deinit();
            break;
        case Mode::SLAVE:
            return;
    }
}

} // namespace I2CDriver