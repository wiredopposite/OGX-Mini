#include "usbd/drivermanager.h"

// #include "net/NetDriver.h"
// #include "keyboard/KeyboardDriver.h"

#include "usbd/hid/HIDDriver.h"
#include "usbd/psclassic/PSClassicDriver.h"
#include "usbd/switch/SwitchDriver.h"
#include "usbd/xboxog/XboxOriginalDriver.h"
#include "usbd/xinput/XInputDriver.h"
#include "usbd/usbserial/USBSerialDriver.h"

void DriverManager::setup(InputMode mode) 
{
    switch (mode) 
    {
        // case INPUT_MODE_CONFIG:
        //     driver = new NetDriver(); 
        //     break;
        // case INPUT_MODE_KEYBOARD:
        //     driver = new KeyboardDriver();
        //     break;
        case INPUT_MODE_HID:
            driver = new HIDDriver();
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
