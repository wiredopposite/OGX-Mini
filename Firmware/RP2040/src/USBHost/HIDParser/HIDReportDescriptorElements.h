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
#include <memory>
#include <vector>
#include <stdint.h>

/* -------------------------------------------------------------------------- */

typedef enum class HIDElementType
{
    HID_UNKNOWN = 0x00,
    HID_INPUT = 0x80,
    HID_OUTPUT = 0x90,
    HID_FEATURE = 0xb0,
    HID_COLLECTION = 0xa0,
    HID_END_COLLECTION = 0xc0,
    HID_USAGE_PAGE = 0x04,
    HID_LOGICAL_MINIMUM = 0x14,
    HID_LOGICAL_MAXIMUM = 0x24,
    HID_PHYSICAL_MINIMUM = 0x34,
    HID_PHYSICAL_MAXIMUM = 0x44,
    HID_UNIT_EXPONENT = 0x54,
    HID_UNIT = 0x64,
    HID_REPORT_SIZE = 0x74,
    HID_REPORT_ID = 0x84,
    HID_REPORT_COUNT = 0x94,
    HID_PUSH = 0xa4,
    HID_POP = 0xb4,
    HID_USAGE = 0x08,
    HID_USAGE_MINIMUM = 0x18,
    HID_USAGE_MAXIMUM = 0x28,
    HID_DESIGNATOR_INDEX = 0x38,
    HID_DESIGNATOR_MINIMUM = 0x48,
    HID_DESIGNATOR_MAXIMUM = 0x58,
    HID_STRING_INDEX = 0x78,
    HID_STRING_MINIMUM = 0x88,
    HID_STRING_MAXIMUM = 0x98,
    HID_DELIMITER = 0xa8
} HIDElementType;

/* -------------------------------------------------------------------------- */

class HIDElement
{
public:
    HIDElement();
    HIDElement(HIDElementType type, const uint8_t *data, uint8_t data_size);
    ~HIDElement();

    uint32_t GetSize() const;
    HIDElementType GetType() const;
    uint32_t GetValueUint32() const;
    int32_t GetValueInt32() const;

private:
    HIDElementType type;
    uint8_t data[4];
    uint8_t data_size;
};

/* -------------------------------------------------------------------------- */

class HIDReportDescriptorElements
{
public:
    HIDReportDescriptorElements(const uint8_t *hid_report_data, uint16_t hid_report_data_len);
    ~HIDReportDescriptorElements();

    class Iterator {
        public:
            Iterator(const uint8_t* hid_report_data, uint16_t hid_report_data_len, uint16_t offset = 0);
            HIDElement& operator*();
            HIDElement* operator->();
            Iterator& operator++();
            
            bool operator!=(const Iterator& other) const;

        private:
            void parse_current_element() ;

            const uint8_t* hid_report_data;
            uint16_t hid_report_data_len;
            uint16_t offset;
            HIDElement current_element;
            uint8_t current_element_length;
    };

    Iterator begin() const;
    Iterator end() const;

private:
    const uint8_t *hid_report_data;
    uint16_t hid_report_data_len;
};
