#pragma once

#include <cstdint>

#include "USBHost/HostDriver/HostDriverTypes.h"

namespace four_ch_i2c {
    void initialize();
    void run();
    void host_mounted_w_type(bool mounted, HostDriverType host_type);
    void host_mounted(bool mounted);
    void wireless_connected(bool connected, uint8_t idx);
} // namespace ext_4ch_i2c