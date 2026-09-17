/* JoyBus host (read GameCube controller). Ported from PicoGamepadConverter (Loc15). */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "controller.pio.h"

#define JOYBUS_STATE_BYTES 8

static PIO joybus_pio;
static uint joybus_sm;
static pio_sm_config joybus_config;
static uint joybus_offset;
static uint8_t controller_state[JOYBUS_STATE_BYTES];

static void update_pio_output_size(uint8_t auto_pull_length)
{
    pio_sm_set_enabled(joybus_pio, joybus_sm, false);
    joybus_pio->sm[joybus_sm].shiftctrl = (joybus_pio->sm[joybus_sm].shiftctrl & 0xA00FFFFF) |
        (0x8u << 20) |
        (((auto_pull_length + 5) & 0x1Fu) << 25);
    joybus_pio->ctrl |= 1u << (4 + joybus_sm);
    pio_sm_set_enabled(joybus_pio, joybus_sm, true);
}

void joybus_host_init(uint pio_index, uint pin)
{
    joybus_pio = pio_index ? pio1 : pio0;
    joybus_offset = pio_add_program(joybus_pio, &controller_program);
    joybus_sm = pio_claim_unused_sm(joybus_pio, true);
    joybus_config = controller_program_get_default_config(joybus_offset);
    controller_program_init(joybus_pio, joybus_sm, joybus_offset, pin, &joybus_config);
    update_pio_output_size(2);  /* first transfer is poll: 2 bytes request */
}

void joybus_send_data(const uint8_t *request, uint8_t data_length, uint8_t response_length)
{
    uint32_t data_with_response_length = ((response_length - 1) & 0x1Fu) << 27;
    for (int i = 0; i < data_length; i++)
    {
        data_with_response_length |= (uint32_t)request[i] << (19 - i * 8);
    }
    pio_sm_put_blocking(joybus_pio, joybus_sm, data_with_response_length);

    int16_t remaining = (int16_t)response_length;
    while (remaining > 0)
    {
        absolute_time_t timeout = make_timeout_time_us(600);
        while (pio_sm_is_rx_fifo_empty(joybus_pio, joybus_sm) && !time_reached(timeout))
        {
        }
        uint32_t data = pio_sm_get(joybus_pio, joybus_sm);
        controller_state[response_length - remaining] = (uint8_t)(data & 0xFFu);
        remaining--;
    }
    /* Next transfer will send data_length bytes (e.g. 2 for poll). */
    update_pio_output_size(data_length);
}

const uint8_t* joybus_get_response(void)
{
    return controller_state;
}
