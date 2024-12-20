/*  

TinyUSB XInput host driver based on https://github.com/Ryzee119/tusb_xinput 

MIT License

Copyright (c) 2020 Ryan Wendland

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef _TUH_XINPUT_H_
#define _TUH_XINPUT_H_

#include <cstdint>
#include <array>

#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"

namespace tuh_xinput
{
    enum class DevType { UNKNOWN, XBOX360, XBOX360W, XBOXOG, XBOXONE };
    enum class ItfType { UNKNOWN, XID, CHATPAD };
    enum class ChatpadStage
    {
        INIT_1 = 0,
        INIT_2,
        INIT_3,
        INIT_4,
        INIT_5,
        INIT_6,
        KEEPALIVE_1,
        KEEPALIVE_2,
        LED_REQUEST
    };

    static constexpr uint8_t ENDPOINT_SIZE = 64;
    static constexpr uint32_t KEEPALIVE_MS = 1000;

    struct Interface
    {
        bool connected{false};

        DevType dev_type{DevType::UNKNOWN};
        ItfType itf_type{ItfType::UNKNOWN};

        bool chatpad_inited{false};
        ChatpadStage chatpad_stage{ChatpadStage::INIT_1};

        uint8_t dev_addr{0xFF};
        uint8_t itf_num{0xFF};

        uint8_t ep_in{0xFF};
        uint8_t ep_out{0xFF};

        uint16_t ep_in_size{0xFF};
        uint16_t ep_out_size{0xFF};

        std::array<uint8_t, ENDPOINT_SIZE> ep_in_buffer{0};
        std::array<uint8_t, ENDPOINT_SIZE> ep_out_buffer{0};
    };

    // API

    const usbh_class_driver_t* class_driver();

    bool send_report(uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len);
    bool receive_report(uint8_t address, uint8_t instance);
    bool set_rumble(uint8_t address, uint8_t instance, uint8_t rumble_l, uint8_t rumble_r, bool block);
    bool set_led(uint8_t address, uint8_t instance, uint8_t led_number, bool block);

    //Wireless only atm
    void xbox360_chatpad_init(uint8_t address, uint8_t instance); 
    bool xbox360_chatpad_keepalive(uint8_t address, uint8_t instance);

    // User implemented callbacks

    TU_ATTR_WEAK void mount_cb(uint8_t dev_addr, uint8_t instance, const Interface *interface);
    TU_ATTR_WEAK void unmount_cb(uint8_t dev_addr, uint8_t instance, const Interface *interface);
    TU_ATTR_WEAK void report_sent_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
    TU_ATTR_WEAK void xbox360w_connect_cb(uint8_t dev_addr, uint8_t instance);
    TU_ATTR_WEAK void xbox360w_disconnect_cb(uint8_t dev_addr, uint8_t instance);
    void report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
    
}; // namespace tuh_xinput

#endif // _TUH_XINPUT_H_