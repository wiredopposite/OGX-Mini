#include "Board/Config.h"
#include "OGXMini/Board/ESP32_Bluepad32_I2C.h"
#if (OGXM_BOARD == ESP32_BLUEPAD32_I2C)

#include <cstring>
#include <pico/multicore.h>
#include <pico/i2c_slave.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "UserSettings/UserSettings.h"
#include "Board/board_api.h"
#include "Board/esp32_api.h"
#include "Gamepad/Gamepad.h"
#include "TaskQueue/TaskQueue.h"

enum class PacketID : uint8_t { 
    UNKNOWN = 0, 
    SET_PAD, 
    GET_PAD, 
    SET_DRIVER 
};

#pragma pack(push, 1)
struct PacketIn {
    uint8_t             packet_len{sizeof(PacketIn)};
    PacketID            packet_id{PacketID::SET_PAD};
    uint8_t             index{0};
    DeviceDriverType    device_type{DeviceDriverType::NONE};
    Gamepad::PadIn      pad_in{Gamepad::PadIn()};
    uint8_t             reserved[5]{0};
};
static_assert(sizeof(PacketIn) == 32, "i2c_driver_esp::PacketIn size mismatch");

struct PacketOut {
    uint8_t         packet_len{sizeof(PacketOut)};
    PacketID        packet_id{PacketID::GET_PAD};
    uint8_t         index{0};
    Gamepad::PadOut pad_out{Gamepad::PadOut()};
    uint8_t         reserved[3]{0};
};
static_assert(sizeof(PacketOut) == 8, "i2c_driver_esp::PacketOut size mismatch");
#pragma pack(pop)

constexpr size_t  MAX_BUFFER_SIZE = std::max(sizeof(PacketOut), sizeof(PacketIn));
constexpr uint8_t I2C_ADDR = 0x01;

static Gamepad _gamepads[MAX_GAMEPADS];
static bool _uart_bridge_mode = false;

static inline void slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    static size_t count = 0;
    static PacketIn packet_in;
    static PacketOut packet_out;
    static DeviceDriverType current_device_type = 
        UserSettings::get_instance().get_current_driver();

    switch (event) {
        case I2C_SLAVE_RECEIVE:
            if (count < sizeof(PacketIn)) {
                reinterpret_cast<uint8_t*>(&packet_in)[count] = i2c_read_byte_raw(i2c);
                ++count;
            }
            break;
        case I2C_SLAVE_FINISH:
            if (count == 0) {
                break;
            }
            switch (packet_in.packet_id) {
                case PacketID::SET_PAD:
                    if (packet_in.index < MAX_GAMEPADS) {
                        _gamepads[packet_in.index].set_pad_in(packet_in.pad_in);
                    }
                    break;
                case PacketID::SET_DRIVER:
                    if (packet_in.device_type != DeviceDriverType::NONE &&
                        packet_in.device_type != current_device_type) {
                        OGXM_LOG("I2C: Driver change detected.\n");
                        //Any writes to flash should be done on Core0
                        TaskQueue::Core0::queue_delayed_task(
                            TaskQueue::Core0::get_new_task_id(), 1000, false, 
                            [new_device_type = packet_in.device_type] { 
                                UserSettings::get_instance().store_driver_type(new_device_type);
                            }
                        );
                    }
                    break;
                default:
                    break;
            }
            count = 0;
            break;
        case I2C_SLAVE_REQUEST:
            if (packet_in.index < MAX_GAMEPADS) {
                packet_out.index = packet_in.index;
                packet_out.pad_out = _gamepads[packet_in.index].get_pad_out();
            }
            i2c_write_raw_blocking( i2c, 
                                    reinterpret_cast<const uint8_t*>(&packet_out), 
                                    packet_out.packet_len);
            break;
        default:
            break;
    }
}

static void core1_task() {
    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_init(I2C_SDA_PIN);
    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_slave_init(I2C_PORT, I2C_ADDR, &slave_handler);

    OGXM_LOG("I2C Driver initialized\n");

    while (true) {
        tight_loop_contents();
    }
}

void run_uart_bridge() {
    esp32_api::enter_programming_mode();

    OGXM_LOG("Entering UART Bridge mode\n");

    //Runs UART Bridge task, doesn't return unless programming is complete
    DeviceManager::get_instance().get_driver()->process(0, _gamepads[0]); 

    OGXM_LOG("Exiting UART Bridge mode\n");

    board_api::usb::disconnect_all(); 
    UserSettings::get_instance().write_datetime();
    board_api::reboot();
}

bool update_needed(UserSettings& user_settings) {
#if defined(OGXM_RETAIL)
    return !user_settings.verify_datetime();
#endif
    return false;
}

void esp32_bp32_i2c::initialize() {
    board_api::init_board();
    esp32_api::init();

    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();

    //MODE_SEL_PIN is used to determine if UART bridge should be run
    _uart_bridge_mode = 
        (esp32_api::uart_bridge_mode() || update_needed(user_settings));

    DeviceDriverType driver_type = 
        _uart_bridge_mode ? 
            DeviceDriverType::UART_BRIDGE : user_settings.get_current_driver();

    DeviceManager::get_instance().initialize_driver(driver_type, _gamepads);
}

void esp32_bp32_i2c::run() {
    if (_uart_bridge_mode) {
        run_uart_bridge();
        return;
    }

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    esp32_api::reset();

    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();

    tud_init(BOARD_TUD_RHPORT);

    while (true) {
        TaskQueue::Core0::process_tasks();

        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
            device_driver->process(i, _gamepads[i]);
            tud_task();
        }
        sleep_ms(1);
    }
}

// #else // OGXM_BOARD == ESP32_BLUEPAD32_I2C

// void esp32_bp32_i2c::initialize() {}
// void esp32_bp32_i2c::run() {}

#endif // OGXM_BOARD == ESP32_BLUEPAD32_I2C