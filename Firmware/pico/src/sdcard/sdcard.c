#include "board_config.h"
#if SD_CARD_ENABLED

#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <pico/sync.h>
#include "log/log.h"
#include "sdcard/sd_spi.h"
#include "sdcard/sdcard.h"

#define KILOBYTE        (1024U)
#define GIGABYTES(x)    ((x) * KILOBYTE * KILOBYTE * KILOBYTE)
#define MEGABYTES(x)    ((x) * KILOBYTE * KILOBYTE)
#define KILOBYTES(x)    ((x) * KILOBYTE)

#define MSC_MINIMUM_PARTITION_BLOCKS    (MEGABYTES(32U) / SD_BLOCK_SIZE)
#define MSC_MAXIMUM_PARTITION_BLOCKS    (GIGABYTES(4U) / SD_BLOCK_SIZE)
#define MSC_MAX_RATIO_DENOMINATOR       4U // 75%
#define MSC_MAX_RATIO_NUMERATOR         3U // 75%

#define FATFS_MINIMUM_PARTITION_BLOCKS  (MEGABYTES(32U) / SD_BLOCK_SIZE)

typedef struct {
    volatile bool   inited;
    mutex_t         mutex;
    uint32_t        block_count;
    struct {
        uint32_t    start_block;
        uint32_t    blocks;
    } partitions[SD_PART_MAX];
} sdcard_state_t;

static sdcard_state_t sdcard = {0};

static inline void sdcard_lock(void) {
    if (!mutex_is_initialized(&sdcard.mutex)) {
        mutex_init(&sdcard.mutex);
    }
    mutex_enter_blocking(&sdcard.mutex);
}

static inline void sdcard_unlock(void) {
    mutex_exit(&sdcard.mutex);
}

/* Private unprotected */

static void sdcard_calculate_partition_sizes(sdcard_state_t* sd) {
    sd->partitions[SD_PART_MSC].start_block = 0;

    uint32_t msc_blocks = 
        (sd->block_count / MSC_MAX_RATIO_DENOMINATOR) * MSC_MAX_RATIO_NUMERATOR;

    sd->partitions[SD_PART_MSC].blocks = msc_blocks;

    if (sd->partitions[SD_PART_MSC].blocks < MSC_MINIMUM_PARTITION_BLOCKS) {
        ogxm_logd("SD: MSC partition size too small, setting to minimum");
        sd->partitions[SD_PART_MSC].blocks = MSC_MINIMUM_PARTITION_BLOCKS;
    } else if (sd->partitions[SD_PART_MSC].blocks > MSC_MAXIMUM_PARTITION_BLOCKS) {
        ogxm_logd("SD: MSC partition size too large, setting to maximum");
        sd->partitions[SD_PART_MSC].blocks = MSC_MAXIMUM_PARTITION_BLOCKS;
    }

    sd->partitions[SD_PART_FATFS].start_block = 
        sd->partitions[SD_PART_MSC].start_block + sd->partitions[SD_PART_MSC].blocks;
    sd->partitions[SD_PART_FATFS].blocks = 
        sd->block_count - sd->partitions[SD_PART_MSC].blocks;

    if (sd->partitions[SD_PART_FATFS].blocks < FATFS_MINIMUM_PARTITION_BLOCKS) {
        ogxm_logd("SD: FATFS partition size too small, setting to minimum");
        sd->partitions[SD_PART_FATFS].blocks = FATFS_MINIMUM_PARTITION_BLOCKS;
    }
}

static bool sdcard_format_disk(sdcard_state_t* sd) {
    sdcard_calculate_partition_sizes(sd);

    uint8_t mbr[SD_BLOCK_SIZE] = {0};

    // Partition 1: Pico/FatFs
    uint8_t *part1 = &mbr[446];
    part1[0] = 0x00; // Not bootable
    part1[4] = 0x0C; // FAT32 LBA
    part1[8]  = (uint8_t)(sd->partitions[SD_PART_FATFS].start_block & 0xFF);
    part1[9]  = (uint8_t)((sd->partitions[SD_PART_FATFS].start_block >> 8) & 0xFF);
    part1[10] = (uint8_t)((sd->partitions[SD_PART_FATFS].start_block >> 16) & 0xFF);
    part1[11] = (uint8_t)((sd->partitions[SD_PART_FATFS].start_block >> 24) & 0xFF);
    part1[12] = (uint8_t)(sd->partitions[SD_PART_FATFS].blocks & 0xFF);
    part1[13] = (uint8_t)((sd->partitions[SD_PART_FATFS].blocks >> 8) & 0xFF);
    part1[14] = (uint8_t)((sd->partitions[SD_PART_FATFS].blocks >> 16) & 0xFF);
    part1[15] = (uint8_t)((sd->partitions[SD_PART_FATFS].blocks >> 24) & 0xFF);

    // Partition 2: USB MSC
    uint8_t *part2 = &mbr[462];
    part2[0] = 0x00; // Not bootable
    part2[4] = 0x0C; // FAT32 LBA (or 0x83 for Linux, etc.)
    part2[8]  = (uint8_t)(sd->partitions[SD_PART_MSC].start_block & 0xFF);
    part2[9]  = (uint8_t)((sd->partitions[SD_PART_MSC].start_block >> 8) & 0xFF);
    part2[10] = (uint8_t)((sd->partitions[SD_PART_MSC].start_block >> 16) & 0xFF);
    part2[11] = (uint8_t)((sd->partitions[SD_PART_MSC].start_block >> 24) & 0xFF);
    part2[12] = (uint8_t)(sd->partitions[SD_PART_MSC].blocks & 0xFF);
    part2[13] = (uint8_t)((sd->partitions[SD_PART_MSC].blocks >> 8) & 0xFF);
    part2[14] = (uint8_t)((sd->partitions[SD_PART_MSC].blocks >> 16) & 0xFF);
    part2[15] = (uint8_t)((sd->partitions[SD_PART_MSC].blocks >> 24) & 0xFF);

    mbr[510] = 0x55;
    mbr[511] = 0xAA;

    return ((sd_spi_write_blocks(0, mbr, 1) == SD_ERR_OKAY) && 
           (sd_spi_sync() == SD_ERR_OKAY));
}

static bool sdcard_check_partitions(const uint8_t* mbr, sdcard_state_t* sd) {
    // Partition 1 (FATFS)
    const uint8_t *part1 = &mbr[446];
    uint32_t part1_start =  part1[8] | (part1[9] << 8) | 
                            (part1[10] << 16) | (part1[11] << 24);
    uint32_t part1_size  =  part1[12] | (part1[13] << 8) | 
                            (part1[14] << 16) | (part1[15] << 24);

    // Partition 2 (MSC)
    const uint8_t *part2 = &mbr[462];
    uint32_t part2_start =  part2[8] | (part2[9] << 8) | 
                            (part2[10] << 16) | (part2[11] << 24);
    uint32_t part2_size  =  part2[12] | (part2[13] << 8) | 
                            (part2[14] << 16) | (part2[15] << 24);

    sdcard_state_t sd_cmp = {0};
    sdcard_calculate_partition_sizes(&sd_cmp);
    // Check if matches expected layout
    if ((part1_start == sd_cmp.partitions[SD_PART_FATFS].start_block) && 
        (part1_size == sd_cmp.partitions[SD_PART_FATFS].blocks) &&
        (part2_start == sd_cmp.partitions[SD_PART_MSC].start_block) && 
        (part2_size == sd_cmp.partitions[SD_PART_MSC].blocks)) {
        return true; // Already partitioned as expected
    }
    return false;
}

static bool sdcard_init_all(sdcard_state_t* sd) {
    uint8_t mbr[SD_BLOCK_SIZE] = {0};
    sd_error_t err = sd_spi_read_blocks(0, mbr, 1);
    if (err != SD_ERR_OKAY) {
        ogxm_loge("Failed to read MBR, Error: %d", err);
        return false;
    }
    if ((mbr[510] != 0x55) || (mbr[511] != 0xAA)) {
        ogxm_logd("SD: No MBR found, formatting disk");
        if (!sdcard_format_disk(sd)) {
            ogxm_loge("SD: Failed to format disk");
            return false;
        }
    } else {
        ogxm_logd("SD: MBR found, checking partitions");
    }
    if (!sdcard_check_partitions(mbr, sd)) {
        ogxm_logd("SD: Partitions do not match expected layout, formatting disk");
        if (!sdcard_format_disk(sd)) {
            ogxm_loge("SD: Failed to format disk");
            return false;
        }
    } else {
        ogxm_logd("SD: Partitions match expected layout");
    }
    sd->inited = true;
    return true;
}

/* Public thread safe methods */

bool sdcard_init(void) {
    sdcard_lock();
    if (sdcard.inited) {
        ogxm_logd("SD card already initialized");
        sdcard_unlock();
        return true;
    }
    sd_error_t err = sd_spi_init();
    if (err != SD_ERR_OKAY) {
        ogxm_loge("SD: Failed to initialize SD SPI, Error: %d", err);
        sdcard_unlock(); 
        return false; 
    } 
    err = sd_spi_get_block_count(&sdcard.block_count);
    if (err != SD_ERR_OKAY) {
        ogxm_loge("SD: Failed to get block count, Error: %d", err);
        sdcard_unlock();
        return false; 
    } 
    if (sdcard.block_count < MSC_MINIMUM_PARTITION_BLOCKS) {
        ogxm_loge("SD: Card too small for minimum partition size");
        sdcard_unlock();
        return false;
    }
    if (!sdcard_init_all(&sdcard)) {
        sdcard_unlock();
        return false;
    }
    ogxm_logi("SD: Initialized successfully, Block Count: %u", sdcard.block_count);
    sdcard_unlock();
    return true;
}

bool sdcard_inited(void) {
    sdcard_lock();
    if (!sdcard.inited) {
        ogxm_loge("SD: Not initialized");
        sdcard_unlock();
        return false;
    }
    sdcard_unlock();
    ogxm_logd("SD: Initialized");
    return true;
}

void sdcard_deinit(void) {
    sdcard_lock();
    if (!sdcard.inited) {
        ogxm_logd("SD card not initialized, nothing to deinitialize");
        sdcard_unlock();
        return;
    }
    sd_error_t err = sd_spi_deinit();
    if (err != SD_ERR_OKAY) {
        ogxm_loge("SD: Failed to deinitialize SD SPI, Error: %d", err);
    } else {
        ogxm_logi("SD card deinitialized successfully");
    }
    sdcard.inited = false;
    sdcard_unlock();
}

bool sdcard_read_blocks(sd_partition_t partition, uint32_t block, uint8_t *buffer, uint32_t count) {
    sdcard_lock();
    if (!sdcard.inited || !buffer || (count == 0) || 
        ((uint8_t)partition >= SD_PART_MAX)) {
        ogxm_loge("SD: Invalid arguments for read_blocks");
        sdcard_unlock();
        return false;
    }
    uint32_t start_block = sdcard.partitions[partition].start_block + block;
    if ((start_block + count) > 
        (sdcard.partitions[partition].start_block + sdcard.partitions[partition].blocks)) {
        ogxm_loge("SD: Read exceeds partition bounds");
        sdcard_unlock();
        return false;
    }
    sd_error_t err = sd_spi_read_blocks(start_block, buffer, count);
    sdcard_unlock();
    if (err != SD_ERR_OKAY) {
        ogxm_loge("SD: Failed to read blocks, Error: %d", err);
        return false;
    }
    return true;
}

bool sdcard_write_blocks(sd_partition_t partition, uint32_t block, 
                         const uint8_t *buffer, uint32_t count) {
    sdcard_lock();
    if (!sdcard.inited || !buffer || (count == 0) || 
        ((uint8_t)partition >= SD_PART_MAX)) {
        ogxm_loge("SD: Invalid arguments for write_blocks");
        sdcard_unlock();
        return false;
    }
    uint32_t start_block = sdcard.partitions[partition].start_block + block;
    if ((start_block + count) > 
        (sdcard.partitions[partition].start_block + sdcard.partitions[partition].blocks)) {
        ogxm_loge("SD: Write exceeds partition bounds");
        sdcard_unlock();
        return false;
    }
    sd_error_t err = sd_spi_write_blocks(start_block, buffer, count);
    sdcard_unlock();
    if (err != SD_ERR_OKAY) {
        ogxm_loge("SD: Failed to write blocks, Error: %d", err);
        return false;
    }
    return true;
}

bool sdcard_sync(void) {
    sdcard_lock();
    if (!sdcard.inited) {
        ogxm_loge("SD: Invalid arguments for sync");
        sdcard_unlock();
        return false;
    }
    sd_error_t err = sd_spi_sync();
    sdcard_unlock();
    if (err != SD_ERR_OKAY) {
        ogxm_loge("SD: Failed to sync, Error: %d", err);
        return false;
    }
    return true;
}

bool sdcard_get_block_count(sd_partition_t partition, uint32_t* count) {
    sdcard_lock();
    if (!count || ((uint8_t)partition >= SD_PART_MAX)) {
        ogxm_loge("SD: Invalid arguments for get_block_count");
        sdcard_unlock();
        return false;
    }
    if (!sdcard.inited) {
        ogxm_loge("SD: Card not initialized");
        sdcard_unlock();
        return false;
    }
    *count = sdcard.partitions[partition].blocks;
    sdcard_unlock();
    return true;
}

#endif // SD_CARD_ENABLED