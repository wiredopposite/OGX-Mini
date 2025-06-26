#include "board_config.h"
#if SD_CARD_ENABLED

#include <string.h>
#include <pico/stdlib.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <pico/sync.h>
#include "sd_spi.h"

typedef enum {
    CMDO_GO_IDLE_STATE = 0,
    CMD2_ALL_SEND_CID = 2,
    CMD3_SEND_RELATIVE_ADDR = 3,
    CMD4_SET_DSR = 4,
    // Reserved for DSIO, I/O cards
    CMD7_SELECT_DESELECT_CARD = 7,
    CMD8_SEND_IF_COND = 8,
    CMD9_SEND_CSD = 9,
    CMD10_SEND_CID = 10,
    CMD11_VOLTAGE_SWITCH = 11,
    CMD12_STOP_TRANSMISSION = 12,
    CMD13_SEND_STATUS = 13,
    CMD15_GO_INACTIVE_STATE = 15,
    CMD16_SET_BLOCKLEN = 16,
    CMD17_READ_SINGLE_BLOCK = 17,
    CMD18_READ_MULTIPLE_BLOCK = 18,
    CMD19_SEND_TUNING_BLOCK = 19,
    CMD20_SPEED_CLASS_CONTROL = 20,
    CMD22_ADDRESS_EXTENSION = 22,
    CMD23_SET_BLOCK_COUNT = 23,
    CMD24_WRITE_BLOCK = 24,
    CMD25_WRITE_MULTIPLE_BLOCK = 25,
    CMD27_PROGRAM_CSD = 27,
    CMD28_SET_WRITE_PROT = 28,
    CMD29_CLR_WRITE_PROT = 29,
    CMD30_SEND_WRITE_PROT = 30,
    CMD32_ERASE_WR_BLK_START = 32,
    CMD33_ERASE_WR_BLK_END = 33,
    CMD38_ERASE = 38,
    CMD43_Q_MANAGEMENT = 43,
    CMD44_Q_TASK_INFO_A = 44,
    CMD45_Q_TASK_INFO_B = 45,
    CMD46_Q_RD_TASK = 46,
    CMD47_Q_WR_TASK = 47,
    CMD48_READ_EXTR_SINGLE = 48,
    CMD49_WRITE_EXTR_SINGLE = 49,
    CMD55_APP_CMD = 55,
    CMD56_GEN_CMD = 56,
    CMD58_READ_EXTR_MULTI = 58,
    CMD59_WRITE_EXTR_MULTI = 59,
    // SDIO commands
} sd_cmd_t;

typedef enum {
    ACMD6_SET_BUS_WIDTH = 6,
    ACMD13_SD_STATUS = 13,
    ACMD22_SEND_NUM_WR_BLOCKS = 22,
    ACMD23_SET_WR_BLK_ERASE_COUNT = 23,
    ACMD41_SD_SEND_OP_COND = 41,
    ACMD42_SET_CLR_CARD_DETECT = 42,
    ACMD51_SEND_SCR = 51,
} sd_acmd_t;

#define SPI_HW              __CONCAT(spi, SD_CARD_SPI_NUM)
#define SPI_BAUDRATE        (1000U*1000U) // 1 MHz

#define SD_SPI_ASSERT()      gpio_put(SD_CARD_SPI_PIN_CS, 0)
#define SD_SPI_DEASSERT()    gpio_put(SD_CARD_SPI_PIN_CS, 1)

typedef struct {
    bool inited;
    bool card_present;
} sd_spi_state_t;

static sd_spi_state_t sd_spi_state = {0};

static uint8_t sd_spi_xfer(uint8_t data) {
    uint8_t rx;
    spi_write_read_blocking(SPI_HW, &data, &rx, 1);
    return rx;
}

static uint8_t sd_spi_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    sd_spi_xfer(0xFF);

    sd_spi_xfer(0x40 | cmd);
    sd_spi_xfer(arg >> 24);
    sd_spi_xfer(arg >> 16);
    sd_spi_xfer(arg >> 8);
    sd_spi_xfer(arg);
    sd_spi_xfer(crc);

    // Wait for response (response is not 0xFF)
    for (int i = 0; i < 8; i++) {
        uint8_t resp = sd_spi_xfer(0xFF);
        if ((resp & 0x80) == 0) {
            return resp;
        }
    }
    return 0xFF; // Timeout
}

// static bool sd_card_preset(void) {
//     SD_SPI_ASSERT();
//     uint8_t resp = sd_spi_send_cmd(CMDO_GO_IDLE_STATE, 0, 0x95); // CMD0: GO_IDLE_STATE
//     SD_SPI_DEASSERT();
//     return (resp == 0x01);
// }

static bool sd_spi_wait_ready(void) {
    uint8_t resp = 0;
    for (int i = 0; i < 50000; i++) {
        if ((resp = sd_spi_xfer(0xFF)) == 0xFF) {
            break;
        }
    }
    return (resp == 0xFF);
}

sd_error_t sd_spi_init(void) {
    if (sd_spi_state.inited) {
        return SD_ERR_OKAY;
    }

    spi_init(SPI_HW, SPI_BAUDRATE);
    spi_set_format(SPI_HW, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(SD_CARD_SPI_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SD_CARD_SPI_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SD_CARD_SPI_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_CARD_SPI_PIN_CS, GPIO_FUNC_SIO);

    gpio_set_dir(SD_CARD_SPI_PIN_CS, GPIO_OUT);
    SD_SPI_DEASSERT();

    // 80+ clock cycles with CS high
    for (int i = 0; i < 10; i++) {
        sd_spi_xfer(0xFF);
    }

    SD_SPI_ASSERT();
    // CMD0: GO_IDLE_STATE
    uint8_t resp = sd_spi_send_cmd(CMDO_GO_IDLE_STATE, 0, 0x95);
    SD_SPI_DEASSERT();
    if (resp != 0x01) {
        sd_spi_state.card_present = false;
        return SD_ERR_NO_DISK;
    }

    // CMD8: SEND_IF_COND (for SDHC/SDXC)
    SD_SPI_ASSERT();
    resp = sd_spi_send_cmd(CMD8_SEND_IF_COND, 0x1AA, 0x87);
    uint8_t cmd8_resp[4];
    for (int i = 0; i < 4; i++) {
        cmd8_resp[i] = sd_spi_xfer(0xFF);
    }
    SD_SPI_DEASSERT();

    bool sdhc = false;
    if (resp == 0x01 && cmd8_resp[2] == 0x01 && cmd8_resp[3] == 0xAA) {
        // Card supports SDHC/SDXC
        // ACMD41 with HCS
        for (int i = 0; i < 1000; i++) {
            // CMD55
            SD_SPI_ASSERT();
            resp = sd_spi_send_cmd(CMD55_APP_CMD, 0, 0x65);
            SD_SPI_DEASSERT();
            // ACMD41
            SD_SPI_ASSERT();
            resp = sd_spi_send_cmd(ACMD41_SD_SEND_OP_COND, 0x40000000, 0x77);
            SD_SPI_DEASSERT();
            if (resp == 0x00) {
                sdhc = true;
                break;
            }
            sleep_ms(1);
        }
    } else {
        // Standard capacity card: ACMD41 without HCS
        for (int i = 0; i < 1000; i++) {
            SD_SPI_ASSERT();
            resp = sd_spi_send_cmd(CMD55_APP_CMD, 0, 0x65);
            SD_SPI_DEASSERT();
            SD_SPI_ASSERT();
            resp = sd_spi_send_cmd(ACMD41_SD_SEND_OP_COND, 0, 0x77);
            SD_SPI_DEASSERT();
            if (resp == 0x00) {
                break;
            }
            sleep_ms(1);
        }
    }

    // Optionally: CMD58 to read OCR and confirm SDHC/SDXC

    sd_spi_state.inited = true;
    sd_spi_state.card_present = true;
    return SD_ERR_OKAY;
}

sd_error_t sd_spi_inited(void) {
    if (!sd_spi_state.inited) {
        return SD_ERR_NO_INIT;
    }
    return sd_spi_state.card_present ? SD_ERR_OKAY : SD_ERR_NO_DISK;
}

sd_error_t sd_spi_deinit(void) {
    if (!sd_spi_state.inited) {
        return SD_ERR_NO_INIT;
    }
    spi_deinit(SPI_HW);
    gpio_deinit(SD_CARD_SPI_PIN_CS);
    gpio_deinit(SD_CARD_SPI_PIN_SCK);
    gpio_deinit(SD_CARD_SPI_PIN_MOSI);
    gpio_deinit(SD_CARD_SPI_PIN_MISO);

    sd_spi_state.inited = false;
    return SD_ERR_OKAY;
}

sd_error_t sd_spi_read_blocks(uint32_t block, uint8_t *buffer, uint32_t count) {
    if (!buffer || (count == 0)) {
        return SD_ERR_INVALID_ARG;
    }
    if (!sd_spi_state.inited) {
        return SD_ERR_NO_INIT;
    }
    if (!sd_spi_state.card_present) {
        return SD_ERR_NO_DISK;
    }
    for (uint32_t i = 0; i < count; i++) {
        SD_SPI_ASSERT();
        uint8_t resp = sd_spi_send_cmd(CMD17_READ_SINGLE_BLOCK, (block + i) * SD_BLOCK_SIZE, 0xFF); // CMD17: READ_SINGLE_BLOCK
        if (resp != 0x00) {
            SD_SPI_DEASSERT();
            return SD_ERR_READ_WRITE;
        }

        int tries = 10000;
        while (sd_spi_xfer(0xFF) != 0xFE && tries--) {
            // Wait for the start token (0xFE)
        }
        if (tries <= 0) {
            return SD_ERR_TIMEOUT;
        }

        for (uint32_t j = 0; j < SD_BLOCK_SIZE; j++) {
            buffer[i * SD_BLOCK_SIZE + j] = sd_spi_xfer(0xFF);
        }

        sd_spi_xfer(0xFF);
        sd_spi_xfer(0xFF);

        SD_SPI_DEASSERT();
        sd_spi_xfer(0xFF);
    }
    return SD_ERR_OKAY;
}

sd_error_t sd_spi_write_blocks(uint32_t block, const uint8_t *buffer, uint32_t count) {
    if (!buffer || (count == 0)) {
        return SD_ERR_INVALID_ARG;
    }
    if (!sd_spi_state.inited) {
        return SD_ERR_NO_INIT;
    }
    if (!sd_spi_state.card_present) {
        return SD_ERR_NO_DISK;
    }
    for (uint32_t i = 0; i < count; i++) {
        SD_SPI_ASSERT();
        uint8_t resp = sd_spi_send_cmd(CMD24_WRITE_BLOCK, (block + i) * SD_BLOCK_SIZE, 0x01); // CMD17: READ_SINGLE_BLOCK
        if (resp != 0x00) {
            SD_SPI_DEASSERT();
            return SD_ERR_READ_WRITE;
        }

        sd_spi_xfer(0xFF); // Ncr
        sd_spi_xfer(0xFE); // Data token

        for (uint32_t j = 0; j < SD_BLOCK_SIZE; j++) {
            sd_spi_xfer(buffer[i * SD_BLOCK_SIZE + j]);
        }

        sd_spi_xfer(0xFF); // CRC (not used, can be 0xFF)
        sd_spi_xfer(0xFF); // CRC (not used, can be 0xFF)

        uint8_t resp2 = sd_spi_xfer(0xFF); // Wait for data response
        if ((resp2 & 0x1F) != 0x05) { // Check for data accepted response
            SD_SPI_DEASSERT();
            return SD_ERR_READ_WRITE;
        }
        if (!sd_spi_wait_ready()) {
            SD_SPI_DEASSERT();
            return SD_ERR_TIMEOUT;
        }
        SD_SPI_DEASSERT();
        sd_spi_xfer(0xFF);
    }
    return SD_ERR_OKAY;
}

sd_error_t sd_spi_sync(void) {
    if (!sd_spi_state.inited) {
        return SD_ERR_NO_INIT;
    }
    if (!sd_spi_state.card_present) {
        return SD_ERR_NO_DISK;
    }
    SD_SPI_ASSERT();
    bool ready = sd_spi_wait_ready(); 
    SD_SPI_DEASSERT();
    return ready ? SD_ERR_OKAY : SD_ERR_TIMEOUT;
}

#include "log/log.h"

sd_error_t sd_spi_get_block_count(uint32_t* count) {
    if (!count) {
        return SD_ERR_INVALID_ARG;
    }
    if (!sd_spi_state.inited) {
        return SD_ERR_NO_INIT;
    }
    if (!sd_spi_state.card_present) {
        return SD_ERR_NO_DISK;
    }
    // Send CMD9 to get the CSD register
    SD_SPI_ASSERT();
    uint8_t resp = sd_spi_send_cmd(CMD9_SEND_CSD, 0, 0xFF); // CMD9: SEND_CSD
    if (resp != 0x00) {
        SD_SPI_DEASSERT();
        ogxm_loge("Failed to read CSD register: %02X", resp);
        return SD_ERR_READ_WRITE;
    }

    // *** Wait for data token (0xFE) ***
    int tries = 10000;
    uint8_t token;
    do {
        token = sd_spi_xfer(0xFF);
    } while (token != 0xFE && --tries);
    if (tries <= 0) {
        SD_SPI_DEASSERT();
        ogxm_loge("Timeout waiting for CSD token\n");
        return SD_ERR_TIMEOUT;
    }

    // Read the CSD register (16 bytes)
    uint8_t csd[16];
    for (int i = 0; i < 16; i++) {
        csd[i] = sd_spi_xfer(0xFF);
    }
    sd_spi_xfer(0xFF); // CRC (not used, can be 0xFF)
    sd_spi_xfer(0xFF); // CRC (not used, can be 0xFF)
    SD_SPI_DEASSERT();

    uint8_t csd_structure = (csd[0] >> 6) & 0x3;
    if (csd_structure == 1) {
        // CSD version 2.0 (SDHC/SDXC)
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) |
                          ((uint32_t)csd[8] << 8) |
                          csd[9];
        *count = (c_size + 1) * 1024;
        ogxm_logd("CSD structure: %u, C_SIZE: %u, Block count: %u\n", csd_structure, c_size, *count);
        return SD_ERR_OKAY;
    } else if (csd_structure == 0) {
        // CSD version 1.0 (SDSC)
        uint32_t c_size = ((uint32_t)(csd[6] & 0x03) << 10) |
                          ((uint32_t)csd[7] << 2) |
                          ((csd[8] & 0xC0) >> 6);
        uint8_t c_size_mult = ((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7);
        uint8_t read_bl_len = csd[5] & 0x0F;
        uint32_t block_len = 1UL << read_bl_len;
        uint32_t mult = 1UL << (c_size_mult + 2);
        uint32_t blocknr = (c_size + 1) * mult;
        *count = (blocknr * block_len) / 512;
        ogxm_logd("CSD structure: %u, C_SIZE: %u, C_SIZE_MULT: %u, READ_BL_LEN: %u, Block count: %u\n",
                  csd_structure, c_size, c_size_mult, read_bl_len, *count);
        return SD_ERR_OKAY;
    } else {
        ogxm_loge("Unknown CSD structure: %u\n", csd_structure);
        return SD_ERR_READ_WRITE;
    }
}

#endif // SD_CARD_ENABLED