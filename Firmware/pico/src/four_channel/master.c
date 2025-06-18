#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL)

#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/i2c.h>
#include "log/log.h"
#include "four_channel/ring.h"
#include "four_channel/master.h"

#define I2C_HW                      __CONCAT(i2c, FOUR_CHANNEL_I2C_NUM)
#define I2C_BAUD                    ((uint32_t)400*1000)
#define IRQ_PIN_TO_SLAVE_ADDR(pin)  (((pin) == FOUR_CHANNEL_I2C_PIN_IRQ_1) ? ((uint8_t)0x41) : \
                                     ((pin) == FOUR_CHANNEL_I2C_PIN_IRQ_2) ? ((uint8_t)0x42) : \
                                     ((pin) == FOUR_CHANNEL_I2C_PIN_IRQ_3) ? ((uint8_t)0x43) : ((uint8_t)0xFF))
#define INDEX_TO_SLAVE_ADDR(index)  ((uint8_t)0x40 | (index))
#define INDEX_TO_IRQ_PIN(index)     (((index) == 1) ? FOUR_CHANNEL_I2C_PIN_IRQ_1 : \
                                     ((index) == 2) ? FOUR_CHANNEL_I2C_PIN_IRQ_2 : \
                                     ((index) == 3) ? FOUR_CHANNEL_I2C_PIN_IRQ_3 : 0xFF)
#define SLAVE_TIMEOUT_MS            5U

typedef struct {
    ring_four_ch_t ring;
    uint8_t buffer[FOUR_CH_MAX_SIZE] __attribute__((aligned(4)));
    gamepad_set_rumble_cb_t rumble_cb;
} four_ch_master_t;

static four_ch_master_t four_ch_master = {0};

void four_ch_master_init(gamepad_set_rumble_cb_t rumble_cb) {
    i2c_init(I2C_HW, I2C_BAUD);
    gpio_set_function(FOUR_CHANNEL_I2C_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(FOUR_CHANNEL_I2C_PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(FOUR_CHANNEL_I2C_PIN_SDA);
    gpio_pull_up(FOUR_CHANNEL_I2C_PIN_SCL);

    gpio_init(FOUR_CHANNEL_I2C_PIN_IRQ_1);
    gpio_init(FOUR_CHANNEL_I2C_PIN_IRQ_2);
    gpio_init(FOUR_CHANNEL_I2C_PIN_IRQ_3);
    gpio_set_dir(FOUR_CHANNEL_I2C_PIN_IRQ_1, GPIO_IN);
    gpio_set_dir(FOUR_CHANNEL_I2C_PIN_IRQ_2, GPIO_IN);
    gpio_set_dir(FOUR_CHANNEL_I2C_PIN_IRQ_3, GPIO_IN);
    gpio_pull_up(FOUR_CHANNEL_I2C_PIN_IRQ_1);
    gpio_pull_up(FOUR_CHANNEL_I2C_PIN_IRQ_2);
    gpio_pull_up(FOUR_CHANNEL_I2C_PIN_IRQ_3);

    four_ch_master.rumble_cb = rumble_cb;
}

void four_ch_master_task(void) {
    for (uint8_t i = 1; i < GAMEPADS_MAX; i++) {
        if (gpio_get(INDEX_TO_IRQ_PIN(i)) == 0) {
            ogxm_logd("IRQ from slave %d\n", i);
            uint8_t addr = INDEX_TO_SLAVE_ADDR(i);
            absolute_time_t timeout = make_timeout_time_ms(SLAVE_TIMEOUT_MS);
            int ret = i2c_read_blocking_until(I2C_HW, addr, four_ch_master.buffer, 
                                              FOUR_CH_MAX_SIZE, false, timeout);
            if (ret == FOUR_CH_MAX_SIZE) {
                four_ch_packet_t* packet = (four_ch_packet_t*)four_ch_master.buffer;
                switch (packet->report_id) {
                    // case FOURCH_REPORT_ID_ENABLED:
                    //     four_ch_master.enabled[packet->index] = (packet->payload[0] != 0);
                    //     break;
                    case FOURCH_REPORT_ID_RUMBLE:
                        if (four_ch_master.rumble_cb) {
                            four_ch_master.rumble_cb(packet->index, (gamepad_rumble_t*)packet->payload);
                        }
                        break;
                    default:
                        break;
                }
            } else {
                ogxm_loge("Failed to read packet from slave %d, ret=%d\n", i, ret);
            }
        }
    }
    if (ring_four_ch_pop(&four_ch_master.ring, four_ch_master.buffer)) {
        four_ch_packet_t* packet = (four_ch_packet_t*)four_ch_master.buffer;
        uint8_t addr = INDEX_TO_SLAVE_ADDR(packet->index);
        absolute_time_t timeout = make_timeout_time_ms(SLAVE_TIMEOUT_MS);
        int ret = i2c_write_blocking_until(I2C_HW, addr, four_ch_master.buffer, 
                                           sizeof(four_ch_packet_t) + packet->payload_len, 
                                           false, timeout);
        if (ret != (sizeof(four_ch_packet_t) + packet->payload_len)) {
            ogxm_loge("Failed to send packet to slave %d, ret=%d\n", packet->index, ret);
        }
    }
}

void four_ch_master_set_pad(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    if ((index < 1) || (index >= GAMEPADS_MAX)) {
        return;
    }
    four_ch_packet_t packet = {
        .report_id = FOURCH_REPORT_ID_PAD,
        .index = index,
        .flags = flags,
        .payload_len = sizeof(gamepad_pad_t)
    };
    ring_four_ch_push(&four_ch_master.ring, &packet, pad);
}

void four_ch_master_set_connected(uint8_t index, bool connected) {
    if ((index < 1) || (index >= GAMEPADS_MAX)) {
        ogxm_loge("Invalid index %d for four channel master\n", index);
        return;
    }
    uint8_t buf[2] = { connected ? 1 : 0, 0 };
    four_ch_packet_t packet = {
        .report_id = FOURCH_REPORT_ID_ENABLED,
        .index = index,
        .flags = 0,
        .payload_len = sizeof(buf)
    };
    ring_four_ch_push(&four_ch_master.ring, &packet, buf);
    ogxm_logd("Four channel master set connected: index=%d, connected=%d\n", 
              index, connected);
}

#endif // OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL