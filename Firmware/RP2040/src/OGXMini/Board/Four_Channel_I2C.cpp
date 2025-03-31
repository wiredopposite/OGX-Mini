#include "Board/Config.h"
#include "OGXMini/Board/Four_Channel_I2C.h"
#if ((OGXM_BOARD == INTERNAL_4CH_I2C) || (OGXM_BOARD == EXTERNAL_4CH_I2C))

#include <atomic>
#include <cstring>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <pico/i2c_slave.h>

#include "tusb.h"
#include "bsp/board_api.h"
#include "pio_usb.h"

#include "USBDevice/DeviceManager.h"
#include "USBHost/HostManager.h"
#include "Board/board_api.h"
#include "Board/ogxm_log.h"
#include "UserSettings/UserSettings.h"
#include "Gamepad/Gamepad.h"
#include "TaskQueue/TaskQueue.h"

constexpr uint32_t FEEDBACK_DELAY_MS = 250;

Gamepad _gamepads[MAX_GAMEPADS];

namespace I2C {
    enum class Role {
        SLAVE = 0,
        MASTER
    };
    enum class PacketID : uint8_t { 
        UNKNOWN = 0, 
        PAD, 
        COMMAND 
    };
    enum class Command : uint8_t { 
        UNKNOWN = 0, 
        STATUS, 
        DISABLE 
    };
    enum class Status : uint8_t { 
        UNKNOWN = 0, 
        NC, 
        ERROR, 
        OK, 
        READY, 
        NOT_READY 
    };

    #pragma pack(push, 1)
    struct PacketIn {
        uint8_t             packet_len{sizeof(PacketIn)};
        PacketID            packet_id{PacketID::PAD};
        Gamepad::PadIn      pad_in{Gamepad::PadIn()};
        Gamepad::ChatpadIn  chatpad_in{0};
        uint8_t             reserved[4]{0};
    };
    static_assert(sizeof(PacketIn) == 32, "I2CDriver::PacketIn is misaligned");

    struct PacketOut {
        uint8_t         packet_len{sizeof(PacketOut)};
        PacketID        packet_id{PacketID::PAD};
        Gamepad::PadOut pad_out{Gamepad::PadOut()};
        uint8_t         reserved[4]{0};
    };
    static_assert(sizeof(PacketOut) == 8, "I2CDriver::PacketOut is misaligned");

    struct PacketCMD {
        uint8_t     packet_len{sizeof(PacketCMD)};
        PacketID    packet_id{PacketID::COMMAND};
        Command     command{Command::UNKNOWN};
        Status      status{Status::UNKNOWN};
        uint8_t     reserved[4]{0};
    };
    static_assert(sizeof(PacketCMD) == 8, "I2CDriver::PacketCMD is misaligned");
    #pragma pack(pop)

    constexpr size_t MAX_PACKET_SIZE = sizeof(PacketIn);

    static Role _i2c_role = Role::SLAVE;

    namespace Slave {
        static inline PacketID get_packet_id(uint8_t* buffer_in) {
            switch (static_cast<PacketID>(buffer_in[1])) {
                case PacketID::PAD:
                    if (buffer_in[0] == sizeof(PacketIn)) {
                        return PacketID::PAD;
                    }
                    break;
                case PacketID::COMMAND:
                    if (buffer_in[0] == sizeof(PacketCMD)) {
                        return PacketID::COMMAND;
                    }
                    break;
                default:
                    break;
            }
            return PacketID::UNKNOWN;
        }

        static void slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
            static size_t count = 0;
            static bool enabled = false;
            static uint8_t buffer_in[MAX_PACKET_SIZE];
            static uint8_t buffer_out[MAX_PACKET_SIZE];

            PacketIn  *packet_in_p = reinterpret_cast<PacketIn*>(buffer_in);
            PacketOut *packet_out_p = reinterpret_cast<PacketOut*>(buffer_out);
            PacketCMD *packet_cmd_in_p = reinterpret_cast<PacketCMD*>(buffer_in);
            PacketCMD *packet_cmd_out_p = reinterpret_cast<PacketCMD*>(buffer_out);

            switch (event) {
                case I2C_SLAVE_RECEIVE: // master has written
                    if (count < MAX_PACKET_SIZE) {
                        buffer_in[count] = i2c_read_byte_raw(i2c);
                        ++count;
                    }
                    break;

                case I2C_SLAVE_FINISH:
                    // Each master write has an ID indicating the type of data to send back on the next read request
                    // Every write has an associated read
                    switch (get_packet_id(buffer_in)) {
                        case PacketID::PAD:
                            // instance_->packet_in_ = *packet_in_p;
                            // *packet_out_p = instance_->packet_out_;
                            // instance_->new_pad_in_.store(true);
                            _gamepads[0].set_pad_in(packet_in_p->pad_in);
                            if (_gamepads[0].new_pad_out()) {
                                packet_out_p->pad_out = _gamepads[0].get_pad_out();
                            }
                            break;

                        case PacketID::COMMAND:
                            switch (packet_cmd_in_p->command) {
                                case Command::DISABLE:
                                    packet_cmd_out_p->packet_len = sizeof(PacketCMD);
                                    packet_cmd_out_p->packet_id = PacketID::COMMAND;
                                    packet_cmd_out_p->command = Command::DISABLE;
                                    packet_cmd_out_p->status = Status::OK;

                                    if (!tuh_mounted(BOARD_TUH_RHPORT)) {
                                        four_ch_i2c::host_mounted(false);
                                    }
                                    break;

                                case Command::STATUS:
                                    packet_cmd_out_p->packet_len = sizeof(PacketCMD);
                                    packet_cmd_out_p->packet_id = PacketID::COMMAND;
                                    packet_cmd_out_p->command = Command::STATUS;
                                    packet_cmd_out_p->status = 
                                        tuh_mounted(BOARD_TUH_RHPORT) ? Status::NOT_READY : Status::READY;

                                    if (!tuh_mounted(BOARD_TUH_RHPORT) && !enabled) {
                                        enabled = true;
                                        four_ch_i2c::host_mounted(true);
                                    }
                                    break;

                                default:
                                    break;
                            }
                            break;
                    }
                    count = 0;
                    std::memset(buffer_in, 0, sizeof(buffer_in));
                    break;

                case I2C_SLAVE_REQUEST:
                    i2c_write_raw_blocking(i2c, buffer_out, buffer_out[0]);
                    break;

                default:
                    break;
            }
        }
    } // namespace Slave

    namespace Master {
        struct Slave {
            uint8_t address{0xFF};
            Status  status{Status::NC};
            bool    enabled{false};
        };

        static constexpr size_t NUM_SLAVES = MAX_GAMEPADS - 1;
        static_assert(NUM_SLAVES > 0, "I2CMaster::NUM_SLAVES must be greater than 0 to use I2C");

        std::array<Slave, NUM_SLAVES> _slaves; 

        static inline bool read_blocking(uint8_t address, void* buffer, size_t len) {
            return (i2c_read_blocking(  I2C_PORT, address, reinterpret_cast<uint8_t*>(buffer), 
                                        len, false) == static_cast<int>(len));
        }

        static inline bool write_blocking(uint8_t address, void* buffer, size_t len) {
            return (i2c_write_blocking( I2C_PORT, address, reinterpret_cast<uint8_t*>(buffer), 
                                        len, false) == static_cast<int>(len));
        }

        static inline bool slave_detected(uint8_t address) {
            uint8_t dummy = 0;
            int result = i2c_write_timeout_us(I2C_PORT, address, &dummy, 0, false, 1000);
            return (result >= 0);
        }

        static void notify_disable(uint8_t address) {
            if (!slave_detected(address)) {
                return;
            }
            int retries = 10;

            while (retries--) {
                PacketCMD packet_cmd;
                packet_cmd.packet_id = PacketID::COMMAND;
                packet_cmd.command = Command::DISABLE;

                if (write_blocking(address, &packet_cmd, sizeof(PacketCMD))) {
                    if (read_blocking(address, &packet_cmd, sizeof(PacketCMD))) {
                        if (packet_cmd.status == Status::OK) {
                            break;
                        }
                    }
                }
                sleep_ms(1);
            }
        }

        static void process() {
            for (uint8_t i = 0; i < NUM_SLAVES; ++i) {
                Slave& slave = _slaves[i];

                if (!slave.enabled || !slave_detected(slave.address)) {
                    continue;
                }

                PacketCMD packet_cmd;
                packet_cmd.packet_id = PacketID::COMMAND;
                packet_cmd.command = Command::STATUS;

                if (!write_blocking(slave.address, &packet_cmd, sizeof(PacketCMD)) ||
                    !read_blocking(slave.address, &packet_cmd, sizeof(PacketCMD))) {
                    continue;
                }

                if (packet_cmd.status == Status::READY) {
                    Gamepad& gamepad = _gamepads[i + 1];
                    PacketIn packet_in;
                    packet_in.pad_in = gamepad.get_pad_in();
                    packet_in.chatpad_in = gamepad.get_chatpad_in();

                    if (write_blocking(slave.address, &packet_in, sizeof(PacketIn))) {
                        PacketOut packet_out;
                        if (read_blocking(slave.address, &packet_out, sizeof(PacketOut))) {
                            gamepad.set_pad_out(packet_out.pad_out);
                        }
                    }
                }
                sleep_ms(1);
            }
        }

        static void xbox360w_connect(bool connected, uint8_t idx) {
            if (idx < 1 || idx >= MAX_GAMEPADS) {
                return;
            }
            // This function can be called from core1 
            // so queue on core0 (i2c thread)
            TaskQueue::Core0::queue_task(
            [&slave = _slaves[idx - 1], connected]() {
                slave.enabled = connected;
                if (!connected) {
                    notify_disable(slave.address);
                }
            });
        }

        static void tuh_connect(bool connected, HostDriverType host_type) {
            if (host_type != HostDriverType::XBOX360W) {
                return;
            }
            if (!connected) {
                //Called from core1 so queue on core0
                TaskQueue::Core0::queue_task(
                []() {
                    for (auto& slave : _slaves) {
                        slave.enabled = false;
                        notify_disable(slave.address);
                    }
                });
            }
        }
    } // namespace Master

    Role role() {
        return _i2c_role;
    }

    uint8_t get_address() {
        gpio_init(SLAVE_ADDR_PIN_1);
        gpio_init(SLAVE_ADDR_PIN_2);
        gpio_pull_up(SLAVE_ADDR_PIN_1);
        gpio_pull_up(SLAVE_ADDR_PIN_2);

        if (gpio_get(SLAVE_ADDR_PIN_1) && gpio_get(SLAVE_ADDR_PIN_2)) {
            return 0x00;
        }
        else if (gpio_get(SLAVE_ADDR_PIN_1) && !gpio_get(SLAVE_ADDR_PIN_2)) {
            return 0x01;
        }
        else if (!gpio_get(SLAVE_ADDR_PIN_1) && gpio_get(SLAVE_ADDR_PIN_2)) {
            return 0x02;
        }
        else if (!gpio_get(SLAVE_ADDR_PIN_1) && !gpio_get(SLAVE_ADDR_PIN_2)) {
            return 0x03;
        }
        return 0xFF;
    }

    void initialize() {
        uint8_t i2c_address = get_address();
        _i2c_role = (i2c_address == 0xFF) ? Role::MASTER : _i2c_role = Role::SLAVE;

        i2c_init(I2C_PORT, I2C_BAUDRATE);

        gpio_init(I2C_SDA_PIN);
        gpio_init(I2C_SCL_PIN);
        gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
        gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
        gpio_pull_up(I2C_SDA_PIN);
        gpio_pull_up(I2C_SCL_PIN);

        if (_i2c_role == Role::SLAVE) {
            i2c_slave_init(I2C_PORT, i2c_address, &Slave::slave_handler);
        }
    }
} // namespace I2C

void core1_task() {
    HostManager& host_manager = HostManager::get_instance();
    host_manager.initialize(_gamepads);

    //Pico-PIO-USB will not reliably detect a hot plug on some boards, 
    //so monitor pins and init host stack after connection
    while(!board_api::usb::host_connected()) {
        sleep_ms(100);
    }

    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tuh_init(BOARD_TUH_RHPORT);

    uint32_t tid_feedback = TaskQueue::Core1::get_new_task_id();
    TaskQueue::Core1::queue_delayed_task(tid_feedback, FEEDBACK_DELAY_MS, true, 
    [&host_manager] {
        host_manager.send_feedback();
    });

    while (true) {
        TaskQueue::Core1::process_tasks();
        tuh_task();
    }
}

void set_gp_check_timer(uint32_t task_id) {
    UserSettings& user_settings = UserSettings::get_instance();
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true, 
    [&user_settings] {
        //Check gamepad inputs for button combo to change usb device driver
        if (user_settings.check_for_driver_change(_gamepads[0]))
        {
            //This will store the new mode and reboot the pico
            user_settings.store_driver_type(user_settings.get_current_driver());
        }
    });
}

void four_ch_i2c::wireless_connected(bool connected, uint8_t idx) {
    if (I2C::role() == I2C::Role::MASTER) {
        I2C::Master::xbox360w_connect(connected, idx);
    }
}

void four_ch_i2c::host_mounted(bool mounted) {
    static std::atomic<bool> tud_is_inited = false;

    board_api::set_led(mounted);

    if (!mounted && tud_is_inited.load()) {
        TaskQueue::Core0::queue_task([]() {
            OGXM_LOG("Disconnecting USB and rebooting.\n");
            board_api::usb::disconnect_all();
            board_api::reboot();
        });
    } else if (!tud_is_inited.load() && mounted) {
        TaskQueue::Core0::queue_task([]() { 
            OGXM_LOG("Initializing USB device stack.\n");
            tud_init(BOARD_TUD_RHPORT); 
            tud_is_inited.store(true);
        });
    }
}

void four_ch_i2c::host_mounted_w_type(bool mounted, HostDriverType host_type) {
    host_mounted(mounted);
    if (I2C::role() == I2C::Role::MASTER) {
        I2C::Master::tuh_connect(mounted, host_type);
    }
}

void four_ch_i2c::initialize() {
    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();

    board_api::init_board();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
        _gamepads[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager::get_instance().initialize_driver(user_settings.get_current_driver(), _gamepads);
}

void four_ch_i2c::run() {
    I2C::initialize();
    
    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    //Wait for something to call tud_init
    while (!tud_inited()) {
        TaskQueue::Core0::process_tasks();
        sleep_ms(100);
    }

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check);

    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();

    if (I2C::role() == I2C::Role::MASTER) {
        while (true) {
            TaskQueue::Core0::process_tasks();
            I2C::Master::process();
            device_driver->process(0, _gamepads[0]);
            tud_task();
            sleep_ms(1);
        }
    } else {
        while (true) {
            TaskQueue::Core0::process_tasks();
            device_driver->process(0, _gamepads[0]);
            tud_task();
            sleep_ms(1);
        }
    }
}

// #else // OGXM_BOARD == INTERNAL_4CH_I2C || OGXM_BOARD == EXTERNAL_4CH_I2C

// void four_ch_i2c::initialize() {}
// void four_ch_i2c::run() {}
// void four_ch_i2c::host_mounted_w_type(bool mounted, HostDriverType host_type) {}
// void four_ch_i2c::host_mounted(bool mounted) {}
// void four_ch_i2c::wireless_connected(bool connected, uint8_t idx) {}

#endif // OGXM_BOARD == INTERNAL_4CH || OGXM_BOARD == EXTERNAL_4CH