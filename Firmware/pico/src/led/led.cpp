#include "board_config.h"
#include "led/led.h"

#if NEOPIXEL_ENABLED
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include "neopixel/WS2812.hpp"

#ifndef NEOPIXEL_PIO
#define NEOPIXEL_PIO pio0
#endif

#ifndef NEOPIXEL_PIO_SM
#define NEOPIXEL_PIO_SM 4
#endif

static WS2812& get_ws2812() {
    static WS2812 ws2812 = 
        WS2812(
            NEOPIXEL_DATA_PIN, 
            1, 
            NEOPIXEL_PIO, 
            NEOPIXEL_PIO_SM, 
            (WS2812::DataFormat)NEOPIXEL_FORMAT
        );
    return ws2812;
}

void led_init(void) {
#ifdef NEOPIXEL_POWER_PIN
    gpio_init(NEOPIXEL_POWER_PIN);
    gpio_set_dir(NEOPIXEL_POWER_PIN, GPIO_OUT);
    gpio_put(NEOPIXEL_POWER_PIN, 1); // Power on the NeoPixel
#endif
}

void led_rgb_set_color(uint8_t r, uint8_t g, uint8_t b) {
    get_ws2812().setPixelColor(0, WS2812::RGB(r, g, b));
    get_ws2812().show();
}

void led_set_on(bool on) {
    get_ws2812().fill(on ? WS2812::RGB(0, 0xFF, 0) : WS2812::RGB(0xFF, 0, 0));
    get_ws2812().show();
}

#elif (BLUETOOTH_ENABLED && (BLUETOOTH_HARDWARE == BLUETOOTH_HARDWARE_PICOW))
#include <pico/cyw43_arch.h>

void led_init(void) {}

void led_rgb_set_color(uint8_t r, uint8_t g, uint8_t b) {
    (void)r; (void)g; (void)b;
}

void led_set_on(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
}

#elif LED_ENABLED
#include <hardware/gpio.h>

void led_init(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
}

void led_rgb_set_color(uint8_t r, uint8_t g, uint8_t b) {
    (void)r; (void)g; (void)b;
}

void led_set_on(bool on) {
    gpio_put(LED_PIN, on);
}

#else // No LED support

void led_init(void) {}

void led_rgb_set_color(uint8_t r, uint8_t g, uint8_t b) {
    (void)r; (void)g; (void)b;
}

void led_set_on(bool on) {
    (void)on;
}

#endif