#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include <cstdint>
#include <memory>

#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"
#include "USBDevice/DeviceDriver/DeviceDriver.h"

class DeviceManager {
public:
	DeviceManager(DeviceManager const&) = delete;
	void operator=(DeviceManager const&)  = delete;

    static DeviceManager& get_instance() {
		static DeviceManager instance;
		return instance;
	}

	//Must be called before any other method
	void initialize_driver(DeviceDriverType driver_type, Gamepad(&gamepads)[MAX_GAMEPADS]);
	
	DeviceDriver* get_driver() { return device_driver_.get(); }
	
private:
    DeviceManager() = default;
	~DeviceManager() = default;

	std::unique_ptr<DeviceDriver> device_driver_{nullptr};
};

#endif // _DEVICE_MANAGER_H_