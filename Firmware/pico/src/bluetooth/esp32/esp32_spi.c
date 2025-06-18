#include "board_config.h"
#if BLUETOOTH_ENABLED 
#if (BLUETOOTH_HARDWARE == BLUETOOTH_HARDWARE_ESP32_SPI)

#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
// #include <hardware/dma.h>
#include "log/log.h"
#include "led/led.h"
#include "bluetooth/esp32/wired_ring.h"
#include "bluetooth/bluetooth.h"

#define ESP32_SPI_PORT __CONCAT(spi, ESP32_SPI_NUM)
#define SPI_FIFO_DEPTH ((size_t)8U)

typedef struct {
    volatile bool           connected[GAMEPADS_MAX];
    uint8_t                 rx[WIRED_MAX_SIZE] __attribute__((aligned(4)));
    uint8_t                 tx[WIRED_MAX_SIZE] __attribute__((aligned(4)));
    uint8_t                 mt_tx[WIRED_MAX_SIZE] __attribute__((aligned(4)));
    ring_wired_t            ring_wired;
    gamepad_connect_cb_t    connect_cb;
    gamepad_pad_cb_t        gamepad_cb;
    gamepad_pcm_cb_t        audio_cb;
} spi_state_t;

static spi_state_t spi_state = {0};

int __not_in_flash_func(spi_slave_xfer)(const uint8_t* src, uint8_t* dst, size_t len) {
    size_t rx_remaining = len, tx_remaining = len;
    while (tx_remaining && spi_is_writable(ESP32_SPI_PORT) && 
           (rx_remaining < (tx_remaining + SPI_FIFO_DEPTH))) {
        spi_get_hw(ESP32_SPI_PORT)->dr = (uint32_t)*src++;
        --tx_remaining;
    }
    ogxm_logd("Start Xfer...\n");
    gpio_put(ESP32_SPI_PIN_IRQ, 0);
    while (rx_remaining || tx_remaining) {
        if (tx_remaining && spi_is_writable(ESP32_SPI_PORT) && 
            (rx_remaining < (tx_remaining + SPI_FIFO_DEPTH))) {
            spi_get_hw(ESP32_SPI_PORT)->dr = (uint32_t)*src++;
            --tx_remaining;
        }
        if (rx_remaining && spi_is_readable(ESP32_SPI_PORT)) {
            *dst++ = (uint8_t) spi_get_hw(ESP32_SPI_PORT)->dr;
            --rx_remaining;
        }
    }
    
    gpio_put(ESP32_SPI_PIN_IRQ, 1);
    ogxm_logd("Xfer complete\n");
    return (int)len;
}

void bluetooth_task(void) {
    if (!ring_wired_pop(&spi_state.ring_wired, spi_state.tx)) {
        memset(spi_state.tx, 0, sizeof(spi_state.tx));
    }
    int len = spi_slave_xfer(spi_state.tx, spi_state.rx, WIRED_MAX_SIZE);
    ogxm_logd("SPI transfer: %d bytes\n", len);

    const wired_packet_t* packet = (const wired_packet_t*)spi_state.rx;
    switch (packet->report_id) {
    case WIRED_REPORT_ID_CONNECT:
        if ((packet->index < GAMEPADS_MAX)) {
            spi_state.connected[packet->index] = (packet->payload[0] != 0);
            led_set_on(spi_state.connected[packet->index]);
            if (spi_state.connect_cb) {
                spi_state.connect_cb(packet->index, (packet->payload[0] != 0));
            }
        }
        ogxm_logd("Gamepad %d connected: %d\n", packet->index, packet->payload[0]);
        break;
    case WIRED_REPORT_ID_PAD:
        if ((packet->index < GAMEPADS_MAX) && (spi_state.gamepad_cb) && 
            (packet->payload_len == sizeof(gamepad_pad_t))) {
            spi_state.gamepad_cb(packet->index, (gamepad_pad_t*)packet->payload, 
                                    GAMEPAD_FLAG_IN_PAD);
        }
        ogxm_logd("Gamepad %d pad\n", packet->index);
        ogxm_logd_hex(spi_state.rx, WIRED_MAX_SIZE, "Pad data: ");
        break;
    case WIRED_REPORT_ID_PCM:
        if ((packet->index < GAMEPADS_MAX) && (spi_state.audio_cb) && 
            (packet->payload_len == sizeof(gamepad_pcm_in_t))) {
            spi_state.audio_cb(packet->index, (gamepad_pcm_in_t*)packet->payload);
        }
        ogxm_logd("Gamepad %d audio\n", packet->index);
        break;
    default:
        ogxm_logd_hex(spi_state.rx, WIRED_MAX_SIZE, "Unknown RX packet: ");
        break;
    }
}

bool bluetooth_init(gamepad_connect_cb_t connect_cb, gamepad_pad_cb_t gamepad_cb, gamepad_pcm_cb_t pcm_cb) {
    led_init();
    
    memset(&spi_state, 0, sizeof(spi_state_t));
    spi_state.connect_cb = connect_cb;
    spi_state.gamepad_cb = gamepad_cb;
    spi_state.audio_cb = audio_cb;

    spi_init(ESP32_SPI_PORT, 4*1000*1000);
    spi_set_slave(ESP32_SPI_PORT, true);
    spi_set_format(ESP32_SPI_PORT,
                   8,
                   SPI_CPOL_1,
                   SPI_CPHA_1,
                   SPI_MSB_FIRST); 
    gpio_set_function(ESP32_SPI_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ESP32_SPI_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ESP32_SPI_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(ESP32_SPI_PIN_CS, GPIO_FUNC_SPI);

    gpio_init(ESP32_SPI_PIN_IRQ);
    gpio_set_dir(ESP32_SPI_PIN_IRQ, GPIO_OUT);
    gpio_put(ESP32_SPI_PIN_IRQ, 1);

    // Reset the ESP32
    gpio_init(ESP32_RESET_PIN);
    gpio_set_dir(ESP32_RESET_PIN, GPIO_OUT);
    gpio_put(ESP32_RESET_PIN, 0);
    sleep_ms(100);
    gpio_put(ESP32_RESET_PIN, 1);

    return true;
}

void bluetooth_set_rumble(uint8_t index, const gamepad_rumble_t* rumble) {
    if (spi_state.connected[index]) {
        wired_packet_t* packet = (wired_packet_t*)spi_state.mt_tx;
        packet->report_id = WIRED_REPORT_ID_RUMBLE;
        packet->index = index;
        packet->reserved = 0;
        packet->payload_len = sizeof(gamepad_rumble_t);
        memcpy(packet->payload, rumble, sizeof(gamepad_rumble_t));
        ring_wired_push(&spi_state.ring_wired, spi_state.mt_tx);
    }
}

void bluetooth_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm) {
    if (spi_state.connected[index]) {
        wired_packet_t* packet = (wired_packet_t*)spi_state.mt_tx;
        packet->report_id = WIRED_REPORT_ID_PCM;
        packet->index = index;
        packet->reserved = 0;
        packet->payload_len = sizeof(gamepad_pcm_in_t);
        memcpy(packet->payload, pcm, sizeof(gamepad_pcm_in_t));
        ring_wired_push(&spi_state.ring_wired, spi_state.mt_tx);
    }
}

#endif // BLUETOOTH_HARDWARE_ESP32_SPI
#endif // BLUETOOTH_ENABLED