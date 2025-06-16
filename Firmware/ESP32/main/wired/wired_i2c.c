#include "sdkconfig.h"
#if defined(CONFIG_I2C_ENABLED)

#include <freertos/FreeRTOS.h>
#include <esp_private/periph_ctrl.h>
#include <esp_rom_gpio.h>
#include <esp_clk_tree.h>
#include <soc/clk_tree_defs.h>
#include <rom/lldesc.h>
#include <hal/gpio_ll.h>
#include <hal/i2c_ll.h>
#include <hal/i2c_hal.h>
#include <esp_timer.h>

#include "log/log.h"
#include "periph/i2c_master.h"
#include "wired/wired.h"
#include "wired/wired_ring.h"

#define I2C_PORT                0
#define I2C_HW                  I2C_LL_GET_HW(I2C_PORT)
#define I2C_PIN_SDA             21
#define I2C_PIN_SCL             22
#define I2C_PIN_IRQ             3
#define I2C_BAUD                (1*1000*1000)
#define I2C_TIMEOUT_US          (10U*1000)
#define I2C_GLITCH_IGNORE_CNT   7

#define SLAVE_ADDR ((uint8_t)0x40)

typedef struct {
    i2c_master_handle_t*    handle;
    ring_wired_t            ring_i2c;
    uint8_t                 bt_buf_tx[WIRED_MAX_SIZE] __attribute__((aligned(4))); /* Belongs to bt thread */
    uint8_t                 i2c_buf_tx[WIRED_MAX_SIZE] __attribute__((aligned(4))); /* Belongs to i2c thread */
    uint8_t                 i2c_buf_rx[WIRED_MAX_SIZE] __attribute__((aligned(4))); /* Belongs to i2c thread */
    wired_rumble_cb_t       rumble_cb;
} i2c_state_t;

static i2c_state_t i2c_state = {0};

static void parse_rx(void) {
    const wired_packet_t* packet = 
        (const wired_packet_t*)i2c_state.i2c_buf_rx;
    switch (packet->report_id) {
    case WIRED_REPORT_ID_RUMBLE:
        if (i2c_state.rumble_cb != NULL) {
            i2c_state.rumble_cb(packet->index, 
                                (gamepad_rumble_t*)packet->payload);
        }
        ogxm_logd("I2C: Gamepad %d rumble set\n", packet->index);
        break;
    default:
        ogxm_logd_hex(packet, WIRED_MAX_SIZE,
                      "I2C: Received unknown report %02X:", 
                      packet->report_id);
        break;
    }
    memset(i2c_state.i2c_buf_rx, 0, WIRED_MAX_SIZE);
}

void wired_task(void* args) {
    (void)args;

    i2c_master_cfg cfg = {
        .port_num = I2C_PORT,
        .freq_hz = I2C_BAUD,
        .pin_sda = I2C_PIN_SDA,
        .pin_scl = I2C_PIN_SCL,
        .lsb_first = false,
        .pullup_en = true,
        .glitch_ignore_cnt = I2C_GLITCH_IGNORE_CNT,
    };
    
    i2c_master_handle_t* handle = i2c_master_init(&cfg);
    if (handle == NULL) {
        ogxm_loge("I2C: Failed to initialize I2C\n");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    gpio_ll_func_sel(&GPIO, I2C_PIN_IRQ, FUNC_GPIO5_GPIO5);
    gpio_ll_input_enable(&GPIO, I2C_PIN_IRQ);
    gpio_ll_pullup_en(&GPIO, I2C_PIN_IRQ);

    vTaskDelay(pdMS_TO_TICKS(1000));

    while (true) {
        if ((gpio_ll_get_level(&GPIO, I2C_PIN_IRQ) == 0)) {
            if (i2c_master_read_blocking(handle, SLAVE_ADDR, i2c_state.i2c_buf_rx, 
                                         WIRED_MAX_SIZE, I2C_TIMEOUT_US)) {
                parse_rx();
            } else {
                ogxm_loge("I2C: Failed to read from slave\n");
            }
        }
        if (ring_wired_pop(&i2c_state.ring_i2c, i2c_state.i2c_buf_tx)) {
            wired_packet_t* packet = (wired_packet_t*)i2c_state.i2c_buf_tx;
            if (packet->report_id == WIRED_REPORT_ID_CONNECTED) {
                ogxm_logd("I2C: Gamepad %d connect event, connected=%d\n",
                          packet->index, packet->payload[0]);
            }
            i2c_master_write_blocking(handle, SLAVE_ADDR, i2c_state.i2c_buf_tx, 
                                      WIRED_MAX_SIZE, I2C_TIMEOUT_US);
        }
        vTaskDelay(1);
    }
}

void wired_config(wired_rumble_cb_t rumble_cb, wired_audio_cb_t audio_cb) {
    (void)audio_cb;
    memset(&i2c_state, 0, sizeof(i2c_state_t));
    i2c_state.rumble_cb = rumble_cb;
}

void wired_set_connected(uint8_t index, bool connected) {
    ogxm_logd("I2C: Set gamepad %d connected=%d\n", index, connected);
    wired_packet_t* packet = (wired_packet_t*)i2c_state.bt_buf_tx;
    packet->report_id = WIRED_REPORT_ID_CONNECTED;
    packet->index = index;
    packet->reserved = 0U;
    packet->payload_len = 2U;
    packet->payload[0] = connected ? 1U : 0U;
    packet->payload[1] = 0U; // Reserved
    ring_wired_push(&i2c_state.ring_i2c, i2c_state.bt_buf_tx, true);
}

void wired_set_pad(uint8_t index, const gamepad_pad_t* pad, uint32_t flags) {
    wired_packet_t* packet = (wired_packet_t*)i2c_state.bt_buf_tx;
    packet->report_id = WIRED_REPORT_ID_PAD;
    packet->index = index;
    packet->reserved = 0U;
    packet->payload_len = sizeof(gamepad_pad_t);
    memcpy(packet->payload, pad, sizeof(gamepad_pad_t));
    ring_wired_push(&i2c_state.ring_i2c, i2c_state.bt_buf_tx, false);
}

void wired_set_audio(uint8_t index, const gamepad_pcm_in_t* pcm) {
    (void)index;
    (void)pcm;
}

#endif // CONFIG_I2C_ENABLED