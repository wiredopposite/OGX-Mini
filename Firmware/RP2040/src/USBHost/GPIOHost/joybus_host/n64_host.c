/* N64 controller host (1-wire). Ported from PicoGamepadConverter (Loc15) / joybus_host. */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "controller.pio.h"

#define N64_RESPONSE_BYTES 4

static PIO n64_pio;
static uint n64_sm;
static pio_sm_config n64_config;
static uint n64_offset;
static uint8_t n64_state[N64_RESPONSE_BYTES];

static void n64_update_pio_output_size(uint8_t auto_pull_length)
{
    pio_sm_set_enabled(n64_pio, n64_sm, false);
    n64_pio->sm[n64_sm].shiftctrl = (n64_pio->sm[n64_sm].shiftctrl & 0xA00FFFFF) |
        (0x8u << 20) |
        (((auto_pull_length + 5) & 0x1Fu) << 25);
    n64_pio->ctrl |= 1u << (4 + n64_sm);
    pio_sm_set_enabled(n64_pio, n64_sm, true);
}

static void n64_send_data(const uint8_t *request, uint8_t data_length, uint8_t response_length)
{
    uint32_t data_with_response_length = ((response_length - 1) & 0x1Fu) << 27;
    for (int i = 0; i < data_length; i++)
    {
        data_with_response_length |= (uint32_t)request[i] << (19 - i * 8);
    }
    pio_sm_put_blocking(n64_pio, n64_sm, data_with_response_length);

    int16_t remaining = (int16_t)response_length;
    while (remaining > 0)
    {
        absolute_time_t timeout = make_timeout_time_us(600);
        while (pio_sm_is_rx_fifo_empty(n64_pio, n64_sm) && !time_reached(timeout))
        {
        }
        uint32_t data = pio_sm_get(n64_pio, n64_sm);
        n64_state[response_length - remaining] = (uint8_t)(data & 0xFFu);
        remaining--;
    }
    n64_update_pio_output_size(data_length);
}

void n64_host_init(uint pio_index, uint data_pin)
{
    n64_pio = pio_index ? pio1 : pio0;
    n64_offset = pio_add_program(n64_pio, &controller_program);
    n64_sm = pio_claim_unused_sm(n64_pio, true);
    n64_config = controller_program_get_default_config(n64_offset);
    controller_program_init(n64_pio, n64_sm, n64_offset, data_pin, &n64_config);
    n64_update_pio_output_size(1);

    /* N64 init: send 0x00, expect 1 byte (identify). */
    const uint8_t init_cmd[] = { 0x00 };
    n64_send_data(init_cmd, 1, 1);
    sleep_us(200);
}

void n64_host_poll(void)
{
    const uint8_t poll_cmd[] = { 0x01 };
    n64_send_data(poll_cmd, 1, N64_RESPONSE_BYTES);
    busy_wait_us(500);
}

const uint8_t* n64_host_get_response(void)
{
    return n64_state;
}
