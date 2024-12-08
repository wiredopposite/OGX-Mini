#include "tusb.h"

#include "board_config.h"
#include "USBDevice/DeviceDriver/PSClassic/PSClassic.h"
#include "USBDevice/DeviceDriver/XInput/XInput.h"   
#include "USBDevice/DeviceDriver/Switch/Switch.h"
#include "USBDevice/DeviceDriver/DInput/DInput.h"
#include "USBDevice/DeviceDriver/PS3/PS3.h"
#include "USBDevice/DeviceDriver/XboxOG/XboxOG_GP.h"
#include "USBDevice/DeviceDriver/XboxOG/XboxOG_SB.h"
#include "USBDevice/DeviceDriver/XboxOG/XboxOG_XR.h"
#include "USBDevice/DeviceDriver/WebApp/WebApp.h"
#include "USBDevice/DeviceManager.h"

#if defined(CONFIG_EN_UART_BRIDGE)
#include "USBDevice/DeviceDriver/UARTBridge/UARTBridge.h"
#endif // defined(CONFIG_EN_UART_BRIDGE)

void DeviceManager::initialize_driver(DeviceDriver::Type driver_type)
{
    switch (driver_type)
    {
        case DeviceDriver::Type::DINPUT:
            device_driver_ = new DInputDevice();
            break;
        case DeviceDriver::Type::PS3:
            device_driver_ = new PS3Device();
            break;
        case DeviceDriver::Type::PSCLASSIC:
            device_driver_ = new PSClassicDevice();
            break;
        case DeviceDriver::Type::SWITCH:
            device_driver_ = new SwitchDevice();
            break;
        case DeviceDriver::Type::XINPUT:
            device_driver_ = new XInputDevice();
            break;
        case DeviceDriver::Type::XBOXOG:
            device_driver_ = new XboxOGDevice();
            break;
        case DeviceDriver::Type::XBOXOG_SB:
            device_driver_ = new XboxOGSBDevice();
            break;
        case DeviceDriver::Type::XBOXOG_XR:
            device_driver_ = new XboxOGXRDevice();
            break;
        case DeviceDriver::Type::WEBAPP:
            device_driver_ = new WebAppDevice();
            break;
#if defined(CONFIG_EN_UART_BRIDGE)
        case DeviceDriver::Type::UART_BRIDGE:
            device_driver_ = new UARTBridgeDevice();
            break;
#endif //defined(CONFIG_EN_UART_BRIDGE)
        default:
            return;
    }

    device_driver_->initialize();
}