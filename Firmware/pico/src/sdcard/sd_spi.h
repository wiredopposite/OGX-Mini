#pragma once

#include "board_config.h"
#if SD_CARD_ENABLED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Don't use directly, use methods in sdcard.h */

#define SD_BLOCK_SIZE 512U

typedef enum {
    SD_ERR_OKAY = 0,
    SD_ERR_NO_INIT,
    SD_ERR_NO_DISK,
    SD_ERR_TIMEOUT,
    SD_ERR_INVALID_ARG,
    SD_ERR_WRITE_PROTECTED,
    SD_ERR_READ_WRITE,
} sd_error_t;

sd_error_t sd_spi_init(void);
sd_error_t sd_spi_inited(void);
sd_error_t sd_spi_deinit(void);
sd_error_t sd_spi_read_blocks(uint32_t block, uint8_t *buffer, uint32_t count);
sd_error_t sd_spi_write_blocks(uint32_t block, const uint8_t *buffer, uint32_t count);
sd_error_t sd_spi_sync(void);
sd_error_t sd_spi_get_block_count(uint32_t* count);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_ENABLED