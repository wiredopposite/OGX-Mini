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

#include "USBHost/HIDParser/HIDUtils.h"

uint32_t HIDUtils::readBitsLE(uint8_t *buffer, uint32_t bitOffset, uint32_t bitLength) {
    // Calculate the starting byte index and bit index within that byte
    uint32_t byteIndex = bitOffset / 8;
    uint32_t bitIndex = bitOffset % 8;  // Little endian, LSB is at index 0

    uint32_t result = 0;

    for (uint32_t i = 0; i < bitLength; ++i) {
        // Check if we need to move to the next byte
        if (bitIndex > 7) {
            ++byteIndex;
            bitIndex = 0;
        }

        // Get the bit at the current position and add it to the result
        uint8_t bit = (buffer[byteIndex] >> bitIndex) & 0x01;
        result |= (bit << i);

        // Move to the next bit
        ++bitIndex;
    }

    return result;
}