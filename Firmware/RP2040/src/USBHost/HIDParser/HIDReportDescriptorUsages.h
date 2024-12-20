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
#include "USBHost/HIDParser/HIDReportDescriptorElements.h"
#include <vector>

enum class HIDUsageIOType 
{
    None = 0x00,
    Input,
    Output,
    Feature
};

enum class HIDUsageType 
{
    Unknown = 0x00,
    ReportId,
    Padding,
    Button,
    GenericDesktop,
    VendorDefined
};

enum class HIDUsageGenericDesktopSubType 
{
    Pointer = 0x01,
    Mouse = 0x02,
    Joystick = 0x04,
    GamePad = 0x05,
    Keyboard = 0x06,
    Keypad = 0x07,
    MultiAxisController = 0x08,

    ReportTypeEnd = 0x1F,
    
    X           = 0x30,
    Y           = 0x31,
    Z           = 0x32,
    Rx          = 0x33,
    Ry          = 0x34,
    Rz          = 0x35,
    Slider      = 0x36,
    Dial        = 0x37,
    Wheel       = 0x38,
    HatSwitch   = 0x39,
};

class HIDProperty
{
public:
    HIDProperty(uint32_t size=0, uint32_t count=0);
    virtual ~HIDProperty();

    virtual bool is_valid();

    int32_t logical_min;
    uint32_t logical_min_unsigned;

    int32_t logical_max;
    uint32_t logical_max_unsigned;

    int32_t physical_min;
    uint32_t physical_min_unsigned;

    int32_t physical_max;
    uint32_t physical_max_unsigned;

    uint32_t unit;
    uint32_t unit_exponent;
    uint32_t size; //Size of the data in bits
    uint32_t count; //Number of data items
};

class HIDUsage
{
public:
    /// @brief 
    /// @param type 
    /// @param sub_type will depend on the type, for example, if type is GenericDesktop, sub_type will be HIDUsageGenericDesktopSubType etc.
    /// @param property 
    HIDUsage(HIDUsageType type, uint32_t sub_type = 0, HIDUsageIOType io_type = HIDUsageIOType::None, HIDProperty property = HIDProperty());
    ~HIDUsage();

    HIDUsageType type; //Input type (Button, X, Y, Hat switch, Padding, etc.)
    uint32_t sub_type; //Sub type (Button number, etc.)
    uint32_t usage_min;
    uint32_t usage_max;
    HIDUsageIOType io_type;
    HIDProperty property;
};

class HIDReport
{
public:
    std::vector<HIDUsage> usages;
};

class HIDReportDescriptorUsages
{
public:
    static std::vector<HIDReport> parse(const HIDReportDescriptorElements &elements);
};