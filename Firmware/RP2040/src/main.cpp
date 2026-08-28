#include <cstdint>
#include <cstdio>

#include "OGXMini/OGXMini.h"
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#ifndef PICO_DEFAULT_UART_BAUD_RATE
#define PICO_DEFAULT_UART_BAUD_RATE 115200
#endif

int main() {
    stdio_init_all();

#if defined(PICO_DEFAULT_UART_TX_PIN) && defined(PICO_DEFAULT_UART_RX_PIN)
    // Use build-defined UART pins (e.g. Debug: GP4/GP5 UART1 for Pico W/2W)
    uart_inst_t* uart = (PICO_DEFAULT_UART == 0) ? uart0 : uart1;
    uart_init(uart, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function(PICO_DEFAULT_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PICO_DEFAULT_UART_RX_PIN, GPIO_FUNC_UART);
#else
    uart_init(uart0, PICO_DEFAULT_UART_BAUD_RATE);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
#endif
    printf("Debug ready\n");

    flash_safe_execute_core_init();

    OGXMini::initialize();
    OGXMini::run();
    return 0;
}