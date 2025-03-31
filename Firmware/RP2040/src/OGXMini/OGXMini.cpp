#include "Board/Config.h"
#include "OGXMini/Board/Standard.h"
#include "OGXMini/Board/PicoW.h"
#include "OGXMini/Board/Four_Channel_I2C.h"
#include "OGXMini/Board/ESP32_Blueretro_I2C.h"
#include "OGXMini/Board/ESP32_Bluepad32_I2C.h"
#include "OGXMini/OGXMini.h"

namespace OGXMini {
    typedef void (*InitFunc)();
    typedef void (*RunFunc)();
    typedef void (*HostMountedFunc)(bool mounted);
    typedef void (*HostMountedWTypeFunc)(bool mounted, HostDriverType host_type);
    typedef void (*WirelessConnectedFunc)(bool connected, uint8_t idx);

    static constexpr InitFunc init_func[BOARDS_COUNT] = {
        standard::initialize,       // PI_PICO
        standard::initialize,       // RP2040_ZERO
        standard::initialize,       // ADAFRUIT_FEATHER
        pico_w::initialize,         // PI_PICOW
        esp32_bp32_i2c::initialize, // ESP32_BLUEPAD32_I2C
        esp32_br_i2c::initialize,   // ESP32_BLUERETRO_I2C
        four_ch_i2c::initialize,    // EXTERNAL_4CH_I2C
        four_ch_i2c::initialize,    // INTERNAL_4CH_I2C
    };

    static constexpr RunFunc run_func[BOARDS_COUNT] = {
        standard::run,          // PI_PICO
        standard::run,          // RP2040_ZERO
        standard::run,          // ADAFRUIT_FEATHER
        pico_w::run,            // PI_PICOW
        esp32_bp32_i2c::run,    // ESP32_BLUEPAD32_I2C
        esp32_br_i2c::run,      // ESP32_BLUERETRO_I2C
        four_ch_i2c::run,       // EXTERNAL_4CH_I2C
        four_ch_i2c::run,       // INTERNAL_4CH_I2C
    };

    static constexpr HostMountedFunc host_mount_func[BOARDS_COUNT] = {
        standard::host_mounted,     // PI_PICO
        standard::host_mounted,     // RP2040_ZERO
        standard::host_mounted,     // ADAFRUIT_FEATHER
        nullptr,                    // PI_PICOW
        nullptr,                    // ESP32_BLUEPAD32_I2C
        nullptr,                    // ESP32_BLUERETRO_I2C
        four_ch_i2c::host_mounted,  // EXTERNAL_4CH_I2C
        four_ch_i2c::host_mounted,  // INTERNAL_4CH_I2C
    };

    static constexpr HostMountedWTypeFunc host_mount_w_type_func[BOARDS_COUNT] = {
        nullptr,                            // PI_PICO
        nullptr,                            // RP2040_ZERO
        nullptr,                            // ADAFRUIT_FEATHER
        nullptr,                            // PI_PICOW
        nullptr,                            // ESP32_BLUEPAD32_I2C
        nullptr,                            // ESP32_BLUERETRO_I2C
        four_ch_i2c::host_mounted_w_type,   // EXTERNAL_4CH_I2C
        four_ch_i2c::host_mounted_w_type,   // INTERNAL_4CH_I2C
    };

    static constexpr WirelessConnectedFunc wl_conn_func[BOARDS_COUNT] = {
        nullptr,                            // PI_PICO
        nullptr,                            // RP2040_ZERO
        nullptr,                            // ADAFRUIT_FEATHER
        nullptr,                            // PI_PICOW
        nullptr,                            // ESP32_BLUEPAD32_I2C
        nullptr,                            // ESP32_BLUERETRO_I2C
        four_ch_i2c::wireless_connected,    // EXTERNAL_4CH_I2C
        four_ch_i2c::wireless_connected,    // INTERNAL_4CH_I2C
    };

    void initialize() {
        if (init_func[OGXM_BOARD] != nullptr) {
            init_func[OGXM_BOARD]();
        }
    }

    void run() {
        if (run_func[OGXM_BOARD] != nullptr) {
            run_func[OGXM_BOARD]();
        }
    }

    void host_mounted(bool mounted, HostDriverType host_type) {
        if (host_mount_w_type_func[OGXM_BOARD] != nullptr) {
            host_mount_w_type_func[OGXM_BOARD](mounted, host_type);
        } else if (host_mount_func[OGXM_BOARD] != nullptr) {
            host_mount_func[OGXM_BOARD](mounted);
        }
    }

    void host_mounted(bool mounted) {
        if (host_mount_func[OGXM_BOARD] != nullptr) {
            host_mount_func[OGXM_BOARD](mounted);
        }
    }

    void wireless_connected(bool connected, uint8_t idx) {
        if (wl_conn_func[OGXM_BOARD] != nullptr) {
            wl_conn_func[OGXM_BOARD](connected, idx);
        }
    }

} // namespace OGXMini