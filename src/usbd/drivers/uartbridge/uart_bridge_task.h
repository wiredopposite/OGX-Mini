#ifndef _UART_BRIDGE_TASK_H_
#define _UART_BRIDGE_TASK_H_

#include "usbd/drivers/uartbridge/helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int itf;

void usbd_serial_init(void);
void update_uart_cfg(uint8_t itf);
void uart_write_bytes(uint8_t itf);
void init_uart_data(uint8_t itf);
void core1_entry(void);

#ifdef __cplusplus
}
#endif

#endif // _UART_BRIDGE_TASK_H_