// #ifndef _I2C_DRIVER_H_
// #define _I2C_DRIVER_H_

// #include <cstdint>

// #include "Gamepad.h"
// #include "USBHost/HostManager.h"

// // /*  Master will only write/read slave if an Xbox360 wireless controller is connected for its corresponding slave.
// //     Slaves will read/write from the bus only if they don't have a device mounted by tusb host.
// //     Could be made to work so that a USB hub could be connected to port 1 and host 4 controllers, maybe later.  
// //     This is run on core0 with the device stack, but some values are checked and modified by core1 (atomic types). */
// // namespace i2c
// // {
// //     void initialize(Gamepad (&gamepads)[MAX_GAMEPADS]);

// //     struct driver 
// //     {
// //         void (*process)(){nullptr};
// //         bool (*is_active)(){nullptr};
// //         void (*notify_tud_deinit)(){nullptr};
// //         void (*notify_tuh_mounted)(HostDriver::Type host_type){nullptr};
// //         void (*notify_tuh_unmounted)(HostDriver::Type host_type){nullptr};
// //         void (*notify_xbox360w_connected)(uint8_t idx){nullptr};
// //         void (*notify_xbox360w_disconnected)(uint8_t idx){nullptr};
// //     };
    
// //     driver* get_driver();

// //     // void process();
// //     // bool is_active();

// //     // void notify_tud_deinit();

// //     // void notify_tuh_mounted(HostDriver::Type host_type = HostDriver::Type::UNKNOWN);
// //     // void notify_tuh_unmounted(HostDriver::Type host_type = HostDriver::Type::UNKNOWN);

// //     // void notify_xbox360w_connected(uint8_t idx);
// //     // void notify_xbox360w_disconnected(uint8_t idx);
// // }
// class I2CDriver4Ch
// {
// public:
//     I2CDriver4Ch& get_instance()
//     {
//         static I2CDriver4Ch instance;
//         return instance;
//     }
    
//     I2CDriver4Ch(const I2CDriver4Ch&) = delete;
//     I2CDriver4Ch& operator=(const I2CDriver4Ch&) = delete;

//     void initialize(Gamepad (&gamepads)[MAX_GAMEPADS]);

// private:
//     I2CDriver4Ch() = default;
//     ~I2CDriver4Ch() = default;

//     I2CDriver4Ch(const I2CDriver4Ch&) = delete;
//     I2CDriver4Ch& operator=(const I2CDriver4Ch&) = delete;

//     enum class Mode { MASTER = 0, SLAVE };
//     enum class ReportID : uint8_t { UNKNOWN = 0, PAD, STATUS, CONNECT, DISCONNECT };
//     enum class SlaveStatus : uint8_t { NC = 0, NOT_READY, READY, RESP_OK };

//     #pragma pack(push, 1)
//     struct ReportIn
//     {
//         uint8_t report_len;
//         uint8_t report_id;
//         uint8_t pad_in[sizeof(Gamepad::PadIn) - sizeof(Gamepad::PadIn::analog)];

//         ReportIn()
//         {
//             std::memset(this, 0, sizeof(ReportIn));
//             report_len = sizeof(ReportIn);
//             report_id = static_cast<uint8_t>(ReportID::PAD);
//         }
//     };
//     static_assert(sizeof(ReportIn) == 18, "I2CDriver::ReportIn size mismatch");

//     struct ReportOut
//     {
//         uint8_t report_len;
//         uint8_t report_id;
//         uint8_t pad_out[sizeof(Gamepad::PadOut)];

//         ReportOut()
//         {
//             std::memset(this, 0, sizeof(ReportOut));
//             report_len = sizeof(ReportOut);
//             report_id = static_cast<uint8_t>(ReportID::PAD);
//         }
//     };
//     static_assert(sizeof(ReportOut) == 4, "I2CDriver::ReportOut size mismatch");

//     struct ReportStatus
//     {
//         uint8_t report_len;
//         uint8_t report_id;
//         uint8_t status;

//         ReportStatus()
//         {
//             report_len = sizeof(ReportStatus);
//             report_id = static_cast<uint8_t>(ReportID::STATUS);
//             status = static_cast<uint8_t>(SlaveStatus::NC);
//         }
//     };
//     static_assert(sizeof(ReportStatus) == 3, "I2CDriver::ReportStatus size mismatch");
//     #pragma pack(pop)

//     static constexpr size_t MAX_BUFFER_SIZE = std::max(sizeof(ReportStatus), std::max(sizeof(ReportIn), sizeof(ReportOut)));

//     struct Master
//     {
//         std::atomic<bool> enabled{false};

//         struct Slave
//         {
//             uint8_t address{0xFF};
//             Gamepad* gamepad{nullptr};

//             SlaveStatus status{SlaveStatus::NC};
//             std::atomic<bool> enabled{false};
//             uint32_t last_update{0};
//         };

//         std::array<Slave, MAX_GAMEPADS - 1> slaves;
//     };

//     struct Slave
//     {
//         uint8_t address{0xFF};
//         Gamepad* gamepad{nullptr};
//         std::atomic<bool> i2c_enabled{false};
//         std::atomic<bool> tuh_enabled{false};
//     };

//     Slave slave_;

//     static Gamepad* gamepads_[MAX_GAMEPADS];
// };

// #endif // _I2C_DRIVER_H_