#pragma once

#include "board_config.h"
#if SD_CARD_ENABLED

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool sd_init(void);
bool sd_present(void);

/* MSC/Ogxbox Partition */

bool sd_msc_read_blocks(uint8_t* buffer, uint32_t lba, uint32_t count);
bool sd_msc_write_blocks(const uint8_t* buffer, uint32_t lba, uint32_t count);
uint32_t sd_msc_get_block_count(void);

/* Internal use partition */

bool sd_fatfs_read_blocks(uint8_t* buffer, uint32_t lba, uint32_t count);
bool sd_fatfs_write_blocks(const uint8_t* buffer, uint32_t lba, uint32_t count);
uint32_t sd_fatfs_get_block_count(void);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_ENABLED