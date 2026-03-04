// SPDX-License-Identifier: MIT
#include <hardware/irq.h>
#include <hardware/structs/sio.h>
#include <hardware/uart.h>
#include <hardware/flash.h>
#include <hardware/clocks.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <string.h>

#include "Board/Config.h"
#include "tusb.h"
#include "device/usbd.h"

#if !defined(MIN)
#define MIN(a, b) ((a > b) ? b : a)
#endif 

#define BUFFER_SIZE 2560
#define DEF_BIT_RATE 115200

const char COMPLETE_FLAG[] = "PROGRAMMING_COMPLETE";
const size_t COMPLETE_FLAG_LEN = 20; // "PROGRAMMING_COMPLETE"

typedef struct {
	uart_inst_t *const inst;
	uint8_t tx_pin;
	uint8_t rx_pin;
} uart_id_t;

typedef struct {
	cdc_line_coding_t usb_lc;
	cdc_line_coding_t uart_lc;
	mutex_t lc_mtx;
	uint8_t uart_buffer[BUFFER_SIZE];
	volatile uint32_t uart_pos;
	mutex_t uart_mtx;
	uint8_t usb_buffer[BUFFER_SIZE];
	volatile uint32_t usb_pos;
	mutex_t usb_mtx;
} uart_data_t;

const uart_id_t UART_ID[CFG_TUD_CDC] = {
	{
		.inst = uart0,
		.tx_pin = UART0_TX_PIN,
		.rx_pin = UART0_RX_PIN,
	}
};

uart_data_t UART_DATA[CFG_TUD_CDC];
static uint32_t bridge_start_ms = 0;
static volatile bool programming_complete = false;

static inline uint databits_usb2uart(uint8_t data_bits) {
	return (data_bits >= 5 && data_bits <= 8) ? data_bits : 8;
}

static inline uart_parity_t parity_usb2uart(uint8_t usb_parity) {
	if (usb_parity == 1) return UART_PARITY_ODD;
	if (usb_parity == 2) return UART_PARITY_EVEN;
	return UART_PARITY_NONE;
}

static inline uint stopbits_usb2uart(uint8_t stop_bits) {
	return (stop_bits == 2) ? 2 : 1;
}

void uart_bridge_line_state_cb(uint8_t itf, bool dtr, bool rts) {
#if defined(ESP_RST_PIN) && defined(ESP_PROG_PIN)
    // Protection: OS drivers often toggle DTR/RTS when the port is opened.
    // If we just entered UART mode manually, we ignore all resets for 5 seconds.
    if (to_ms_since_boot(get_absolute_time()) - bridge_start_ms < 5000) return;

    // Corrected direct-connect auto-reset logic:
	gpio_put(ESP_RST_PIN,  !( dtr && (!rts) ));
	gpio_put(ESP_PROG_PIN, !( rts && (!dtr) ));
#endif
}

void uart_bridge_line_coding_cb(uint8_t itf, cdc_line_coding_t const* lc) {
	uart_data_t *ud = &UART_DATA[itf];
	if (mutex_try_enter(&ud->lc_mtx, NULL)) {
		ud->usb_lc = *lc;
		mutex_exit(&ud->lc_mtx);
	}
}

void update_uart_cfg(uint8_t itf) {
	const uart_id_t *ui = &UART_ID[itf];
	uart_data_t *ud = &UART_DATA[itf];
	if (mutex_try_enter(&ud->lc_mtx, NULL)) {
		if (ud->usb_lc.bit_rate != ud->uart_lc.bit_rate && ud->usb_lc.bit_rate > 0) {
			uart_set_baudrate(ui->inst, ud->usb_lc.bit_rate);
			ud->uart_lc.bit_rate = ud->usb_lc.bit_rate;
		}
		if ((ud->usb_lc.stop_bits != ud->uart_lc.stop_bits) ||
			(ud->usb_lc.parity != ud->uart_lc.parity) ||
			(ud->usb_lc.data_bits != ud->uart_lc.data_bits)) {
			uart_set_format(ui->inst, databits_usb2uart(ud->usb_lc.data_bits), 
                            stopbits_usb2uart(ud->usb_lc.stop_bits), 
                            parity_usb2uart(ud->usb_lc.parity));
			ud->uart_lc = ud->usb_lc;
		}
		mutex_exit(&ud->lc_mtx);
	}
}

void core1_entry(void) {
	for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) {
		const uart_id_t *ui = &UART_ID[itf];
		gpio_set_function(ui->tx_pin, GPIO_FUNC_UART);
		gpio_set_function(ui->rx_pin, GPIO_FUNC_UART);
		uart_init(ui->inst, DEF_BIT_RATE);
		uart_set_fifo_enabled(ui->inst, true);
        while (uart_is_readable(ui->inst)) uart_getc(ui->inst);
	}
	while (1) {
		for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) {
			update_uart_cfg(itf);
			uart_data_t *ud = &UART_DATA[itf];
			const uart_id_t *ui = &UART_ID[itf];

			if (uart_is_readable(ui->inst) && mutex_try_enter(&ud->uart_mtx, NULL)) {
				while (uart_is_readable(ui->inst) && (ud->uart_pos < BUFFER_SIZE)) {
					ud->uart_buffer[ud->uart_pos++] = uart_getc(ui->inst);
				}
				mutex_exit(&ud->uart_mtx);
			}

			if (ud->usb_pos && mutex_try_enter(&ud->usb_mtx, NULL)) {
				uint32_t count = 0;
				while (uart_is_writable(ui->inst) && count < ud->usb_pos) {
					uart_putc_raw(ui->inst, ud->usb_buffer[count++]);
				}
				if (count < ud->usb_pos) memmove(ud->usb_buffer, &ud->usb_buffer[count], ud->usb_pos - count);
				ud->usb_pos -= count;
				mutex_exit(&ud->usb_mtx);
			}
		}
	}
}

int uart_bridge_run(void) {
    bridge_start_ms = to_ms_since_boot(get_absolute_time());
    programming_complete = false;

#if defined(ESP_RST_PIN) && defined(ESP_PROG_PIN)
	gpio_init(ESP_RST_PIN); gpio_set_dir(ESP_RST_PIN, GPIO_OUT); gpio_put(ESP_RST_PIN, 1);
	gpio_init(ESP_PROG_PIN); gpio_set_dir(ESP_PROG_PIN, GPIO_OUT); gpio_put(ESP_PROG_PIN, 1);
#endif

	for (uint8_t i = 0; i < CFG_TUD_CDC; i++) {
		mutex_init(&UART_DATA[i].lc_mtx);
		mutex_init(&UART_DATA[i].uart_mtx);
		mutex_init(&UART_DATA[i].usb_mtx);
		UART_DATA[i].usb_lc.bit_rate = DEF_BIT_RATE;
        UART_DATA[i].usb_lc.data_bits = 8;
	}

    if (!tud_inited()) tud_init(BOARD_TUD_RHPORT);
	multicore_launch_core1(core1_entry);

	while (!programming_complete) {
		tud_task();
		for (uint8_t i = 0; i < CFG_TUD_CDC; i++) {
			uart_data_t *ud = &UART_DATA[i];
			if (ud->uart_pos && mutex_try_enter(&ud->uart_mtx, NULL)) {
				uint32_t count = tud_cdc_n_write(i, ud->uart_buffer, ud->uart_pos);
				if (count < ud->uart_pos) memmove(ud->uart_buffer, &ud->uart_buffer[count], ud->uart_pos - count);
				ud->uart_pos -= count;
				mutex_exit(&ud->uart_mtx);
				tud_cdc_n_write_flush(i);
			}
			uint32_t avail = tud_cdc_n_available(i);
			if (avail && mutex_try_enter(&ud->usb_mtx, NULL)) {
				uint32_t n = MIN(avail, BUFFER_SIZE - ud->usb_pos);
				uint32_t read = tud_cdc_n_read(i, &ud->usb_buffer[ud->usb_pos], n);
                
                // Check for completion flag in the data from PC
                if (read >= COMPLETE_FLAG_LEN) {
                    for (uint32_t j = 0; j <= read - COMPLETE_FLAG_LEN; j++) {
                        if (memcmp(&ud->usb_buffer[ud->usb_pos + j], COMPLETE_FLAG, COMPLETE_FLAG_LEN) == 0) {
                            programming_complete = true;
                        }
                    }
                }
				ud->usb_pos += read;
				mutex_exit(&ud->usb_mtx);
			}
		}
        sleep_ms(1);
	}
	return 0;
}
