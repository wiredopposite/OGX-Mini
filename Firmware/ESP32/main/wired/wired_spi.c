#include "sdkconfig.h"
#if defined(CONFIG_SPI_ENABLED)

#include <freertos/FreeRTOS.h>
#include <esp_private/periph_ctrl.h>
#include <rom/lldesc.h>
#include <hal/gpio_ll.h>
#include <hal/spi_ll.h>
#include <hal/spi_hal.h>

#include "log/log.h"
#include "wired/wired.h"
#include "wired/wired_ring.h"

#define SPI_DEVICE      SPI2_HOST
#define SPI_PIN_CS      15
#define SPI_PIN_SCK     14
#define SPI_PIN_MOSI    13 
#define SPI_PIN_MISO    12 
#define SPI_PIN_IRQ     5 

typedef struct {
    spi_hal_context_t       hal;
    spi_hal_dev_config_t    hal_dev_cfg;
    int                     dma_channel;
    ring_wired_t            ring_spi;
    uint8_t                 bt_buf_tx[WIRED_MAX_SIZE] __attribute__((aligned(4))); /* Belongs to bt thread */
    uint8_t*                dma_buf_tx; /* Belongs to spi thread */
    uint8_t*                dma_buf_rx; /* Belongs to spi thread */
    lldesc_t*               dma_desc_tx;
    lldesc_t*               dma_desc_rx;
    wired_rumble_cb_t       rumble_cb;
    wired_audio_cb_t        audio_cb;
} spi_state_t;

static spi_state_t spi_state = {0};
// static const size_t spi_state_size = sizeof(spi_state_t);

static void spi_init(void) {
    spi_state.dma_channel = 1;
    PERIPH_RCC_ATOMIC() {
        spi_ll_enable_bus_clock(SPI_DEVICE, true);
        spi_ll_reset_register(SPI_DEVICE);
        spi_dma_ll_enable_bus_clock(spi_state.dma_channel, true);
        spi_dma_ll_reset_register(spi_state.dma_channel);
        /* Link DMA */
        DPORT_SET_PERI_REG_BITS(DPORT_SPI_DMA_CHAN_SEL_REG, 3, spi_state.dma_channel, (SPI_DEVICE * 2));
    }

    gpio_ll_iomux_func_sel(IO_MUX_GPIO14_REG, FUNC_MTMS_HSPICLK);
    gpio_ll_iomux_func_sel(IO_MUX_GPIO13_REG, FUNC_MTCK_HSPID);  
    gpio_ll_iomux_func_sel(IO_MUX_GPIO12_REG, FUNC_MTDI_HSPIQ);  
    gpio_ll_iomux_func_sel(IO_MUX_GPIO15_REG, FUNC_MTDO_HSPICS0);

    gpio_ll_output_enable(&GPIO, SPI_PIN_SCK); 
    gpio_ll_output_enable(&GPIO, SPI_PIN_MOSI);
    gpio_ll_input_enable(&GPIO, SPI_PIN_MISO); 
    gpio_ll_output_enable(&GPIO, SPI_PIN_CS);

    spi_hal_timing_param_t timing = {
        .clk_src_hz = APB_CLK_FREQ,
        .half_duplex = false,
        .no_compensate = false,
        .expected_freq = 1*1000*1000,
        .duty_cycle = 128,
        .input_delay_ns = 0,
        .use_gpio = true,
    };

    spi_state.hal_dev_cfg.mode = 3;
    spi_state.hal_dev_cfg.cs_setup = 1;
    spi_state.hal_dev_cfg.cs_hold = 1;
    spi_state.hal_dev_cfg.cs_pin_id = -1;
    ESP_ERROR_CHECK(spi_hal_cal_clock_conf(&timing, &spi_state.hal_dev_cfg.timing_conf));
    spi_state.hal_dev_cfg.sio = 0;
    spi_state.hal_dev_cfg.half_duplex = 0;
    spi_state.hal_dev_cfg.tx_lsbfirst = 0;
    spi_state.hal_dev_cfg.rx_lsbfirst = 0;
    spi_state.hal_dev_cfg.no_compensate = 0;
    spi_state.hal_dev_cfg.as_cs = 0;
    spi_state.hal_dev_cfg.positive_cs = 0;

    spi_hal_init(&spi_state.hal, SPI_DEVICE);
    spi_hal_setup_device(&spi_state.hal, &spi_state.hal_dev_cfg);
    spi_hal_enable_data_line(spi_state.hal.hw, true, true);

    gpio_ll_output_enable(&GPIO, SPI_PIN_CS);
    gpio_ll_set_level(&GPIO, SPI_PIN_CS, 1);

    gpio_ll_input_enable(&GPIO, SPI_PIN_IRQ);
    gpio_ll_pullup_en(&GPIO, SPI_PIN_IRQ);

    spi_state.dma_buf_tx = heap_caps_malloc(WIRED_MAX_SIZE, MALLOC_CAP_DMA);
    spi_state.dma_buf_rx = heap_caps_malloc(WIRED_MAX_SIZE, MALLOC_CAP_DMA);
    spi_state.dma_desc_tx = heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA);
    spi_state.dma_desc_rx = heap_caps_malloc(sizeof(lldesc_t), MALLOC_CAP_DMA);
    if ((spi_state.dma_buf_tx == NULL) || 
        (spi_state.dma_buf_rx == NULL) ||
        (spi_state.dma_desc_rx == NULL) || 
        (spi_state.dma_desc_tx == NULL)) {
        ogxm_loge("SPI: Failed to allocate DMA buffers\n");
        abort();
    }
    memset(spi_state.dma_buf_tx, 0, WIRED_MAX_SIZE);
    memset(spi_state.dma_buf_rx, 0, WIRED_MAX_SIZE);
    memset(spi_state.dma_desc_tx, 0, sizeof(lldesc_t));
    memset(spi_state.dma_desc_rx, 0, sizeof(lldesc_t));

    spi_state.dma_desc_rx->size = WIRED_MAX_SIZE;
    spi_state.dma_desc_rx->length = WIRED_MAX_SIZE;
    spi_state.dma_desc_rx->eof = 1;
    spi_state.dma_desc_rx->owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
    spi_state.dma_desc_rx->buf = spi_state.dma_buf_rx;
    spi_state.dma_desc_rx->empty = 0;

    spi_state.dma_desc_tx->size = WIRED_MAX_SIZE;
    spi_state.dma_desc_tx->length = WIRED_MAX_SIZE;
    spi_state.dma_desc_tx->eof = 1;
    spi_state.dma_desc_tx->owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
    spi_state.dma_desc_tx->buf = spi_state.dma_buf_tx;
    spi_state.dma_desc_tx->empty = 0;
}

static bool sync_slave(void) {
    return true;
}

static void transmit_blocking(void) {
    spi_hal_trans_config_t xfer_cfg = {
        .cmd = 0,
        .cmd_bits = 0,
        .addr = 0,
        .addr_bits = 0,
        .dummy_bits = 0,
        .tx_bitlen = WIRED_MAX_SIZE * 8U,
        .rx_bitlen = WIRED_MAX_SIZE * 8U,
        .send_buffer = spi_state.dma_buf_tx,
        .rcv_buffer = spi_state.dma_buf_rx,
        .line_mode = {1,1,1},
        .cs_keep_active = false,
    };

    ogxm_logd_hex(spi_state.dma_buf_tx, WIRED_MAX_SIZE, "SPI TX Buffer before:");

    gpio_ll_set_level(&GPIO, SPI_PIN_CS, 0);

    spi_hal_setup_trans(&spi_state.hal, &spi_state.hal_dev_cfg, &xfer_cfg);
    spi_hal_hw_prepare_tx(spi_state.hal.hw);
    spi_hal_hw_prepare_rx(spi_state.hal.hw);
    spi_dma_ll_rx_start(spi_state.hal.hw, spi_state.dma_channel, spi_state.dma_desc_rx);
    spi_dma_ll_tx_start(spi_state.hal.hw, spi_state.dma_channel, spi_state.dma_desc_tx);

    spi_ll_user_start(spi_state.hal.hw);
    while (!spi_ll_usr_is_done(spi_state.hal.hw)) {
        // Wait
    }

    gpio_ll_set_level(&GPIO, SPI_PIN_CS, 1);

    ogxm_logd_hex(spi_state.dma_buf_tx, WIRED_MAX_SIZE, "SPI TX Buffer after:");
}

static void parse_rx(void) {
    const wired_packet_t* packet = 
        (const wired_packet_t*)spi_state.dma_buf_rx;
    switch (packet->report_id) {
    case WIRED_REPORT_ID_PCM:
        if (spi_state.audio_cb != NULL) {
            spi_state.audio_cb(packet->index, 
                            (gamepad_pcm_out_t*)packet->payload);
        }
        ogxm_logd("SPI: Gamepad %d PCM data received\n", packet->index);
        break;
    case WIRED_REPORT_ID_RUMBLE:
        if (spi_state.rumble_cb != NULL) {
            spi_state.rumble_cb(packet->index, 
                                (gamepad_rumble_t*)packet->payload);
        }
        ogxm_logd("SPI: Gamepad %d rumble set\n", packet->index);
        break;
    default:
        ogxm_logd_hex(packet, WIRED_MAX_SIZE,
                      "SPI: Received unknown report %02X:", 
                      packet->report_id);
        break;
    }
    memset(spi_state.dma_buf_rx, 0, WIRED_MAX_SIZE);
}

void wired_task(void* args) {
    (void)args;

    spi_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    transmit_blocking();

    while (true) {
        if (gpio_ll_get_level(&GPIO, SPI_PIN_IRQ) == 1) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        if (ring_wired_pop(&spi_state.ring_spi, spi_state.dma_buf_tx)) {
            wired_packet_t* packet = (wired_packet_t*)spi_state.dma_buf_tx;
            if (packet->report_id == WIRED_REPORT_ID_CONNECTED) {
                ogxm_logd("SPI: Gamepad %d connect event, connected=%d\n",
                          packet->index, packet->payload[0]);
            }
            transmit_blocking();
            parse_rx();
        } 
        else {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        } 
        // spi_transmit_blocking();
        vTaskDelay(1);
    }
}

void wired_config(wired_rumble_cb_t rumble_cb, wired_audio_cb_t audio_cb) {
    memset(&spi_state, 0, sizeof(spi_state_t));
    spi_state.rumble_cb = rumble_cb;
    spi_state.audio_cb = audio_cb;
}

void wired_set_connected(uint8_t index, bool connected) {
    ogxm_logd("SPI: Set gamepad %d connected=%d\n", index, connected);
    wired_packet_t* packet = (wired_packet_t*)spi_state.bt_buf_tx;
    packet->report_id = WIRED_REPORT_ID_CONNECTED;
    packet->index = index;
    packet->reserved = 0U;
    packet->payload_len = 2U;
    packet->payload[0] = connected ? 1U : 0U;
    packet->payload[1] = 0U; // Reserved
    ring_wired_push(&spi_state.ring_spi, spi_state.bt_buf_tx, true);
}

void wired_set_pad(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    wired_packet_t* packet = (wired_packet_t*)spi_state.bt_buf_tx;
    packet->report_id = WIRED_REPORT_ID_PAD;
    packet->index = index;
    packet->reserved = 0U;
    packet->payload_len = sizeof(gamepad_pad_t);
    memcpy(packet->payload, pad, sizeof(gamepad_pad_t));
    ring_wired_push(&spi_state.ring_spi, spi_state.bt_buf_tx, false);
}

void wired_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm) {
    wired_packet_t* packet = (wired_packet_t*)spi_state.bt_buf_tx;
    packet->report_id = WIRED_REPORT_ID_PCM;
    packet->index = index;
    packet->reserved = 0U;
    packet->payload_len = sizeof(gamepad_pcm_in_t);
    memcpy(packet->payload, pcm, sizeof(gamepad_pcm_in_t));
    ring_wired_push(&spi_state.ring_spi, spi_state.bt_buf_tx, false);
}

#endif // CONFIG_SPI_ENABLED