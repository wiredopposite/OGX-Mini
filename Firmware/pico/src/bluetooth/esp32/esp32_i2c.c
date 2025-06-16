#include "board_config.h"
#if BLUETOOTH_ENABLED 
#if (BLUETOOTH_HARDWARE == BLUETOOTH_HARDWARE_ESP32_I2C)

#include <pico/stdlib.h>
#include <pico/i2c_slave.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include "log/log.h"
#include "led/led.h"
#include "bluetooth/esp32/wired_ring.h"
#include "bluetooth/bluetooth.h"

#define ESP32_I2C_PORT          __CONCAT(i2c, ESP32_I2C_NUM)
#define ESP32_I2C_BAUD          (1U*1000U*1000U)
#define I2C_ADDR                ((uint8_t)0x40)
#define I2C_SHORT_PACKET_LEN    (28U)

typedef enum {
    XFER_IDLE = 0,
    XFER_RX,
    XFER_TX,
} xfer_state_t;

typedef struct {
    volatile bool           connected[GAMEPADS_MAX];
    uint8_t                 rx[WIRED_MAX_SIZE] __attribute__((aligned(4)));
    uint8_t                 tx[WIRED_MAX_SIZE] __attribute__((aligned(4)));
    uint8_t                 mt_tx[WIRED_MAX_SIZE] __attribute__((aligned(4)));
    ring_wired_t            ring_wired;
    bt_connect_cb_t         connect_cb;
    bt_gamepad_cb_t         gamepad_cb;
    volatile xfer_state_t   xfer_state;
    volatile size_t         xfer_idx;
    volatile bool           tx_pending;
} i2c_state_t;

static i2c_state_t i2c_state = {0};

static void __not_in_flash_func(i2c_slave_xfer)(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE:
        i2c_state.xfer_state = XFER_RX;
        if (i2c_state.xfer_idx < WIRED_MAX_SIZE) {
            i2c_state.rx[i2c_state.xfer_idx++] = i2c_read_byte_raw(i2c);
        } else {
            (void)i2c_read_byte_raw(i2c);
        }
        break;
    case I2C_SLAVE_REQUEST:
        if (i2c_state.tx_pending && 
            (i2c_state.xfer_idx < WIRED_MAX_SIZE)) {
            i2c_state.xfer_state = XFER_TX;
            i2c_write_byte_raw(i2c, i2c_state.tx[i2c_state.xfer_idx++]);
        } else {
            i2c_write_byte_raw(i2c, 0);
        }
        break;
    case I2C_SLAVE_FINISH:
        i2c_state.xfer_idx = 0;

        switch (i2c_state.xfer_state) {
        case XFER_RX:
            {
            const wired_packet_t* packet = (const wired_packet_t*)i2c_state.rx;
            switch (packet->report_id) {
            case WIRED_REPORT_ID_CONNECT:
                if ((packet->index < GAMEPADS_MAX)) {
                    i2c_state.connected[packet->index] = (packet->payload[0] != 0);
                    led_set_on(i2c_state.connected[packet->index]);
                    if (i2c_state.connect_cb) {
                        i2c_state.connect_cb(packet->index, 
                                             i2c_state.connected[packet->index]);
                    }
                }
                ogxm_logd("I2C: Gamepad %d connected: %d\n", packet->index, packet->payload[0]);
                break;
            case WIRED_REPORT_ID_PAD:
                if ((packet->index < GAMEPADS_MAX) && 
                    (i2c_state.gamepad_cb != NULL)) {
                    i2c_state.gamepad_cb(packet->index, 
                                         (gamepad_pad_t*)packet->payload, 
                                         GAMEPAD_FLAG_IN_PAD);
                }
                ogxm_logv("I2C: Gamepad %d pad\n", packet->index);
                break;
            default:
                ogxm_logd_hex(i2c_state.rx, WIRED_MAX_SIZE, "I2C: Unknown RX packet: ");
                break;
            }
            }
            break;
        case XFER_TX:
            gpio_put(ESP32_I2C_PIN_IRQ, 1);
            i2c_state.tx_pending = false;
            break;
        default:
            break;
        }
        i2c_state.xfer_state = XFER_IDLE;
        break;
    default:
        break;
    }
}

void bluetooth_task(void) {
    if (!i2c_state.tx_pending && 
        ring_wired_pop(&i2c_state.ring_wired, i2c_state.tx)) {
        ogxm_logd("I2C: Rumble queued\n");
        i2c_state.tx_pending = true;
        gpio_put(ESP32_I2C_PIN_IRQ, 0);
    }
}

bool bluetooth_init(bt_connect_cb_t connect_cb, bt_gamepad_cb_t gamepad_cb, bt_audio_cb_t audio_cb) {
    led_init();
    
    memset(&i2c_state, 0, sizeof(i2c_state_t));
    i2c_state.connect_cb = connect_cb;
    i2c_state.gamepad_cb = gamepad_cb;

    i2c_init(ESP32_I2C_PORT, ESP32_I2C_BAUD);
    i2c_set_slave_mode(ESP32_I2C_PORT, true, I2C_ADDR);
    gpio_set_function(ESP32_I2C_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(ESP32_I2C_PIN_SCL, GPIO_FUNC_I2C);

    gpio_init(ESP32_I2C_PIN_IRQ);
    gpio_set_dir(ESP32_I2C_PIN_IRQ, GPIO_OUT);
    gpio_put(ESP32_I2C_PIN_IRQ, 1);

    gpio_init(ESP32_RESET_PIN);
    gpio_set_dir(ESP32_RESET_PIN, GPIO_OUT);
    gpio_put(ESP32_RESET_PIN, 0);
    sleep_ms(100);
    gpio_put(ESP32_RESET_PIN, 1);

    i2c_slave_init(ESP32_I2C_PORT, I2C_ADDR, &i2c_slave_xfer);
    return true;
}

void bluetooth_set_rumble(uint8_t index, const gamepad_rumble_t* rumble) {
    if (i2c_state.connected[index]) {
        wired_packet_t* packet = (wired_packet_t*)i2c_state.mt_tx;
        packet->report_id = WIRED_REPORT_ID_RUMBLE;
        packet->index = index;
        packet->reserved = 0;
        packet->payload_len = sizeof(gamepad_rumble_t);
        memcpy(packet->payload, rumble, sizeof(gamepad_rumble_t));
        ring_wired_push(&i2c_state.ring_wired, i2c_state.mt_tx);
    }
}

void bluetooth_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm) {
    (void)index;
    (void)pcm;
}

#endif // BLUETOOTH_HARDWARE_ESP32_SPI
#endif // BLUETOOTH_ENABLED