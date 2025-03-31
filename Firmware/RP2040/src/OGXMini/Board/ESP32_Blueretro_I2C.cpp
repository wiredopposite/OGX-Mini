#include "Board/Config.h"
#include "OGXMini/Board/ESP32_Blueretro_I2C.h"
#if (OGXM_BOARD == ESP32_BLUERETRO_I2C)

#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "UserSettings/UserSettings.h"
#include "USBDevice/DeviceManager.h"
#include "Board/board_api.h"
#include "Board/esp32_api.h"
#include "Gamepad/Gamepad.h"
#include "TaskQueue/TaskQueue.h"

#pragma pack(push, 1)
struct PacketIn {
    uint8_t          len{sizeof(PacketIn)};
    uint8_t          index{0};
    DeviceDriverType device_type{DeviceDriverType::NONE};
    uint8_t          gp_data[13]{0};
};
static_assert(sizeof(PacketIn) == 16, "i2c_driver_esp::PacketIn size mismatch");

struct PacketOut {
    uint8_t len{sizeof(PacketOut)};
    uint8_t rumble_l{0};
    uint8_t rumble_r{0};
    uint8_t reserved[5]{0};
};
static_assert(sizeof(PacketOut) == 8, "i2c_driver_esp::PacketOut size mismatch");
#pragma pack(pop)

constexpr uint8_t SLAVE_ADDR = 0x50;
constexpr uint32_t FEEDBACK_DELAY_MS = 250;

static Gamepad _gamepads[MAX_GAMEPADS];
static bool _uart_bridge_mode = false;

static void core1_task() {
    i2c_init(I2C_PORT, I2C_BAUDRATE);

    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);
    gpio_pull_up(I2C_SDA_PIN);

    bool slave_ready = false;
    PacketIn packet_in;
    PacketOut packet_out;
    Gamepad::PadIn pad_in;
    Gamepad& gamepad = _gamepads[0];
    uint32_t tid = TaskQueue::Core1::get_new_task_id();

    sleep_ms(500); // Wait for ESP32 to start

    TaskQueue::Core1::queue_delayed_task(tid, FEEDBACK_DELAY_MS, true, 
    [&packet_out, &gamepad, &slave_ready] { 
        if (!slave_ready) { // Check if slave present
            uint8_t addr = SLAVE_ADDR;
            int result = i2c_read_blocking(I2C_PORT, SLAVE_ADDR, &addr, 1, false);
            slave_ready = (result == 1);

        } else { // Update rumble
            Gamepad::PadOut pad_out = gamepad.get_pad_out();
            packet_out.rumble_l = pad_out.rumble_l;
            packet_out.rumble_r = pad_out.rumble_r;
            int result = i2c_write_blocking(I2C_PORT, SLAVE_ADDR, 
                                            reinterpret_cast<const uint8_t*>(&packet_out), 
                                            sizeof(PacketOut), false);

            if (result != sizeof(PacketOut)) {
                OGXM_LOG("I2C write failed\n");
            } else {
                OGXM_LOG("I2C sent rumble, L: %02X, R: %02X\n", 
                    packet_out.rumble_l, packet_out.rumble_r);
                sleep_ms(1);
            }
        }
    });

    OGXM_LOG("I2C Driver initialized\n");

    //Wait for slave to be detected
    while (!slave_ready) {
        TaskQueue::Core1::process_tasks();
        sleep_ms(100);
    }

    OGXM_LOG("I2C Slave ready\n");

    while (true) {
        TaskQueue::Core1::process_tasks();
        int result = i2c_read_blocking( I2C_PORT, SLAVE_ADDR, 
                                        reinterpret_cast<uint8_t*>(&packet_in), 
                                        sizeof(PacketIn), false);

        if (result == sizeof(PacketIn)) {
            std::memcpy(reinterpret_cast<uint8_t*>(&pad_in), 
                        packet_in.gp_data, 
                        sizeof(packet_in.gp_data));
            gamepad.set_pad_in(pad_in);

        } else {
            OGXM_LOG("I2C read failed\n");
            return;
        }
        sleep_ms(1);
    }
}

void set_gp_check_timer(uint32_t task_id) {
    UserSettings& user_settings = UserSettings::get_instance();
    
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true, 
    [&user_settings] {
        //Check gamepad inputs for button combo to change usb device driver
        if (user_settings.check_for_driver_change(_gamepads[0])) {
            OGXM_LOG("Driver change detected, storing new driver.\n");
            //This will store the new mode and reboot the pico
            user_settings.store_driver_type(user_settings.get_current_driver());
        }
    });
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

void esp32_br_i2c::initialize() {
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

void esp32_br_i2c::run() {
    if (_uart_bridge_mode) {
        run_uart_bridge();
        return;
    }

    esp32_api::reset();

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check);

    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();

    tud_init(BOARD_TUD_RHPORT);

    while (true) {
        TaskQueue::Core0::process_tasks();
        device_driver->process(0, _gamepads[0]);
        tud_task();
        sleep_ms(1);
    }
}

// #else // OGXM_BOARD == ESP32_BLUERETRO_I2C

// void esp32_br_i2c::initialize() {}
// void esp32_br_i2c::run() {}

#endif // OGXM_BOARD == ESP32_BLUERETRO_I2C