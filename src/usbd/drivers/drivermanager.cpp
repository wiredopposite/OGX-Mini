#include "drivermanager.h"

// #include "net/NetDriver.h"
// #include "astro/AstroDriver.h"
// #include "egret/EgretDriver.h"
// #include "hid/HIDDriver.h"
// #include "keyboard/KeyboardDriver.h"
// #include "mdmini/MDMiniDriver.h"
// #include "neogeo/NeoGeoDriver.h"
// #include "pcengine/PCEngineDriver.h"
// #include "psclassic/PSClassicDriver.h"
// #include "ps4/PS4Driver.h"
#include "switch/SwitchDriver.h"
// #include "xbone/XBOneDriver.h"
#include "xboxog/XboxOriginalDriver.h"
#include "xinput/XInputDriver.h"

// ^ working on more of these

void DriverManager::setup(InputMode mode) {
    switch (mode) {
        // case INPUT_MODE_CONFIG:
        //     driver = new NetDriver(); 
        //     break;
        // case INPUT_MODE_ASTRO:
        //     driver = new AstroDriver();
        //     break;
        // case INPUT_MODE_EGRET:
        //     driver = new EgretDriver();
        //     break;
        // case INPUT_MODE_HID:
        //     driver = new HIDDriver();
        //     break;
        // case INPUT_MODE_KEYBOARD:
        //     driver = new KeyboardDriver();
        //     break;
        // case INPUT_MODE_MDMINI:
        //     driver = new MDMiniDriver();
        //     break;
        // case INPUT_MODE_NEOGEO:
        //     driver = new NeoGeoDriver();
        //     break;
        // case INPUT_MODE_PSCLASSIC:
        //     driver = new PSClassicDriver();
        //     break;
        // case INPUT_MODE_PCEMINI:
        //     driver = new PCEngineDriver();
        //     break;
        // case INPUT_MODE_PS4:
        //     driver = new PS4Driver();
        //     break;
        case INPUT_MODE_SWITCH:
            driver = new SwitchDriver();
            break;
        // case INPUT_MODE_XBONE:
        //     driver = new XBOneDriver();
        //     break;
        case INPUT_MODE_XBOXORIGINAL:
            driver = new XboxOriginalDriver();
            break;
        case INPUT_MODE_XINPUT:
            driver = new XInputDriver();
            break;
        default:
            return;
    }
    
    // Initialize our chosen driver
    driver->initialize();

    // Start the TinyUSB Device functionality
    tud_init(TUD_OPT_RHPORT);
}
