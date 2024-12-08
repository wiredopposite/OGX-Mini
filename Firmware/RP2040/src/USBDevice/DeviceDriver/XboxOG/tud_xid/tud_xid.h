/*  

TinyUSB XID Device driver based on https://github.com/Ryzee119/ogx360_t4

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

#ifndef _TUD_XID_H_
#define _TUD_XID_H_

#include <cstdint>
#include <array>

#include "tusb.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h"

namespace tud_xid
{
    enum class Type { GAMEPAD, STEELBATTALION, XREMOTE };

    void initialize(tud_xid::Type xid_type);

    const usbd_class_driver_t* class_driver();

    uint8_t get_index_by_type(uint8_t type_index, tud_xid::Type xid_type);
    bool receive_report(uint8_t idx, uint8_t* buffer, uint16_t len);
    bool send_report(uint8_t idx, const uint8_t* buffer, uint16_t len);
    bool send_report_ready(uint8_t idx);
    bool xremote_rom_available();

} // namespace TUDXID

#endif // _TUD_XID_H_