#ifndef _I2C_DRIVER_H_
#define _I2C_DRIVER_H_

#include <cstdint>

#include "Gamepad.h"
#include "USBHost/HostManager.h"

// /*  Master will only write/read slave if an Xbox360 wireless controller is connected for its corresponding slave.
//     Slaves will read/write from the bus only if they don't have a device mounted by tusb host.
//     Could be made to work so that a USB hub could be connected to port 1 and host 4 controllers, maybe later.  
//     This is run on core0 with the device stack, but some values are checked and modified by core1 (atomic types). */
namespace i2c_driver_4ch
{
    void initialize(std::array<Gamepad, MAX_GAMEPADS>& gamepads);
    void process();
    bool is_active();

    void notify_tud_deinit();

    void notify_tuh_mounted(HostDriver::Type host_type = HostDriver::Type::UNKNOWN);
    void notify_tuh_unmounted(HostDriver::Type host_type = HostDriver::Type::UNKNOWN);

    void notify_xbox360w_connected(uint8_t idx);
    void notify_xbox360w_disconnected(uint8_t idx);
}

#endif // _I2C_DRIVER_H_