#ifndef _UART_BRIDGE_H_
#define _UART_BRIDGE_H_

#ifdef __cplusplus
extern "C" {
#endif

int uart_bridge_run(void);
void uart_bridge_line_state_cb(uint8_t itf, bool dtr, bool rts);
void uart_bridge_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding);

#ifdef __cplusplus
}
#endif

#endif // _UART_BRIDGE_H_
