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

#include "USBHost/HIDParser/HIDReportDescriptorElements.h"
#include <cstring>

// https://docs.kernel.org/hid/hidreport-parsing.html
#define HID_FUNC_TYPE_MASK 0xFC
#define HID_TYPE_MASK      0x0C
#define HID_LENGTH_MASK    0x03

/* -------------------------------------------------------------------------- */

HIDElement::HIDElement() : type(HIDElementType::HID_UNKNOWN),
                           data_size(0)
{
    memset(data, 0x00, sizeof(data));
}

/* -------------------------------------------------------------------------- */

HIDElement::~HIDElement()
{
}

/* -------------------------------------------------------------------------- */

HIDElement::HIDElement(HIDElementType type, const uint8_t *data, uint8_t data_size)
{
    this->type = type;
    memset(this->data, 0x00, sizeof(this->data));
    memcpy(this->data, data, data_size);
    this->data_size = data_size;
}

/* -------------------------------------------------------------------------- */

uint32_t HIDElement::GetSize() const
{
    return data_size;
}

/* -------------------------------------------------------------------------- */

HIDElementType HIDElement::GetType() const
{
    return type;
}

/* -------------------------------------------------------------------------- */

uint32_t HIDElement::GetValueUint32() const
{
    return data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

/* -------------------------------------------------------------------------- */

int32_t HIDElement::GetValueInt32() const
{
    if (this->data_size == 1)
        return (int8_t)data[0];
    else if (this->data_size == 2)
        return (int16_t)(data[0] | ((uint16_t)data[1] << 8));
    else if (this->data_size == 4)
        return (int32_t)(data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24));
    
    return 0;
}

/* -------------------------------------------------------------------------- */

 HIDReportDescriptorElements::HIDReportDescriptorElements(const uint8_t *hid_report_data, uint16_t hid_report_data_len):
    hid_report_data(hid_report_data),
    hid_report_data_len(hid_report_data_len)
{
}

/* -------------------------------------------------------------------------- */

HIDReportDescriptorElements::~HIDReportDescriptorElements()
{
}

/* -------------------------------------------------------------------------- */

HIDReportDescriptorElements::Iterator HIDReportDescriptorElements::begin() const 
{
    return HIDReportDescriptorElements::Iterator(hid_report_data, hid_report_data_len);
}

/* -------------------------------------------------------------------------- */

HIDReportDescriptorElements::Iterator HIDReportDescriptorElements::end() const 
{
    return HIDReportDescriptorElements::Iterator(hid_report_data, hid_report_data_len, hid_report_data_len);
}

/* -------------------------------------------------------------------------- */

HIDReportDescriptorElements::Iterator::Iterator(const uint8_t* hid_report_data, uint16_t hid_report_data_len, uint16_t offset) : 
    hid_report_data(hid_report_data),
    hid_report_data_len(hid_report_data_len), 
    offset(offset) 
{
    if (offset < hid_report_data_len)
        parse_current_element();
}

/* -------------------------------------------------------------------------- */

HIDElement& HIDReportDescriptorElements::Iterator::operator*() 
{
    return current_element;
}

/* -------------------------------------------------------------------------- */

HIDElement* HIDReportDescriptorElements::Iterator::operator->()
{
    return &current_element;
}

/* -------------------------------------------------------------------------- */

HIDReportDescriptorElements::Iterator& HIDReportDescriptorElements::Iterator::operator++() 
{
    offset += 1 + current_element_length;

    if (offset < hid_report_data_len)
        parse_current_element();
    else
        offset = hid_report_data_len; // End condition
    
    return *this;
}

/* -------------------------------------------------------------------------- */

bool HIDReportDescriptorElements::Iterator::operator!=(const Iterator& other) const 
{
    return offset != other.offset;
}

/* -------------------------------------------------------------------------- */

void HIDReportDescriptorElements::Iterator::parse_current_element() 
{
    uint8_t type = hid_report_data[offset];
    uint8_t datalen = type & HID_LENGTH_MASK;
    if (datalen == 3)
        datalen = 4;

    current_element = HIDElement((HIDElementType)(type & HID_FUNC_TYPE_MASK), &hid_report_data[offset + 1], datalen);
    current_element_length = datalen;
}