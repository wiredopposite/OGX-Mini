/*  
    Copyright ForsakenNGS 2021
    https://github.com/ForsakenNGS/Pico_WS2812 
*/

#include "Board/Pico_WS2812/WS2812.hpp"
#include "WS2812.pio.h"

WS2812::WS2812(uint pin, uint length, PIO pio, uint sm)  {
    initialize(pin, length, pio, sm, NONE, GREEN, RED, BLUE);
}

WS2812::WS2812(uint pin, uint length, PIO pio, uint sm, DataFormat format) {
    switch (format) {
        case FORMAT_RGB:
            initialize(pin, length, pio, sm, NONE, RED, GREEN, BLUE);
            break;
        case FORMAT_GRB:
            initialize(pin, length, pio, sm, NONE, GREEN, RED, BLUE);
            break;
        case FORMAT_WRGB:
            initialize(pin, length, pio, sm, WHITE, RED, GREEN, BLUE);
            break;
    }
}

WS2812::WS2812(uint pin, uint length, PIO pio, uint sm, DataByte b1, DataByte b2, DataByte b3) {
    initialize(pin, length, pio, sm, b1, b1, b2, b3);
}

WS2812::WS2812(uint pin, uint length, PIO pio, uint sm, DataByte b1, DataByte b2, DataByte b3, DataByte b4) {
    initialize(pin, length, pio, sm, b1, b2, b3, b4);
}

WS2812::~WS2812() {
    
}

void WS2812::initialize(uint pin, uint length, PIO pio, uint sm, DataByte b1, DataByte b2, DataByte b3, DataByte b4) {
    this->pin = pin;
    this->length = length;
    this->pio = pio;
    this->sm = sm;
    this->data = new uint32_t[length];
    this->bytes[0] = b1;
    this->bytes[1] = b2;
    this->bytes[2] = b3;
    this->bytes[3] = b4;
    uint offset = pio_add_program(pio, &ws2812_program);
    uint bits = (b1 == NONE ? 24 : 32);

    ws2812_program_init(pio, sm, offset, pin, 800000, bits);
}

uint32_t WS2812::convertData(uint32_t rgbw) {
    uint32_t result = 0;
    for (uint b = 0; b < 4; b++) {
        switch (bytes[b]) {
            case RED:
                result |= (rgbw & 0xFF);
                break;
            case GREEN:
                result |= (rgbw & 0xFF00) >> 8;
                break;
            case BLUE:
                result |= (rgbw & 0xFF0000) >> 16;
                break;
            case WHITE:
                result |= (rgbw & 0xFF000000) >> 24;
                break;
        }
        result <<= 8;
    }
    return result;
}

void WS2812::setPixelColor(uint index, uint32_t color) {
    if (index < length) {
        data[index] = convertData(color);
    }
}

void WS2812::setPixelColor(uint index, uint8_t red, uint8_t green, uint8_t blue) {
    setPixelColor(index, RGB(red, green, blue));
}

void WS2812::setPixelColor(uint index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    setPixelColor(index, RGBW(red, green, blue, white));
}

void WS2812::fill(uint32_t color) {
    fill(color, 0, length);
}

void WS2812::fill(uint32_t color, uint first) {
    fill(color, first, length-first);
}

void WS2812::fill(uint32_t color, uint first, uint count) {
    uint last = (first + count);
    if (last > length) {
        last = length;
    }
    color = convertData(color);
    for (uint i = first; i < last; i++) {
        data[i] = color;
    }
}

void WS2812::show() {
    for (uint i = 0; i < length; i++) {
        pio_sm_put_blocking(pio, sm, data[i]);
    }
}