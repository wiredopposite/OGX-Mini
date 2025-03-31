#include "tusb.h"

#include "Board/Config.h"
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

void DeviceManager::initialize_driver(  DeviceDriverType driver_type, 
                                        Gamepad(&gamepads)[MAX_GAMEPADS]) {
    //TODO: Put gamepad setup in the drivers themselves
    bool has_analog = false; 
    
    switch (driver_type) {
        case DeviceDriverType::DINPUT:
            has_analog = true;
            device_driver_ = std::make_unique<DInputDevice>();
            break;
        case DeviceDriverType::PS3:
            has_analog = true;
            device_driver_ = std::make_unique<PS3Device>();
            break;
        case DeviceDriverType::PSCLASSIC:
            device_driver_ = std::make_unique<PSClassicDevice>();
            break;
        case DeviceDriverType::SWITCH:
            device_driver_ = std::make_unique<SwitchDevice>();
            break;
        case DeviceDriverType::XINPUT:
            device_driver_ = std::make_unique<XInputDevice>();
            break;
        case DeviceDriverType::XBOXOG:
            has_analog = true;
            device_driver_ = std::make_unique<XboxOGDevice>();
            break;
        case DeviceDriverType::XBOXOG_SB:
            device_driver_ = std::make_unique<XboxOGSBDevice>();
            break;
        case DeviceDriverType::XBOXOG_XR:
            device_driver_ = std::make_unique<XboxOGXRDevice>();
            break;
        case DeviceDriverType::WEBAPP:
            device_driver_ = std::make_unique<WebAppDevice>();
            break;
#if defined(CONFIG_EN_UART_BRIDGE)
        case DeviceDriverType::UART_BRIDGE:
            device_driver_ = std::make_unique<UARTBridgeDevice>();
            break;
#endif //defined(CONFIG_EN_UART_BRIDGE)
        default:
            return;
    }

    if (has_analog) {
        for (size_t i = 0; i < MAX_GAMEPADS; ++i) {
            gamepads[i].set_analog_device(true);
        }
    }

    device_driver_->initialize();
}