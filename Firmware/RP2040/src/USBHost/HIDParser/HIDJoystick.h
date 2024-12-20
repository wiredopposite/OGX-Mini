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
#include <memory>
#include <vector>

#define MAX_BUTTONS 32

enum class HIDJoystickHatSwitch
{
    UP = 0,
    UP_RIGHT = 1,
    RIGHT = 2,
    DOWN_RIGHT = 3,
    DOWN = 4,
    DOWN_LEFT = 5,
    LEFT = 6,
    UP_LEFT = 7,
    NEUTRAL = 8
};

#define JOYSTICK_SUPPORT_X         0x0001
#define JOYSTICK_SUPPORT_Y         0x0002
#define JOYSTICK_SUPPORT_Z         0x0004
#define JOYSTICK_SUPPORT_Rx        0x0008
#define JOYSTICK_SUPPORT_Ry        0x0010
#define JOYSTICK_SUPPORT_Rz        0x0020
#define JOYSTICK_SUPPORT_Slider    0x0040
#define JOYSTICK_SUPPORT_Dial      0x0080
#define JOYSTICK_SUPPORT_HatSwitch 0x0100

class HIDJoystickData
{
public:
    HIDJoystickData();
    ~HIDJoystickData();

    uint8_t index;

    uint16_t support;

    int16_t X;  //-32768 to 32767
    int16_t Y;  //-32768 to 32767
    int16_t Z;  //-32768 to 32767
    int16_t Rx; //-32768 to 32767
    int16_t Ry; //-32768 to 32767
    int16_t Rz; //-32768 to 32767
    int16_t Slider; //-32768 to 32767
    int16_t Dial; //-32768 to 32767

    HIDJoystickHatSwitch hat_switch;

    uint8_t button_count;
    uint8_t buttons[MAX_BUTTONS];
};

class HIDJoystick
{
public:
    HIDJoystick(const std::shared_ptr<HIDReportDescriptor> &descriptor);
    ~HIDJoystick();

    bool isValid();
    uint8_t getCount();

    bool parseData(uint8_t *data, uint16_t datalen, HIDJoystickData *joystick_data);

private:
    std::vector<HIDIOReport> m_reports;
};