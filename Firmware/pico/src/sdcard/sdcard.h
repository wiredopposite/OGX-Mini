#pragma once

#include "board_config.h"
#if SD_CARD_ENABLED

#include <stdint.h>
#include <stdbool.h>
#include "sdcard/sd_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_PART_MSC = 0,
    SD_PART_FATFS,
    SD_PART_MAX
} sd_partition_t;

bool sdcard_init(void);
void sdcard_deinit(void);
bool sdcard_inited(void);
bool sdcard_read_blocks(sd_partition_t partition, uint32_t block, uint8_t *buffer, uint32_t count);
bool sdcard_write_blocks(sd_partition_t partition, uint32_t block, const uint8_t *buffer, uint32_t count);
bool sdcard_sync(void);
bool sdcard_get_block_count(sd_partition_t partition, uint32_t* count);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_ENABLED