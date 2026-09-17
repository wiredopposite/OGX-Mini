#ifndef _SDK_CONFIG_H_
#define _SDK_CONFIG_H_

#include "Board/Config.h"

//
// Emulate "menuconfig"
//
/* Joy-Con L+R pair needs two BLE connections while USB may still expose one player. */
#if MAX_GAMEPADS >= 2
#define CONFIG_BLUEPAD32_MAX_DEVICES MAX_GAMEPADS
#else
#define CONFIG_BLUEPAD32_MAX_DEVICES 2
#endif
#define CONFIG_BLUEPAD32_MAX_ALLOWLIST 4
#define CONFIG_BLUEPAD32_GAP_SECURITY 1
#define CONFIG_BLUEPAD32_ENABLE_BLE_BY_DEFAULT 1
// #define CONFIG_BLUEPAD32_ENABLE_VIRTUAL_DEVICE_BY_DEFAULT 1

#define CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#define CONFIG_TARGET_PICO_W

// 2 == Info (Debug builds only; Release stays silent)
#if defined(CONFIG_OGXM_DEBUG)
#define CONFIG_BLUEPAD32_LOG_LEVEL 2
#else
#define CONFIG_BLUEPAD32_LOG_LEVEL 0
#endif

#endif //_SDK_CONFIG_H_