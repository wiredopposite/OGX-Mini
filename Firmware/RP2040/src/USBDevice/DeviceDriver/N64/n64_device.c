/* N64 controller device (output to N64 console). Ported from pdaxrom/usb2n64-adapter. */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "n64send.pio.h"
#include "n64_device.h"

#define N64SEND_DATA(d0, d1, b) ((((b) - 1) << 16) | ((uint32_t)(d0) << 8) | (uint32_t)(d1))

/* Shared report from Core0; Core1 reads when handling poll. */
static volatile N64Report s_n64_report;

void n64_device_set_report(const N64Report* report) {
    if (report) {
        s_n64_report.buttons1 = report->buttons1;
        s_n64_report.buttons2 = report->buttons2;
        s_n64_report.joy_x = report->joy_x;
        s_n64_report.joy_y = report->joy_y;
    }
}

static __not_in_flash_func(uint32_t) read_bits(int pin, int num_bits) {
    uint32_t value = 0;
    for (int i = 0; i < num_bits; i++) {
        busy_wait_us(2);
        value = (value << 1) | (gpio_get(pin) ? 1u : 0u);
    }
    return value;
}

/* Wait for console to pull line low (start of command). */
static __not_in_flash_func(void) wait_for_line_low(int pin) {
    while (gpio_get(pin)) { }
}

static PIO s_n64_pio;
static uint s_n64_sm;
static uint s_n64_offset;
static uint s_n64_dma_chan;

static __not_in_flash_func(void) send_response(uint32_t* dma_buf, int num_words) {
    pio_sm_exec(s_n64_pio, s_n64_sm, pio_encode_jmp(s_n64_offset + n64send_dma_offset_loop));
    dma_channel_transfer_from_buffer_now(s_n64_dma_chan, dma_buf, num_words);
    dma_channel_wait_for_finish_blocking(s_n64_dma_chan);
    while (pio_sm_get_pc(s_n64_pio, s_n64_sm) != (s_n64_offset + n64send_dma_offset_stop)) { }
}

static uint32_t s_dma_buf[8];

void n64_device_main(int data_pin) {
    gpio_init(data_pin);
    gpio_put(data_pin, 0);
    gpio_pull_up(data_pin);
    gpio_set_dir(data_pin, GPIO_IN);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &n64send_dma_program);
    uint sm = pio_claim_unused_sm(pio, true);
    s_n64_pio = pio;
    s_n64_sm = sm;
    s_n64_offset = offset;

    uint dma_chan = dma_claim_unused_channel(true);
    s_n64_dma_chan = dma_chan;

    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &cfg, &pio->txf[sm], NULL, 0, false);

    pio_sm_config c = n64send_dma_program_get_default_config(offset);
    sm_config_set_out_shift(&c, false, false, 32);
    sm_config_set_in_shift(&c, false, false, 32);
    sm_config_set_out_pins(&c, data_pin, 1);
    sm_config_set_set_pins(&c, data_pin, 1);
    pio_gpio_init(pio, data_pin);
    pio_sm_set_consecutive_pindirs(pio, sm, data_pin, 1, false);
    sm_config_set_clkdiv(&c, 16.625f);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    for (;;) {
        wait_for_line_low(data_pin);
        uint32_t cmd = read_bits(data_pin, 9);
        /* Command byte is bits 1-8 (bit 0 is start). */
        uint8_t cmd_byte = (uint8_t)(cmd >> 1);

        if (cmd_byte == 0x00 || cmd_byte == 0xFF) {
            /* Identity: standard controller 0x05, 0x00; then 0x01 (1 byte). */
            s_dma_buf[0] = N64SEND_DATA(0x05, 0x00, 16);
            s_dma_buf[1] = N64SEND_DATA(0x01, 0x00, 8);
            s_dma_buf[2] = 0;
            send_response(s_dma_buf, 3);
        } else if (cmd_byte == 0x01) {
            /* Poll: 4 bytes = buttons1, buttons2, joy_x, joy_y */
            uint8_t b0 = s_n64_report.buttons1;
            uint8_t b1 = s_n64_report.buttons2;
            int8_t jx = s_n64_report.joy_x;
            int8_t jy = s_n64_report.joy_y;
            s_dma_buf[0] = N64SEND_DATA(b0, b1, 16);
            s_dma_buf[1] = N64SEND_DATA((uint8_t)jx, (uint8_t)jy, 16);
            s_dma_buf[2] = 0;
            send_response(s_dma_buf, 3);
        }
        /* Ignore 0x02/0x03 (memory pak) and 0x13 (keyboard) for now. */
    }
}
