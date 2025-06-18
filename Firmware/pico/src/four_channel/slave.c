#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL)

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <pico/i2c_slave.h>
#include "four_channel/ring.h"
#include "four_channel/slave.h"

#define I2C_HW                      __CONCAT(i2c, FOUR_CHANNEL_I2C_NUM)
#define I2C_BAUD                    ((uint32_t)400*1000)
#define INDEX_TO_SLAVE_ADDR(index)  ((uint8_t)0x40 | (index))

typedef enum {
    XFER_IDLE = 0,
    XFER_RX,
    XFER_TX,
} xfer_state_t;

typedef struct {
    uint8_t                 index;
    uint8_t                 buf_rx[FOUR_CH_MAX_SIZE] __attribute__((aligned(4)));
    uint8_t                 buf_tx[FOUR_CH_MAX_SIZE] __attribute__((aligned(4)));
    volatile bool           tx_pending;
    volatile uint8_t        xfer_idx;
    xfer_state_t            xfer_state;
    gamepad_set_pad_cb_t    gamepad_cb;
    four_ch_slave_en_cb_t   enabled_cb;
    ring_four_ch_t          ring;
} four_ch_slave_t;

static four_ch_slave_t four_ch_slave = {0};

static void __not_in_flash_func(four_ch_slave_cb)(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE:
        four_ch_slave.xfer_state = XFER_RX;
        if (four_ch_slave.xfer_idx < FOUR_CH_MAX_SIZE) {
            four_ch_slave.buf_rx[four_ch_slave.xfer_idx++] = i2c_read_byte_raw(i2c);
        } else {
            (void)i2c_read_byte_raw(i2c);
        }
        break;
    case I2C_SLAVE_REQUEST:
        if (four_ch_slave.tx_pending && 
            (four_ch_slave.xfer_idx < FOUR_CH_MAX_SIZE)) {
            four_ch_slave.xfer_state = XFER_TX;
            i2c_write_byte_raw(i2c, four_ch_slave.buf_tx[four_ch_slave.xfer_idx++]);
        } else {
            i2c_write_byte_raw(i2c, 0);
        }
        break;
    case I2C_SLAVE_FINISH:
        if (four_ch_slave.xfer_state == XFER_RX) {
            four_ch_packet_t* packet = (four_ch_packet_t*)four_ch_slave.buf_rx;
            switch (packet->report_id) {
            case FOURCH_REPORT_ID_PAD:
                if (four_ch_slave.gamepad_cb) {
                    four_ch_slave.gamepad_cb(0, (gamepad_pad_t*)packet->payload, packet->flags);
                }
                break;
            case FOURCH_REPORT_ID_ENABLED:
                if (four_ch_slave.enabled_cb) {
                    four_ch_slave.enabled_cb(packet->payload[0] != 0);
                }
                break;
            default:
                break;
            }
        } else if (four_ch_slave.xfer_state == XFER_TX) {
            gpio_put(FOUR_CHANNEL_I2C_PIN_IRQ_1, 1);
            four_ch_slave.tx_pending = false;
        }
        four_ch_slave.xfer_idx = 0;
        four_ch_slave.xfer_state = XFER_IDLE;
        break;
    }
}

void four_ch_slave_init(uint8_t index, four_ch_slave_en_cb_t enabled_cb, gamepad_set_pad_cb_t gamepad_cb) {
    i2c_init(I2C_HW, I2C_BAUD);
    i2c_set_slave_mode(I2C_HW, true, INDEX_TO_SLAVE_ADDR(index));
    gpio_set_function(FOUR_CHANNEL_I2C_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(FOUR_CHANNEL_I2C_PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(FOUR_CHANNEL_I2C_PIN_SDA);
    gpio_pull_up(FOUR_CHANNEL_I2C_PIN_SCL);

    gpio_init(FOUR_CHANNEL_I2C_PIN_IRQ_1);
    gpio_set_dir(FOUR_CHANNEL_I2C_PIN_IRQ_1, GPIO_OUT);
    gpio_put(FOUR_CHANNEL_I2C_PIN_IRQ_1, true);

    four_ch_slave.gamepad_cb = gamepad_cb;
    four_ch_slave.enabled_cb = enabled_cb;
    four_ch_slave.index = index;

    i2c_slave_init(I2C_HW, INDEX_TO_SLAVE_ADDR(index), four_ch_slave_cb);
    // four_ch_slave_set_enabled(true);
}

void four_ch_slave_task(void) {
    if (!four_ch_slave.tx_pending &&
        ring_four_ch_pop(&four_ch_slave.ring, four_ch_slave.buf_tx)) {
        four_ch_slave.tx_pending = true;
        gpio_put(FOUR_CHANNEL_I2C_PIN_IRQ_1, 0);
    }
}

void four_ch_slave_set_rumble(const gamepad_rumble_t* rumble) {
    four_ch_packet_t packet = {
        .report_id = FOURCH_REPORT_ID_RUMBLE,
        .index = four_ch_slave.index,
        .flags = 0,
        .payload_len = sizeof(gamepad_rumble_t)
    };
    ring_four_ch_push(&four_ch_slave.ring, &packet, rumble);
}

#endif // OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL