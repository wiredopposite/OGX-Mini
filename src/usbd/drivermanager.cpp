#include "usbd/drivermanager.h"

#include "usbd/hid/HIDDriver.h"
#include "usbd/dinput/DInputDriver.h"
#include "usbd/psclassic/PSClassicDriver.h"
#include "usbd/switch/SwitchDriver.h"
#include "usbd/xboxog/XboxOriginalDriver.h"
#include "usbd/xinput/XInputDriver.h"
#include "usbd/usbserial/USBSerialDriver.h"

void DriverManager::setup(InputMode mode) 
{
    // driver = new DInputDriver();
    switch (mode) 
    {
        case INPUT_MODE_HID:
            // driver = new HIDDriver();
            driver = new DInputDriver();
            break;
        case INPUT_MODE_PSCLASSIC:
            driver = new PSClassicDriver();
            break;
        case INPUT_MODE_SWITCH:
            driver = new SwitchDriver();
            break;
        case INPUT_MODE_XBOXORIGINAL:
            driver = new XboxOriginalDriver();
            break;
        case INPUT_MODE_XINPUT:
            driver = new XInputDriver();
            break;
        case INPUT_MODE_USBSERIAL:
            driver = new USBSerialDriver();
            break;
        default:
            return;
    }
    
    // Initialize our chosen driver
    driver->initialize();

    // Start the TinyUSB Device functionality
    tud_init(TUD_OPT_RHPORT);
}
