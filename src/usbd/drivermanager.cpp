#include "usbd/drivermanager.h"

#if (OGX_TYPE == WIRELESS) && (OGX_MCU != MCU_ESP32S3)
#include "usbd/drivers/uartbridge/UARTBridgeDriver.h"
#endif

// #include "usbd/drivers/hid/HIDDriver.h"
#include "usbd/drivers/dinput/DInputDriver.h"
// #include "usbd/drivers/ps3/PS3Driver.h"

#include "usbd/drivers/psclassic/PSClassicDriver.h"
#include "usbd/drivers/switch/SwitchDriver.h"
#include "usbd/drivers/xboxog/XboxOriginalDriver.h"
#include "usbd/drivers/xinput/XInputDriver.h"
#include "usbd/drivers/usbserial/USBSerialDriver.h"

void DriverManager::setup(InputMode mode) 
{
    // driver = new DInputDriver();
    switch (mode) 
    {
        case INPUT_MODE_HID:
            // driver = new HIDDriver();
            driver = new DInputDriver();
            // driver = new PS3Driver();
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
        #if (OGX_TYPE == WIRELESS) && (OGX_MCU != MCU_ESP32S3)
        case INPUT_MODE_UART_BRIDGE:
            driver = new UARTBridgeDriver();
            break;
        #endif
        default:
            return;
    }
    
    // Initialize our chosen driver
    driver->initialize();

    // Start the TinyUSB Device functionality
    tud_init(TUD_OPT_RHPORT);
}
