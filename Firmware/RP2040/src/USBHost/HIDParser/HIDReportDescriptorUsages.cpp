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

#include "USBHost/HIDParser/HIDReportDescriptorUsages.h"
#include <iostream>
#include <vector>
#include <memory>
#include <stack>
#include <cassert>
#include <algorithm>

//---------------USAGE_PAGE-----------------
#define USAGE_PAGE_GenericDesktop 0x01
#define USAGE_PAGE_Simulation     0x02
#define USAGE_PAGE_VR             0x03
#define USAGE_PAGE_Sport          0x04
#define USAGE_PAGE_Game           0x05
#define USAGE_PAGE_GenericDevice  0x06
#define USAGE_PAGE_Keyboard       0x07
#define USAGE_PAGE_LEDs           0x08
#define USAGE_PAGE_Button         0x09
#define USAGE_PAGE_Ordinal        0x0A
#define USAGE_PAGE_Telephony      0x0B
#define USAGE_PAGE_Consumer       0x0C
#define USAGE_PAGE_VendorDefined  0xFF00

//---------------USAGE-----------------
#define USAGE_X          0x30
#define USAGE_Y          0x31
#define USAGE_Z          0x32
#define USAGE_Rx         0x33
#define USAGE_Ry         0x34
#define USAGE_Rz         0x35
#define USAGE_Slider     0x36
#define USAGE_Dial       0x37
#define USAGE_Wheel      0x38
#define USAGE_Hat_switch 0x39

//---------------INPUT-----------------
#define INPUT_Const 0x01
#define INPUT_Var   0x02
#define INPUT_Rel   0x04
#define INPUT_Wrap  0x08
#define INPUT_NLin  0x10
#define INPUT_NPrf  0x20
#define INPUT_Null  0x40
#define INPUT_Vol   0x80

//---------------COLLECTION-----------------
#define HID_COLLECTION_PHYSICAL       0x00
#define HID_COLLECTION_APPLICATION    0x01
#define HID_COLLECTION_LOGICAL        0x02
#define HID_COLLECTION_REPORT         0x03
#define HID_COLLECTION_NAMED_ARRAY    0x04
#define HID_COLLECTION_USAGE_SWITCH   0x05
#define HID_COLLECTION_USAGE_MODIFIER 0x06

/* -------------------------------------------------------------------------- */

HIDUsage::HIDUsage(HIDUsageType type, uint32_t sub_type, HIDUsageIOType io_type, HIDProperty property) : type(type),
                                                                                                         sub_type(sub_type),
                                                                                                         usage_min(0),
                                                                                                         usage_max(0),
                                                                                                         io_type(io_type),
                                                                                                         property(property)
{
    if (sub_type != 0)
    {
        usage_min = sub_type;
        usage_max = sub_type;
    }
}

/* -------------------------------------------------------------------------- */

HIDUsage::~HIDUsage()
{
}

/* -------------------------------------------------------------------------- */

HIDProperty::HIDProperty(uint32_t size, uint32_t count) : logical_min(0),
                                                          logical_max(0),
                                                          physical_min(0),
                                                          physical_max(0),
                                                          unit(0),
                                                          unit_exponent(0),
                                                          size(size),
                                                          count(count)
{
}

/* -------------------------------------------------------------------------- */

HIDProperty::~HIDProperty()
{
}

/* -------------------------------------------------------------------------- */

bool HIDProperty::is_valid()
{
    return size != 0 && count != 0;
}

/* -------------------------------------------------------------------------- */

HIDUsageType convert_usage_page(uint32_t usage_page)
{
    switch (usage_page)
    {
        case USAGE_PAGE_Button:
            return HIDUsageType::Button;
        case USAGE_PAGE_GenericDesktop:
            return HIDUsageType::GenericDesktop;
        case USAGE_PAGE_VendorDefined:
            return HIDUsageType::VendorDefined;
        default:
            return HIDUsageType::Unknown;
    }
}

/* -------------------------------------------------------------------------- */

std::vector<HIDReport> HIDReportDescriptorUsages::parse(const HIDReportDescriptorElements &elements)
{
    HIDProperty current_property;
    std::vector<HIDUsage> current_usages;
    HIDUsageType current_usage_page_type = HIDUsageType::Unknown;
    std::vector<HIDReport> report;
    uint8_t current_report_id = 0;

    for (const HIDElement &element : elements)
    {
        switch (element.GetType())
        {
            case HIDElementType::HID_USAGE_PAGE:
                current_usage_page_type = convert_usage_page(element.GetValueUint32());
                break;

            case HIDElementType::HID_USAGE:
                current_usages.push_back(HIDUsage(current_usage_page_type, element.GetValueUint32()));
                break;

            case HIDElementType::HID_USAGE_MAXIMUM:
            case HIDElementType::HID_USAGE_MINIMUM:
            {
                if (current_usages.size() == 0)
                    current_usages.push_back(HIDUsage(current_usage_page_type));

                for (HIDUsage &usage : current_usages)
                {
                    if (element.GetType() == HIDElementType::HID_USAGE_MINIMUM)
                        usage.usage_min = element.GetValueUint32();
                    else if (element.GetType() == HIDElementType::HID_USAGE_MAXIMUM)
                        usage.usage_max = element.GetValueUint32();
                }
                break;
            }

            case HIDElementType::HID_REPORT_ID:
                current_report_id = element.GetValueUint32();
                break;

            case HIDElementType::HID_LOGICAL_MINIMUM:
                current_property.logical_min = element.GetValueInt32();
                current_property.logical_min_unsigned = element.GetValueUint32();
                break;

            case HIDElementType::HID_LOGICAL_MAXIMUM:
                current_property.logical_max = element.GetValueInt32();
                current_property.logical_max_unsigned = element.GetValueUint32();
                break;

            case HIDElementType::HID_PHYSICAL_MINIMUM:
                current_property.physical_min = element.GetValueInt32();
                current_property.physical_min_unsigned = element.GetValueUint32();
                break;

            case HIDElementType::HID_PHYSICAL_MAXIMUM:
                current_property.physical_max = element.GetValueInt32();
                current_property.physical_max_unsigned = element.GetValueUint32();
                break;

            case HIDElementType::HID_UNIT_EXPONENT:
                current_property.unit_exponent = element.GetValueUint32();
                break;

            case HIDElementType::HID_UNIT:
                current_property.unit = element.GetValueUint32();
                break;

            case HIDElementType::HID_REPORT_SIZE:
                current_property.size = element.GetValueUint32();
                break;

            case HIDElementType::HID_REPORT_COUNT:
                current_property.count = element.GetValueUint32();
                break;

            case HIDElementType::HID_INPUT:
            case HIDElementType::HID_OUTPUT:
            case HIDElementType::HID_FEATURE:
            {
                if (current_usages.size() == 0)
                    current_usages.push_back(HIDUsage(HIDUsageType::Padding));

                HIDUsageIOType io_type = HIDUsageIOType::None;
                if (element.GetType() == HIDElementType::HID_INPUT)
                    io_type = HIDUsageIOType::Input;
                else if (element.GetType() == HIDElementType::HID_OUTPUT)
                    io_type = HIDUsageIOType::Output;
                else if (element.GetType() == HIDElementType::HID_FEATURE)
                    io_type = HIDUsageIOType::Feature;

                for (HIDUsage &usage : current_usages)
                {
                        //Fix bug on few controllers, that provide incorrect "Unsigned" values
                    if (current_property.logical_max < current_property.logical_min)
                        current_property.logical_max = (int32_t)current_property.logical_max_unsigned;

                    if (current_property.physical_max < current_property.physical_min)
                        current_property.physical_max = (int32_t)current_property.physical_max_unsigned;

                    usage.io_type = io_type;
                    usage.property = current_property;
                    usage.property.count = (current_property.count / (uint32_t)current_usages.size());
                }

                if (current_report_id != 0)
                {
                    current_usages.insert(current_usages.begin(), HIDUsage(HIDUsageType::ReportId, current_report_id, io_type, HIDProperty(8, 1)));
                    current_report_id = 0;
                }

                report.back().usages.insert(report.back().usages.end(), current_usages.begin(), current_usages.end());
                current_usages.clear();
                break;
            }

                // For now collections are ignored
            case HIDElementType::HID_COLLECTION:
            {
                if (element.GetValueUint32() == HID_COLLECTION_APPLICATION)
                    report.push_back(HIDReport());

                report.back().usages.insert(report.back().usages.end(), current_usages.begin(), current_usages.end());
                current_usages.clear();
                break;
            }

            case HIDElementType::HID_END_COLLECTION:
            {
                break;
            }

            case HIDElementType::HID_UNKNOWN:
            case HIDElementType::HID_PUSH:
            case HIDElementType::HID_POP:
            case HIDElementType::HID_DELIMITER:
            case HIDElementType::HID_DESIGNATOR_INDEX:
            case HIDElementType::HID_DESIGNATOR_MINIMUM:
            case HIDElementType::HID_DESIGNATOR_MAXIMUM:
            case HIDElementType::HID_STRING_INDEX:
            case HIDElementType::HID_STRING_MINIMUM:
            case HIDElementType::HID_STRING_MAXIMUM:
                break;
        }
    }

    return report;
}
