/*
    MIT License

    Copyright (c) 2024 o0zz

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

#pragma once

#include <stdint.h>
#include <vector>

enum class HIDIOType 
{
    Unknown = 0x00,
    ReportId,
    VendorDefined,
    Padding,
    Button,
    X,
    Y,
    Z,
    Rx,
    Ry,
    Rz,
    Slider,
    Dial,
    HatSwitch,
    Wheel
};
class HIDUsage;

class HIDInputOutput
{
public:
    HIDInputOutput(HIDIOType type = HIDIOType::Unknown, uint32_t size=0, uint32_t id=0);
    ~HIDInputOutput();
    HIDInputOutput(const HIDUsage &usage, uint32_t idx);

    HIDIOType type; //Type (Button, X, Y, Hat switch, Padding, etc.)
    uint32_t sub_type; //Sub type (Usefull for vendor defined and non handled types)
    uint32_t size; //Size of the data in bits
    uint32_t id; //Index of the input in the report

    int32_t logical_min;
    int32_t logical_max;
    int32_t physical_min;
    int32_t physical_max;
    uint32_t unit;
    uint32_t unit_exponent;
};

/* -------------------------------------------------------------------------- */

//https://usb.org/sites/default/files/hut1_2.pdf p31
typedef enum class HIDIOReportType
{
    Unknown = 0x00,
    Pointer = 0x01,
    Mouse = 0x02,
    Joystick = 0x04,
    GamePad = 0x05,
    Keyboard = 0x06,
    Keypad = 0x07,
    MultiAxis = 0x08,
    Tablet = 0x09,

    MAX = 0x2F
} HIDIOReportType;

class HIDIOBlock
{
public:
    HIDIOBlock() {}
    ~HIDIOBlock() {}

    std::vector<HIDInputOutput> data;
};

class HIDIOReport
{
public:
    HIDIOReport(HIDIOReportType report_type = HIDIOReportType::Unknown) :
        report_type(report_type)
    {}

    HIDIOReportType report_type;
    std::vector<HIDIOBlock> inputs;
    std::vector<HIDIOBlock> outputs;
    std::vector<HIDIOBlock> features;
};

/* -------------------------------------------------------------------------- */

class HIDReportDescriptor
{
public:
    HIDReportDescriptor();
    HIDReportDescriptor(const uint8_t *hid_report_data, uint16_t hid_report_data_size);
    ~HIDReportDescriptor();

    std::vector<HIDIOReport> GetReports() const { return m_reports; }
    
private:
    void parse(const uint8_t *hid_report_data, uint16_t hid_report_data_len);

    std::vector<HIDIOReport> m_reports;
};
