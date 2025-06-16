#include <string.h>
#include <esp_private/periph_ctrl.h>
#include <esp_rom_gpio.h>
#include <esp_clk_tree.h>
#include <esp_intr_alloc.h>
#include <soc/clk_tree_defs.h>
#include <rom/lldesc.h>
#include <hal/gpio_ll.h>
#include <hal/i2c_ll.h>
#include <hal/i2c_hal.h>
#include <esp_rom_sys.h>
#include <esp_timer.h>

#include "log/log.h"
#include "periph/i2c_master.h"

typedef struct i2c_master_handle_ {
    bool                alloc;
    int                 port;
    int                 freq_hz;
    int                 pin_sda;
    int                 pin_scl;
    bool                lsb_first;
    bool                pullup_en;
    int                 glitch_ignore_cnt;
    i2c_dev_t*          dev;
    uint32_t            clk_src_hz;
    volatile bool       xfer_done;
    i2c_intr_event_t    xfer_result; 
} i2c_master_handle_t;

static const uint32_t IO_MUX_REGS[40] = {
    IO_MUX_GPIO0_REG, IO_MUX_GPIO1_REG, IO_MUX_GPIO2_REG, IO_MUX_GPIO3_REG,
    IO_MUX_GPIO4_REG, IO_MUX_GPIO5_REG, IO_MUX_GPIO6_REG, IO_MUX_GPIO7_REG,
    IO_MUX_GPIO8_REG, IO_MUX_GPIO9_REG, IO_MUX_GPIO10_REG, IO_MUX_GPIO11_REG,
    IO_MUX_GPIO12_REG, IO_MUX_GPIO13_REG, IO_MUX_GPIO14_REG, IO_MUX_GPIO15_REG,
    IO_MUX_GPIO16_REG, IO_MUX_GPIO17_REG, IO_MUX_GPIO18_REG, IO_MUX_GPIO19_REG,
    IO_MUX_GPIO20_REG, IO_MUX_GPIO21_REG, IO_MUX_GPIO22_REG, IO_MUX_GPIO23_REG,
    IO_MUX_GPIO24_REG, IO_MUX_GPIO25_REG, IO_MUX_GPIO26_REG, IO_MUX_GPIO27_REG,
    0, 0, 0, 0,
    IO_MUX_GPIO32_REG, IO_MUX_GPIO33_REG, IO_MUX_GPIO34_REG, IO_MUX_GPIO35_REG,
    IO_MUX_GPIO36_REG, IO_MUX_GPIO37_REG, IO_MUX_GPIO38_REG, IO_MUX_GPIO39_REG,
};

static i2c_master_handle_t i2c_handles[2] = {0};

static void i2c_master_config_gpio(i2c_master_handle_t* handle);
static void i2c_master_config_bus(i2c_master_handle_t* handle);
static void i2c_master_isr_init(i2c_master_handle_t* handle);
void i2c_master_isr_enable(i2c_dev_t* dev, bool enable);
void i2c_master_isr_handler(void* arg);
static void i2c_master_clear_bus(i2c_master_handle_t* handle);
static inline void i2c_master_cmd_start(i2c_dev_t* dev, int* cmd_idx);
static inline void i2c_master_cmd_stop(i2c_dev_t* dev, int* cmd_idx);
static inline void i2c_master_cmd_end(i2c_dev_t* dev, int* cmd_idx);
static inline void i2c_master_cmd_write(i2c_dev_t* dev, int* cmd_idx, uint8_t* data, uint8_t len);
static inline void i2c_master_cmd_read(i2c_dev_t* dev, int* cmd_idx, uint8_t len, bool last);
static inline i2c_intr_event_t i2c_master_xfer_blocking(i2c_master_handle_t* handle, int cmd_idx, uint32_t timeout_us);
static inline bool i2c_master_err_check(i2c_master_handle_t* handle, i2c_intr_event_t event);

i2c_master_handle_t* i2c_master_init(const i2c_master_cfg* cfg) {
    if (cfg == NULL) {
        ogxm_loge("I2C: Invalid config\n");
        return NULL;
    }
    if (cfg->port_num >= 2) {
        ogxm_loge("I2C: Invalid port number %d\n", 
            cfg->port_num);
        return NULL;
    }
    if (i2c_handles[cfg->port_num].alloc) {
        ogxm_loge("I2C: Port %d already initialized\n", 
            cfg->port_num);
        return NULL;
    }
    if ((cfg->pin_sda >= 40) || 
        (cfg->pin_scl >= 40) ||
        (IO_MUX_REGS[cfg->pin_sda] == 0) || 
        (IO_MUX_REGS[cfg->pin_scl] == 0)) {
        ogxm_loge("I2C: Invalid pin number sda=%d scl=%d\n", 
            cfg->pin_sda, cfg->pin_scl);
        return NULL;
    }

    i2c_master_handle_t* handle = &i2c_handles[cfg->port_num];
    memset(handle, 0, sizeof(i2c_master_handle_t));
    handle->alloc = true;
    handle->port = cfg->port_num;
    handle->dev = I2C_LL_GET_HW(handle->port);
    handle->freq_hz = cfg->freq_hz;
    handle->pin_sda = cfg->pin_sda;
    handle->pin_scl = cfg->pin_scl;
    handle->lsb_first = cfg->lsb_first;
    handle->pullup_en = cfg->pullup_en;
    handle->glitch_ignore_cnt = cfg->glitch_ignore_cnt;

    i2c_master_config_gpio(handle);
    i2c_master_config_bus(handle);
    i2c_master_isr_init(handle);
    return handle;
}

bool i2c_master_write_blocking(i2c_master_handle_t* handle, uint8_t addr, 
                               uint8_t* data, size_t len, uint32_t timeout_us) {
    if ((handle == NULL) || ((data == NULL) && (len > 0)) || !handle->alloc) {
        ogxm_loge("I2C: Invalid write params\n");
        return false;
    }
    i2c_ll_txfifo_rst(handle->dev);

    uint8_t addr7 = (addr << 1) | 0;
    int cmd_idx = 0;
    i2c_master_cmd_start(handle->dev, &cmd_idx);
    i2c_master_cmd_write(handle->dev, &cmd_idx, &addr7, 1);

    uint32_t offset = 0;
    uint32_t txfifo_free = 0;
    i2c_ll_get_txfifo_len(handle->dev, &txfifo_free);

    while (len > txfifo_free) {
        i2c_master_cmd_write(handle->dev, &cmd_idx, 
                             data + offset, txfifo_free);
        i2c_master_cmd_end(handle->dev, &cmd_idx);
        i2c_intr_event_t result = i2c_master_xfer_blocking(handle, cmd_idx, timeout_us);
        if (!i2c_master_err_check(handle, result)) {
            return false;
        }

        offset += txfifo_free;
        len -= txfifo_free;

        i2c_ll_txfifo_rst(handle->dev);
        i2c_ll_get_txfifo_len(handle->dev, &txfifo_free);
    }

    if (len) {
        i2c_master_cmd_write(handle->dev, &cmd_idx, data + offset, len);
    }
    i2c_master_cmd_stop(handle->dev, &cmd_idx);
    i2c_intr_event_t result = i2c_master_xfer_blocking(handle, cmd_idx, timeout_us);
    return i2c_master_err_check(handle, result);
}

bool i2c_master_read_blocking(i2c_master_handle_t* handle, uint8_t addr, 
                              uint8_t* data, size_t len, uint32_t timeout_us) {
    if ((handle == NULL) || ((data == NULL) && (len > 0)) || !handle->alloc) {
        ogxm_loge("I2C: Invalid read params\n");
        return false;
    }
    i2c_ll_txfifo_rst(handle->dev);
    i2c_ll_rxfifo_rst(handle->dev);

    uint8_t addr7 = (addr << 1) | 1;
    int cmd_idx = 0;

    i2c_master_cmd_start(handle->dev, &cmd_idx);
    i2c_master_cmd_write(handle->dev, &cmd_idx, &addr7, 1);

    uint32_t offset = 0;

    while (len > SOC_I2C_FIFO_LEN) {
        i2c_master_cmd_read(handle->dev, &cmd_idx, 
                            SOC_I2C_FIFO_LEN, false);
        i2c_master_cmd_end(handle->dev, &cmd_idx);
        i2c_intr_event_t result = i2c_master_xfer_blocking(handle, cmd_idx, timeout_us);
        if (!i2c_master_err_check(handle, result)) {
            return false;
        }
        i2c_ll_read_rxfifo(handle->dev, data + offset, len);
        i2c_ll_rxfifo_rst(handle->dev);

        offset += SOC_I2C_FIFO_LEN;
        len -= SOC_I2C_FIFO_LEN;
    }

    if (len) {
        i2c_master_cmd_read(handle->dev, &cmd_idx, len, true);
    }

    i2c_master_cmd_stop(handle->dev, &cmd_idx);
    i2c_intr_event_t result = i2c_master_xfer_blocking(handle, cmd_idx, timeout_us);
    if (!i2c_master_err_check(handle, result)) {
        return false;
    }
    i2c_ll_read_rxfifo(handle->dev, data + offset, len);
    return true;
}

void i2c_master_deinit(i2c_master_handle_t* handle) {
    if ((handle == NULL) || !handle->alloc) {
        ogxm_loge("I2C: Invalid handle\n");
        return;
    }
    i2c_master_isr_enable(handle->dev, false);
    i2c_ll_enable_controller_clock(handle->dev, false);

    PERIPH_RCC_ATOMIC() {
        i2c_ll_reset_register(handle->port);
        i2c_ll_enable_bus_clock(handle->port, false);
    }
    i2c_master_clear_bus(handle);
    memset(handle, 0, sizeof(i2c_master_handle_t));
}

static void i2c_master_config_gpio(i2c_master_handle_t* handle) {
    gpio_ll_input_enable(&GPIO, handle->pin_sda);
    gpio_ll_input_enable(&GPIO, handle->pin_scl);
    gpio_ll_od_enable(&GPIO, handle->pin_sda);
    gpio_ll_od_enable(&GPIO, handle->pin_scl);

    if (handle->pullup_en) {
        gpio_ll_pullup_en(&GPIO, handle->pin_sda);
        gpio_ll_pullup_en(&GPIO, handle->pin_scl);
    } else {
        gpio_ll_pullup_dis(&GPIO, handle->pin_sda);
        gpio_ll_pullup_dis(&GPIO, handle->pin_scl);
    }

    gpio_ll_iomux_func_sel(IO_MUX_REGS[handle->pin_scl], PIN_FUNC_GPIO);
    gpio_ll_iomux_func_sel(IO_MUX_REGS[handle->pin_sda], PIN_FUNC_GPIO);

    int ext_scl_out_idx = (handle->port == 0) ? I2CEXT0_SCL_OUT_IDX 
                                              : I2CEXT1_SCL_OUT_IDX;
    int ext_sda_out_idx = (handle->port == 0) ? I2CEXT0_SDA_OUT_IDX 
                                              : I2CEXT1_SDA_OUT_IDX;
    int ext_scl_in_idx = (handle->port == 0) ? I2CEXT0_SCL_IN_IDX 
                                             : I2CEXT1_SCL_IN_IDX;
    int ext_sda_in_idx = (handle->port == 0) ? I2CEXT0_SDA_IN_IDX 
                                             : I2CEXT1_SDA_IN_IDX;

    esp_rom_gpio_connect_out_signal(handle->pin_scl, ext_scl_out_idx, false, false);
    esp_rom_gpio_connect_in_signal(handle->pin_scl, ext_scl_in_idx, false);
    esp_rom_gpio_connect_out_signal(handle->pin_sda, ext_sda_out_idx, false, false);
    esp_rom_gpio_connect_in_signal(handle->pin_sda, ext_sda_in_idx, false);
}

static void i2c_master_config_bus(i2c_master_handle_t* handle) {
    i2c_ll_enable_controller_clock(handle->dev, true);

    PERIPH_RCC_ATOMIC() {
        i2c_ll_enable_bus_clock(handle->port, true);
        i2c_ll_reset_register(handle->port);
    }

    i2c_ll_master_init(handle->dev);
    i2c_ll_set_data_mode(handle->dev, 
                         handle->lsb_first 
                            ? I2C_DATA_MODE_LSB_FIRST 
                            : I2C_DATA_MODE_MSB_FIRST, 
                         handle->lsb_first 
                            ? I2C_DATA_MODE_LSB_FIRST 
                            : I2C_DATA_MODE_MSB_FIRST);
    i2c_ll_txfifo_rst(handle->dev);
    i2c_ll_rxfifo_rst(handle->dev);

    soc_module_clk_t clk_src = (soc_module_clk_t)I2C_CLK_SRC_DEFAULT;
    esp_clk_tree_src_get_freq_hz(clk_src, 
                                 ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, 
                                 &handle->clk_src_hz);

    i2c_hal_clk_config_t clk_cfg;
    i2c_ll_master_cal_bus_clk(handle->clk_src_hz, handle->freq_hz, &clk_cfg);
    i2c_ll_master_set_bus_timing(handle->dev, &clk_cfg);
    i2c_ll_master_set_filter(handle->dev, handle->glitch_ignore_cnt);
    i2c_ll_set_source_clk(handle->dev, (soc_periph_i2c_clk_src_t)clk_src);
    i2c_ll_update(handle->dev);
    i2c_ll_clear_intr_mask(handle->dev, I2C_LL_MASTER_EVENT_INTR);
}

static void i2c_master_isr_init(i2c_master_handle_t* handle) {
    i2c_master_isr_enable(handle->dev, false);
    i2c_ll_clear_intr_mask(handle->dev, I2C_LL_MASTER_EVENT_INTR);
    esp_intr_alloc(ETS_I2C_EXT0_INTR_SOURCE, 
                   ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1, 
                   i2c_master_isr_handler, (void*)handle, NULL);
}

void IRAM_ATTR i2c_master_isr_enable(i2c_dev_t* dev, bool enable) {
    if (enable) {
        i2c_ll_enable_intr_mask(dev, I2C_LL_MASTER_EVENT_INTR);
    } else {
        i2c_ll_disable_intr_mask(dev, I2C_LL_MASTER_EVENT_INTR);
    }
}

void IRAM_ATTR i2c_master_isr_handler(void* arg) {
    i2c_master_handle_t* handle = (i2c_master_handle_t*)arg;
    i2c_intr_event_t event;
    i2c_ll_master_get_event(handle->dev, &event);
    i2c_ll_clear_intr_mask(handle->dev, I2C_LL_MASTER_EVENT_INTR);
    if (!handle->xfer_done && (event != I2C_INTR_EVENT_ERR)) {
        i2c_master_isr_enable(handle->dev, false);
        handle->xfer_result = event;
        handle->xfer_done = true;
    }
}

static void i2c_master_clear_bus(i2c_master_handle_t* handle) {
    i2c_ll_master_fsm_rst(handle->dev);
    i2c_ll_txfifo_rst(handle->dev);
    i2c_ll_rxfifo_rst(handle->dev);
    i2c_ll_update(handle->dev);

    gpio_ll_iomux_func_sel(IO_MUX_REGS[handle->pin_sda], PIN_FUNC_GPIO);
    gpio_ll_iomux_func_sel(IO_MUX_REGS[handle->pin_scl], PIN_FUNC_GPIO);
    gpio_ll_od_enable(&GPIO, handle->pin_sda);
    gpio_ll_od_enable(&GPIO, handle->pin_scl);
    gpio_ll_pullup_en(&GPIO, handle->pin_sda);
    gpio_ll_pullup_en(&GPIO, handle->pin_scl);

    /*
     * If SDA is low it is driven by the slave, wait until SDA goes high, at
     * maximum 9 clock cycles in standard mode at 100 kHz including the ACK bit.
     */
    uint32_t half_cycle = 5;
    int count = 9;

    while (!gpio_ll_get_level(&GPIO, handle->pin_sda) && count--) {
        // Drive SCL low
        gpio_ll_output_enable(&GPIO, handle->pin_scl);
        gpio_ll_set_level(&GPIO, handle->pin_scl, 0);
        esp_rom_delay_us(half_cycle);
        // Drive SCL high
        gpio_ll_set_level(&GPIO, handle->pin_scl, 1);
        esp_rom_delay_us(half_cycle);
    }

    // Generate STOP condition
    // SCL low, SDA low
    gpio_ll_set_level(&GPIO, handle->pin_scl, 0);
    esp_rom_delay_us(half_cycle);
    gpio_ll_set_level(&GPIO, handle->pin_sda, 0);
    esp_rom_delay_us(half_cycle);
    // SCL high
    gpio_ll_set_level(&GPIO, handle->pin_scl, 1);
    esp_rom_delay_us(half_cycle);
    // SDA high (STOP)
    gpio_ll_set_level(&GPIO, handle->pin_sda, 1);
    esp_rom_delay_us(half_cycle);
}

static inline const char* i2c_master_evt_to_str(i2c_intr_event_t event) {
    switch (event) {
    case I2C_INTR_EVENT_ERR:
        return "I2C_INTR_EVENT_ERR";
    case I2C_INTR_EVENT_ARBIT_LOST:
        return "I2C_INTR_EVENT_ARBIT_LOST";
    case I2C_INTR_EVENT_NACK:
        return "I2C_INTR_EVENT_NACK";
    case I2C_INTR_EVENT_TOUT:
        return "I2C_INTR_EVENT_TOUT";
    case I2C_INTR_EVENT_END_DET:
        return "I2C_INTR_EVENT_END_DET";
    case I2C_INTR_EVENT_TRANS_DONE:
        return "I2C_INTR_EVENT_TRANS_DONE";
    case I2C_INTR_EVENT_RXFIFO_FULL:
        return "I2C_INTR_EVENT_RXFIFO_FULL";
    case I2C_INTR_EVENT_TXFIFO_EMPTY:
        return "I2C_INTR_EVENT_TXFIFO_EMPTY";
    default:
        return "Unknown event";
    }
}

static inline bool i2c_master_err_check(i2c_master_handle_t* handle, 
                                        i2c_intr_event_t event) {
    if ((event != I2C_INTR_EVENT_TRANS_DONE) &&
        (event != I2C_INTR_EVENT_END_DET)) {
        ogxm_loge("I2C: Xfer failed: %s\n", 
            i2c_master_evt_to_str(event));
        i2c_master_clear_bus(handle);
        i2c_master_config_gpio(handle);
        i2c_master_config_bus(handle);
        return false;
    }
    return true;
}

static inline void i2c_master_cmd_start(i2c_dev_t* dev, int* cmd_idx) {
    i2c_ll_hw_cmd_t cmd = {0};
    cmd.op_code = I2C_LL_CMD_RESTART;
    i2c_ll_master_write_cmd_reg(dev, cmd, *cmd_idx);
    (*cmd_idx)++;
}

static inline void i2c_master_cmd_stop(i2c_dev_t* dev, int* cmd_idx) {
    i2c_ll_hw_cmd_t cmd = {0};
    cmd.op_code = I2C_LL_CMD_STOP;
    i2c_ll_master_write_cmd_reg(dev, cmd, *cmd_idx);
    (*cmd_idx)++;
}

static inline void i2c_master_cmd_end(i2c_dev_t* dev, int* cmd_idx) {
    i2c_ll_hw_cmd_t cmd = {0};
    cmd.op_code = I2C_LL_CMD_END;
    i2c_ll_master_write_cmd_reg(dev, cmd, *cmd_idx);
    (*cmd_idx)++;
}

static inline void i2c_master_cmd_write(i2c_dev_t* dev, int* cmd_idx, 
                                        uint8_t* data, uint8_t len) {
    i2c_ll_write_txfifo(dev, data, len);
    i2c_ll_hw_cmd_t cmd = { 
        .op_code = I2C_LL_CMD_WRITE,
        .byte_num = len,
        .ack_en = 1,
        .ack_exp = 0,
        .ack_val = 0 
    };
    i2c_ll_master_write_cmd_reg(dev, cmd, *cmd_idx);
    (*cmd_idx)++;
}

static inline void i2c_master_cmd_read(i2c_dev_t* dev, int* cmd_idx, uint8_t len, bool last) {
    if ((len == 0) || (len > SOC_I2C_FIFO_LEN)) {
        ogxm_loge("I2C: Invalid read length: %d\n", len);
        return;
    }
    if (len > 1) {
        i2c_ll_hw_cmd_t cmd = { 
            .op_code = I2C_LL_CMD_READ,
            .byte_num = len - 1,
            .ack_en = 0,
            .ack_exp = 0,
            .ack_val = 0 
        };
        i2c_ll_master_write_cmd_reg(dev, cmd, *cmd_idx);
        (*cmd_idx)++;
    }
    i2c_ll_hw_cmd_t cmd = { 
        .op_code = I2C_LL_CMD_READ,
        .byte_num = 1,
        .ack_en = 1,
        .ack_exp = 0,
        .ack_val = last ? 1 : 0
    };
    i2c_ll_master_write_cmd_reg(dev, cmd, *cmd_idx);
    (*cmd_idx)++;
}

static inline i2c_intr_event_t i2c_master_xfer_blocking(i2c_master_handle_t* handle, 
                                                        int cmd_idx, uint32_t timeout_us) {
    uint32_t reg_val = 
        i2c_ll_calculate_timeout_us_to_reg_val(handle->clk_src_hz, 
                                               timeout_us);
    i2c_ll_set_tout(handle->dev, reg_val);
    i2c_ll_update(handle->dev);
    handle->xfer_done = false;
    i2c_master_isr_enable(handle->dev, true);
    i2c_ll_start_trans(handle->dev);

    uint32_t start = esp_timer_get_time();
    while (!handle->xfer_done) {
        // A timeout is written to the register, but this can get stuck anyway
        if ((esp_timer_get_time() - start) > (timeout_us + 100)) {
            i2c_master_isr_enable(handle->dev, false);
            handle->xfer_result = I2C_INTR_EVENT_TOUT;
            break;
        }
    }
    ogxm_logv("I2C: xfer done: %s\n", i2c_master_evt_to_str(handle->xfer_result));
    return handle->xfer_result;
}