/* PS1/PS2 controller host (read real controller). Ported from PicoGamepadConverter (Loc15).
 * https://github.com/Loc15/PicoGamepadConverter */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "clock.pio.h"

#define N_BYTES 9

static PIO psx_pio;
static uint psx_sm;
static int psx_dma_chan;
static uint32_t data_psx[N_BYTES];
static const uint32_t cmd = 0x01u | (0x42u << 8);

static volatile uint8_t psx_host_buffer[N_BYTES];
static volatile bool psx_host_ready;

static void __not_in_flash_func(dma_handler)(void)
{
    dma_hw->ints0 = 1u << psx_dma_chan;
    dma_channel_set_read_addr(psx_dma_chan, &psx_pio->rxf[psx_sm], false);
    dma_channel_set_write_addr(psx_dma_chan, (void *)&data_psx[0], false);
    dma_channel_set_trans_count(psx_dma_chan, N_BYTES, false);
    dma_channel_start(psx_dma_chan);

    for (int i = 0; i < N_BYTES; i++)
    {
        psx_host_buffer[i] = (uint8_t)(data_psx[i] & 0xFFu);
    }
    psx_host_ready = true;

    pio_sm_put_blocking(psx_pio, psx_sm, cmd);
}

void psx_host_init(uint pio_index, uint gpio_output, uint gpio_input)
{
    psx_pio = pio_index ? pio1 : pio0;
    psx_host_ready = false;

    uint offset = pio_add_program(psx_pio, &clock_program);
    psx_sm = pio_claim_unused_sm(psx_pio, true);
    pio_sm_config c = clock_program_get_default_config(offset);
    sm_config_set_set_pins(&c, gpio_output + 1, 2);
    sm_config_set_out_pins(&c, gpio_output, 1);
    sm_config_set_sideset_pins(&c, gpio_output);
    sm_config_set_in_pins(&c, gpio_input);
    sm_config_set_in_shift(&c, true, true, 8);
    pio_gpio_init(psx_pio, gpio_output);
    pio_gpio_init(psx_pio, gpio_output + 1);
    pio_gpio_init(psx_pio, gpio_output + 2);
    pio_gpio_init(psx_pio, gpio_input);
    gpio_pull_up(gpio_input);
    gpio_pull_up(gpio_output + 1);
    gpio_pull_up(gpio_output + 2);
    pio_sm_set_consecutive_pindirs(psx_pio, psx_sm, gpio_output, 3, true);
    pio_sm_set_consecutive_pindirs(psx_pio, psx_sm, gpio_input, 1, false);
    float div = (float)clock_get_hz(clk_sys) / (6.0f * 12500.0f);
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(psx_pio, psx_sm, offset, &c);
    pio_sm_set_enabled(psx_pio, psx_sm, true);

    psx_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c_dma = dma_channel_get_default_config(psx_dma_chan);
    channel_config_set_transfer_data_size(&c_dma, DMA_SIZE_32);
    channel_config_set_read_increment(&c_dma, false);
    channel_config_set_write_increment(&c_dma, true);
    channel_config_set_dreq(&c_dma, pio_get_dreq(psx_pio, psx_sm, false));
    dma_channel_configure(psx_dma_chan, &c_dma,
                         (void *)&data_psx[0],
                         &psx_pio->rxf[psx_sm],
                         N_BYTES,
                         false);
    dma_channel_set_irq0_enabled(psx_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_handler();
}

bool psx_host_take_data(uint8_t *out)
{
    if (!psx_host_ready)
    {
        return false;
    }
    for (int i = 0; i < N_BYTES; i++)
    {
        out[i] = psx_host_buffer[i];
    }
    psx_host_ready = false;
    return true;
}
