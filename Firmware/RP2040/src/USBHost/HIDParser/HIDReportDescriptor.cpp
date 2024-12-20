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

#include "USBHost/HIDParser/HIDReportDescriptor.h"
#include "USBHost/HIDParser/HIDReportDescriptorElements.h"
#include "USBHost/HIDParser/HIDReportDescriptorUsages.h"
#include <iostream>
#include <vector>
#include <memory>
#include <stack>
#include <cassert>
#include <algorithm>

// https://github.com/pasztorpisti/hid-report-parser/blob/master/src/hid_report_parser.cpp
// https://docs.kernel.org/hid/hidintro.html

HIDInputOutput::HIDInputOutput(HIDIOType type, uint32_t size, uint32_t id) : type(type),
                                                                             sub_type(0),
                                                                             size(size),
                                                                             id(id),
                                                                             logical_min(0),
                                                                             logical_max(0),
                                                                             physical_min(0),
                                                                             physical_max(0),
                                                                             unit(0),
                                                                             unit_exponent(0)
{
}

HIDInputOutput::~HIDInputOutput()
{
}

HIDInputOutput::HIDInputOutput(const HIDUsage &usage, uint32_t idx) : type(HIDIOType::Unknown),
                                                                      sub_type(0),
                                                                      size(usage.property.size),
                                                                      id(0),
                                                                      logical_min(usage.property.logical_min),
                                                                      logical_max(usage.property.logical_max),
                                                                      physical_min(usage.property.physical_min),
                                                                      physical_max(usage.property.physical_max),
                                                                      unit(usage.property.unit),
                                                                      unit_exponent(usage.property.unit_exponent)
{
    if (usage.type == HIDUsageType::GenericDesktop)
    {
        if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::X)
            this->type = HIDIOType::X;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Y)
            this->type = HIDIOType::Y;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Z)
            this->type = HIDIOType::Z;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Rx)
            this->type = HIDIOType::Rx;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Ry)
            this->type = HIDIOType::Ry;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Rz)
            this->type = HIDIOType::Rz;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Slider)
            this->type = HIDIOType::Slider;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Dial)
            this->type = HIDIOType::Dial;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::HatSwitch)
            this->type = HIDIOType::HatSwitch;
        else if (usage.sub_type == (uint32_t)HIDUsageGenericDesktopSubType::Wheel)
            this->type = HIDIOType::Wheel;
    }
    else if (usage.type == HIDUsageType::Button)
        this->type = HIDIOType::Button;
    else if (usage.type == HIDUsageType::ReportId)
        this->type = HIDIOType::ReportId;
    else if (usage.type == HIDUsageType::Padding)
        this->type = HIDIOType::Padding;
    else if (usage.type == HIDUsageType::VendorDefined)
    {
        this->type = HIDIOType::VendorDefined;
        this->sub_type = usage.sub_type;
    }
    else
    {
        this->type = HIDIOType::Unknown;
        this->sub_type = usage.sub_type;
    }
    this->id = usage.usage_min + idx;
}

/* -------------------------------------------------------------------- */

HIDReportDescriptor::HIDReportDescriptor()
{
}

/* -------------------------------------------------------------------- */

HIDReportDescriptor::HIDReportDescriptor(const uint8_t *hid_report_data, uint16_t hid_report_data_len)
{
    parse(hid_report_data, hid_report_data_len);
}

/* -------------------------------------------------------------------- */

HIDReportDescriptor::~HIDReportDescriptor()
{
}

/* -------------------------------------------------------------------- */

void HIDReportDescriptor::parse(const uint8_t *hid_report_data, uint16_t hid_report_data_len)
{
    HIDReportDescriptorElements hid_report_elements = HIDReportDescriptorElements(hid_report_data, hid_report_data_len);
    std::vector<HIDReport> hid_report_usage = HIDReportDescriptorUsages::parse(hid_report_elements);
    for (auto report : hid_report_usage)
    {
        m_reports.push_back(HIDIOReport((HIDIOReportType)report.usages[0].sub_type));

        for (size_t k = 1; k < report.usages.size(); k++)
        {
            std::vector<HIDIOBlock> *ioblocks = NULL;
            HIDUsage &usage = report.usages[k];

            if (usage.io_type == HIDUsageIOType::Input)
                ioblocks = &m_reports.back().inputs;
            else if (usage.io_type == HIDUsageIOType::Output)
                ioblocks = &m_reports.back().outputs;
            else if (usage.io_type == HIDUsageIOType::Feature)
                ioblocks = &m_reports.back().features;
            else
                continue;

            for (uint32_t i = 0; i < usage.property.count; i++)
            {
                HIDInputOutput io(usage, i);
                
                //We need to create a new block everytime we meet a ReportId and if there is no block
                if (io.type == HIDIOType::ReportId || ioblocks->size() == 0)
                    ioblocks->push_back(HIDIOBlock());

                ioblocks->back().data.push_back(io);
            }
        }
    }

    assert(m_reports.size() > 0);
}
