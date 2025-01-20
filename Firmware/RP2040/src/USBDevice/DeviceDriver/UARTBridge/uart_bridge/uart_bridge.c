// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2021 Álvaro Fernández Rojas <noltari@gmail.com>
 *
 * This file is based on a file originally part of the
 * MicroPython project, http://micropython.org/
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2019 Damien P. George
 */

#include <hardware/irq.h>
#include <hardware/structs/sio.h>
#include <hardware/uart.h>
#include <hardware/flash.h>
#include <hardware/clocks.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <string.h>

#include "tusb.h"

#if !defined(MIN)
#define MIN(a, b) ((a > b) ? b : a)
#endif /* MIN */

#define BUFFER_SIZE 2560

#define DEF_BIT_RATE 115200
#define DEF_STOP_BITS 1
#define DEF_PARITY 0
#define DEF_DATA_BITS 8

const char COMPLETE_FLAG[] = "PROGRAMMING_COMPLETE";
const size_t COMPLETE_FLAG_READ_LEN = 22;

typedef struct {
	uart_inst_t *const inst;
	uint irq;
	void *irq_fn;
	uint8_t tx_pin;
	uint8_t rx_pin;
} uart_id_t;

typedef struct {
	cdc_line_coding_t usb_lc;
	cdc_line_coding_t uart_lc;
	mutex_t lc_mtx;
	uint8_t uart_buffer[BUFFER_SIZE];
	uint32_t uart_pos;
	mutex_t uart_mtx;
	uint8_t usb_buffer[BUFFER_SIZE];
	uint32_t usb_pos;
	mutex_t usb_mtx;
} uart_data_t;

void uart0_irq_fn(void);
void uart1_irq_fn(void);

const uart_id_t UART_ID[CFG_TUD_CDC] = {
	{
		.inst = uart0,
		.irq = UART0_IRQ,
		.irq_fn = &uart0_irq_fn,
		.tx_pin = UART0_TX_PIN,
		.rx_pin = UART0_RX_PIN,
	}, 
#if CFG_TUD_CDC > 1	
	{
		.inst = uart1,
		.irq = UART1_IRQ,
		.irq_fn = &uart1_irq_fn,
		.tx_pin = 4,
		.rx_pin = 5,
	}
#endif
};

uart_data_t UART_DATA[CFG_TUD_CDC];
bool programming_complete = false;

static inline uint databits_usb2uart(uint8_t data_bits)
{
	switch (data_bits) {
		case 5:
			return 5;
		case 6:
			return 6;
		case 7:
			return 7;
		default:
			return 8;
	}
}

static inline uart_parity_t parity_usb2uart(uint8_t usb_parity)
{
	switch (usb_parity) {
		case 1:
			return UART_PARITY_ODD;
		case 2:
			return UART_PARITY_EVEN;
		default:
			return UART_PARITY_NONE;
	}
}

static inline uint stopbits_usb2uart(uint8_t stop_bits)
{
	switch (stop_bits) {
		case 2:
			return 2;
		default:
			return 1;
	}
}

void update_uart_cfg(uint8_t itf)
{
	const uart_id_t *ui = &UART_ID[itf];
	uart_data_t *ud = &UART_DATA[itf];

	mutex_enter_blocking(&ud->lc_mtx);

	if (ud->usb_lc.bit_rate != ud->uart_lc.bit_rate) {
		uart_set_baudrate(ui->inst, ud->usb_lc.bit_rate);
		ud->uart_lc.bit_rate = ud->usb_lc.bit_rate;
	}

	if ((ud->usb_lc.stop_bits != ud->uart_lc.stop_bits) ||
	    (ud->usb_lc.parity != ud->uart_lc.parity) ||
	    (ud->usb_lc.data_bits != ud->uart_lc.data_bits)) {
		uart_set_format(ui->inst,
				databits_usb2uart(ud->usb_lc.data_bits),
				stopbits_usb2uart(ud->usb_lc.stop_bits),
				parity_usb2uart(ud->usb_lc.parity));
		ud->uart_lc.data_bits = ud->usb_lc.data_bits;
		ud->uart_lc.parity = ud->usb_lc.parity;
		ud->uart_lc.stop_bits = ud->usb_lc.stop_bits;
	}

	mutex_exit(&ud->lc_mtx);
}

void usb_read_bytes(uint8_t itf)
{
	uart_data_t *ud = &UART_DATA[itf];
	uint32_t len = tud_cdc_n_available(itf);

	if (len && mutex_try_enter(&ud->usb_mtx, NULL)) 
	{
		len = MIN(len, BUFFER_SIZE - ud->usb_pos);
		if (len) 
		{
			uint32_t count;
			count = tud_cdc_n_read(itf, &ud->usb_buffer[ud->usb_pos], len);
			
			if (count >= COMPLETE_FLAG_READ_LEN) 
			{
				for (uint32_t i = 0; i < count; i++)
				{
					uint32_t remaining = BUFFER_SIZE - ud->usb_pos - i;
					if (remaining >= sizeof(COMPLETE_FLAG) - 1 &&
						memcmp(&ud->usb_buffer[ud->usb_pos + i], COMPLETE_FLAG, sizeof(COMPLETE_FLAG) - 1) == 0)
					{
						programming_complete = true;
						break;
					}
				}
			}

			ud->usb_pos += count;
		}

		mutex_exit(&ud->usb_mtx);
	}
}

void usb_write_bytes(uint8_t itf)
{
	uart_data_t *ud = &UART_DATA[itf];

	if (ud->uart_pos &&
	    mutex_try_enter(&ud->uart_mtx, NULL)) {
		uint32_t count;

		count = tud_cdc_n_write(itf, ud->uart_buffer, ud->uart_pos);
		if (count < ud->uart_pos)
			memmove(ud->uart_buffer, &ud->uart_buffer[count],
			       ud->uart_pos - count);
		ud->uart_pos -= count;

		mutex_exit(&ud->uart_mtx);

		if (count)
			tud_cdc_n_write_flush(itf);
	}
}

void usb_cdc_process(uint8_t itf)
{
	uart_data_t *ud = &UART_DATA[itf];

	mutex_enter_blocking(&ud->lc_mtx);
	tud_cdc_n_get_line_coding(itf, &ud->usb_lc);
	mutex_exit(&ud->lc_mtx);

	usb_read_bytes(itf);
	usb_write_bytes(itf);
}

static inline void uart_read_bytes(uint8_t itf)
{
	uart_data_t *ud = &UART_DATA[itf];
	const uart_id_t *ui = &UART_ID[itf];

	if (uart_is_readable(ui->inst)) {
		mutex_enter_blocking(&ud->uart_mtx);

		while (uart_is_readable(ui->inst) &&
		       (ud->uart_pos < BUFFER_SIZE)) {
			ud->uart_buffer[ud->uart_pos] = uart_getc(ui->inst);
			ud->uart_pos++;
		}

		mutex_exit(&ud->uart_mtx);
	}
}

void uart0_irq_fn(void)
{
	uart_read_bytes(0);
}

void uart1_irq_fn(void)
{
	uart_read_bytes(1);
}

void uart_write_bytes(uint8_t itf)
{
	uart_data_t *ud = &UART_DATA[itf];

	if (ud->usb_pos &&
	    mutex_try_enter(&ud->usb_mtx, NULL)) {
		const uart_id_t *ui = &UART_ID[itf];
		uint32_t count = 0;

		while (uart_is_writable(ui->inst) &&
		       count < ud->usb_pos) {
			uart_putc_raw(ui->inst, ud->usb_buffer[count]);
			count++;
		}

		if (count < ud->usb_pos)
			memmove(ud->usb_buffer, &ud->usb_buffer[count],
			       ud->usb_pos - count);
		ud->usb_pos -= count;

		mutex_exit(&ud->usb_mtx);
	}
}

void init_uart_data(uint8_t itf)
{
	const uart_id_t *ui = &UART_ID[itf];
	uart_data_t *ud = &UART_DATA[itf];

	/* Pinmux */
	gpio_set_function(ui->tx_pin, GPIO_FUNC_UART);
	gpio_set_function(ui->rx_pin, GPIO_FUNC_UART);

	/* USB CDC LC */
	ud->usb_lc.bit_rate = DEF_BIT_RATE;
	ud->usb_lc.data_bits = DEF_DATA_BITS;
	ud->usb_lc.parity = DEF_PARITY;
	ud->usb_lc.stop_bits = DEF_STOP_BITS;

	/* UART LC */
	ud->uart_lc.bit_rate = DEF_BIT_RATE;
	ud->uart_lc.data_bits = DEF_DATA_BITS;
	ud->uart_lc.parity = DEF_PARITY;
	ud->uart_lc.stop_bits = DEF_STOP_BITS;

	/* Buffer */
	ud->uart_pos = 0;
	ud->usb_pos = 0;

	/* Mutex */
	mutex_init(&ud->lc_mtx);
	mutex_init(&ud->uart_mtx);
	mutex_init(&ud->usb_mtx);

	/* UART start */
	uart_init(ui->inst, ud->usb_lc.bit_rate);
	uart_set_hw_flow(ui->inst, false, false);
	uart_set_format(ui->inst, databits_usb2uart(ud->usb_lc.data_bits),
			stopbits_usb2uart(ud->usb_lc.stop_bits),
			parity_usb2uart(ud->usb_lc.parity));
	uart_set_fifo_enabled(ui->inst, false);

	/* UART RX Interrupt */
	irq_set_exclusive_handler(ui->irq, ui->irq_fn);
	irq_set_enabled(ui->irq, true);
	uart_set_irq_enables(ui->inst, true, false);
}

void core1_entry(void)
{
	for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++)
	{
		init_uart_data(0);
	}

	while (1) 
	{
		for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) 
		{
			update_uart_cfg(itf);
			uart_write_bytes(itf);
		}
	}
}

int uart_bridge_run(void)
{
	set_sys_clock_khz(125000, false);

	tud_init(BOARD_TUD_RHPORT);

	multicore_reset_core1();
	multicore_launch_core1(core1_entry);

	while (1) 
	{
		tud_task();

		for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) 
		{
			if (tud_cdc_n_connected(itf)) 
			{
				usb_cdc_process(itf);
			}
		}

		if (programming_complete)
		{
			multicore_reset_core1();
			break;
		}
	}

	return 0;
}