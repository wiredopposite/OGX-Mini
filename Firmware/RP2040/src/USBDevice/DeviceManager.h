#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include <memory>

#include "USBDevice/DeviceDriver/DeviceDriver.h"

class DeviceManager 
{
public:
	DeviceManager(DeviceManager const&) = delete;
	void operator=(DeviceManager const&)  = delete;

    static DeviceManager& get_instance() 
    {
		static DeviceManager instance;
		return instance;
	}

	//Must be called before any other method
	void initialize_driver(DeviceDriver::Type driver_type);
	
	DeviceDriver* get_driver() { return device_driver_; }
	
private:
    DeviceManager() {}
	~DeviceManager() { delete device_driver_; }
    DeviceDriver* device_driver_{nullptr};
};

#endif // _DEVICE_MANAGER_H_