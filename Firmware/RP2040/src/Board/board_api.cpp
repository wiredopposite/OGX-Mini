/* This is too messy I'll clean it up at some point */

#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <hardware/gpio.h>
#include <hardware/watchdog.h>

#include "Board/board_api.h"
#include "board_config.h"

#if defined(CONFIG_EN_BLUETOOTH)
#include <pico/cyw43_arch.h>
#endif // defined(CONFIG_EN_BLUETOOTH)

#if defined(CONFIG_EN_RGB)
#include "WS2812.pio.h"
#include "Board/Pico_WS2812/WS2812.hpp"
#endif // defined(CONFIG_EN_RGB)

namespace board_api {

bool inited_ = false;
mutex_t gpio_mutex_;

#if defined(CONFIG_EN_RGB)
WS2812 ws2812_ = WS2812(RGB_PXL_PIN, 1, pio1, 0, WS2812::FORMAT_GRB);
#endif // defined(CONFIG_EN_RGB)

void init_gpio()
{   
    if (inited_)
    {
        return;
    }
    if (!mutex_is_initialized(&gpio_mutex_))
    {
        mutex_init(&gpio_mutex_); 
    }
    mutex_enter_blocking(&gpio_mutex_);

#ifdef VCC_EN_PIN
    gpio_init(VCC_EN_PIN);
    gpio_set_dir(VCC_EN_PIN, GPIO_OUT);
    gpio_put(VCC_EN_PIN, 1);
#endif 

#if defined(CONFIG_EN_RGB)
    #ifdef RGB_PWR_PIN
        gpio_init(RGB_PWR_PIN);
        gpio_set_dir(RGB_PWR_PIN, GPIO_OUT);
        gpio_put(RGB_PWR_PIN, 1);
    #endif

#elif defined(CONFIG_EN_BLUETOOTH)
    //

#elif defined(LED_INDICATOR_PIN)    
    gpio_init(LED_INDICATOR_PIN);
    gpio_set_dir(LED_INDICATOR_PIN, GPIO_OUT);
    gpio_put(LED_INDICATOR_PIN, 0);

#endif

#if defined(CONFIG_EN_ESP32)
    gpio_init(ESP_PROG_PIN);
    gpio_set_dir(ESP_PROG_PIN, GPIO_OUT);
    gpio_put(ESP_PROG_PIN, 1);

    gpio_init(ESP_RST_PIN);
    gpio_set_dir(ESP_RST_PIN, GPIO_OUT);
    gpio_put(ESP_RST_PIN, 1);

#endif //defined(CONFIG_EN_ESP32)

#if defined(CONFIG_EN_UART_BRIDGE)
    gpio_init(MODE_SEL_PIN);
    gpio_set_dir(MODE_SEL_PIN, GPIO_IN);
    gpio_pull_up(MODE_SEL_PIN);

#endif // defined(CONFIG_EN_UART_BRIDGE)

    inited_ = true;
    mutex_exit(&gpio_mutex_);    
}

void set_led(bool state)
{
    if (!inited_)
    {
        return;
    }
    mutex_enter_blocking(&gpio_mutex_);

#if defined(CONFIG_EN_RGB)
    ws2812_.setPixelColor(0, state ? WS2812::RGB(0x00, 0xFF, 0x00) : WS2812::RGB(0xFF, 0, 0));
    ws2812_.show();

#elif defined(CONFIG_EN_BLUETOOTH)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state ? 1 : 0);

#elif defined(LED_INDICATOR_PIN)  
    gpio_put(LED_INDICATOR_PIN, state ? 1 : 0);

#endif //defined(CONFIG_EN_RGB)

    mutex_exit(&gpio_mutex_);
}

#if defined(CONFIG_EN_UART_BRIDGE)
bool uart_bridge_mode()
{
    if (!inited_)
    {
        return false;
    }

    bool mode = false;
    mutex_enter_blocking(&gpio_mutex_);

    gpio_pull_up(MODE_SEL_PIN);
    if (gpio_get(MODE_SEL_PIN) == 0) 
    {
        mode = true;
    } 

    mutex_exit(&gpio_mutex_);
    return mode;
}  
#endif // defined(CONFIG_EN_UART_BRIDGE)

#if defined(CONFIG_EN_ESP32)
void reset_esp32_unsafe() 
{
    gpio_put(ESP_RST_PIN, 0);
    sleep_ms(500);
    gpio_put(ESP_RST_PIN, 1);
    sleep_ms(250);
}

void reset_esp32() 
{
    if (!inited_)
    {
        return;
    }
    mutex_enter_blocking(&gpio_mutex_);
    reset_esp32_unsafe();
    mutex_exit(&gpio_mutex_);
}

void enter_esp32_prog_mode() 
{
    if (!inited_)
    {
        return;
    }
    mutex_enter_blocking(&gpio_mutex_);

    gpio_put(ESP_PROG_PIN, 1);
    sleep_ms(250);
    gpio_put(ESP_PROG_PIN, 0);
	sleep_ms(250);

    reset_esp32_unsafe();

	gpio_put(ESP_PROG_PIN, 1);

    mutex_exit(&gpio_mutex_);
}
#endif // defined(CONFIG_EN_ESP32)

void reboot()
{
    #define AIRCR_REG (*((volatile uint32_t *)(0xE000ED0C)))
    #define AIRCR_SYSRESETREQ (1 << 2)
    #define AIRCR_VECTKEY (0x5FA << 16)

    AIRCR_REG = AIRCR_VECTKEY | AIRCR_SYSRESETREQ;
    while(1);

    // watchdog_enable(500, true); // 10-second timeout
    // while (true)
    // {
    //     // Your main loop logic
    //     watchdog_update(); // Reset the watchdog timer
    // }
}

} // namespace board_api