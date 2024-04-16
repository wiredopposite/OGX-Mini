#include "board_config.h"

#if (OGX_TYPE == WIRELESS) && (OGX_MCU == MCU_RP2040)

#ifndef UART_HELPERS_H_
#define UART_HELPERS_H_

bool uart_programming_mode();
void esp32_reset();

#endif // UART_HELPERS_H_

#endif // (OGX_TYPE == WIRELESS) && (OGX_MCU == MCU_RP2040)