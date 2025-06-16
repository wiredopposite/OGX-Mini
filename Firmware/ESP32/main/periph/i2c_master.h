#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct i2c_master_handle_ i2c_master_handle_t;

typedef struct {
    uint8_t     port_num;
    uint32_t    freq_hz;
    uint8_t     pin_sda;
    uint8_t     pin_scl;
    bool        lsb_first;
    bool        pullup_en;
    int         glitch_ignore_cnt;
} i2c_master_cfg;

i2c_master_handle_t* i2c_master_init(const i2c_master_cfg* cfg);
bool i2c_master_write_blocking(i2c_master_handle_t* handle, uint8_t addr, uint8_t* data, size_t len, uint32_t timeout_us);
bool i2c_master_read_blocking(i2c_master_handle_t* handle, uint8_t addr, uint8_t* data, size_t len, uint32_t timeout_us);
void i2c_master_deinit(i2c_master_handle_t* handle);

#ifdef __cplusplus
}
#endif