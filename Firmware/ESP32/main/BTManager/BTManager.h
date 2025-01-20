#ifndef _BT_MANAGER_H_ 
#define _BT_MANAGER_H_

#include <cstdint>
#include <array>
#include <atomic>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "I2CDriver/I2CDriver.h"
#include "Gamepad/Gamepad.h"

class BTManager
{
public:
    static BTManager& get_instance()
    {
        static BTManager instance;
        return instance;
    }

    void run_task();
    bool any_connected();
    bool is_connected(uint8_t index);
    I2CDriver::PacketIn get_packet_in(uint8_t index);

private:
    BTManager() = default;
    ~BTManager() = default;
    BTManager(const BTManager&) = delete;
    BTManager& operator=(const BTManager&) = delete;

    static constexpr uint32_t FEEDBACK_TIME_MS = 200;
    static constexpr uint32_t LED_TIME_MS = 500;

    struct Device
    {
        std::atomic<bool> connected{false};
        GamepadMapper mapper;
        I2CDriver::PacketIn packet_in;
        std::atomic<I2CDriver::PacketOut> packet_out; //Can be updated from i2c thread
    };

    struct FBContext
    {
        uint8_t index;
        std::atomic<I2CDriver::PacketOut>* packet_out;
        btstack_context_callback_registration_t cb_reg;
    };

    std::array<Device, MAX_GAMEPADS> devices_;
    I2CDriver i2c_driver_;

    btstack_timer_source_t fb_timer_;
    bool fb_timer_running_ = false;

    void send_driver_type(DeviceDriverType driver_type);
    void manage_connection(uint8_t index, bool connected);
    
    static uni_hid_device_t* get_connected_bp32_device(uint8_t index);
    static void check_led_cb(btstack_timer_source *ts);
    static void send_feedback_cb(void* context);
    static void feedback_timer_cb(btstack_timer_source *ts);
    static void driver_update_timer_cb(btstack_timer_source *ts);

    //Bluepad32 driver

    void init(int argc, const char** arg_V);
    void init_complete_cb(void);
    uni_error_t device_discovered_cb(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi);
    void device_connected_cb(uni_hid_device_t* device);
    void device_disconnected_cb(uni_hid_device_t* device);
    uni_error_t device_ready_cb(uni_hid_device_t* device);
    void controller_data_cb(uni_hid_device_t* bp_device, uni_controller_t* controller);
    const uni_property_t* get_property_cb(uni_property_idx_t idx);
    void oob_event_cb(uni_platform_oob_event_t event, void* data);

    static uni_platform* get_bp32_driver();

}; // class BTManager

#endif // _BT_MANAGER_H_