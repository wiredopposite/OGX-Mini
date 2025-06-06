#include "sdcard/sdcard.h"
#if SD_CARD_ENABLED

#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <pico/sync.h>

// #ifndef SDCARD_SPI
// #define SDCARD_SPI              0
// #endif

// #ifndef SDCARD_SPI_PIN_SCK
// #define SDCARD_SPI_PIN_SCK      2U
// #endif

// #ifndef SDCARD_SPI_PIN_MOSI
// #define SDCARD_SPI_PIN_MOSI     3U
// #endif

// #ifndef SDCARD_SPI_PIN_MISO
// #define SDCARD_SPI_PIN_MISO     4U
// #endif

// #ifndef SDCARD_SPI_PIN_CS
// #define SDCARD_SPI_PIN_CS       5U
// #endif

#define SDCARD_SPI_BAUDRATE     1000U*1000U // 1 MHz

#define SDCARD_ERR              0xFFU

static spi_inst_t* spi = __CONCAT(spi, SDCARD_SPI_NUM);
static volatile bool spi_inited = false;
static bool connected = false;
static mutex_t sd_mutex;

static void sd_xfer_begin(void) {
    if (!mutex_is_initialized(&sd_mutex)) {
        mutex_init(&sd_mutex);
    }
    mutex_enter_blocking(&sd_mutex);
    gpio_put(SDCARD_SPI_PIN_CS, 0);
}

static void sdcard_xfer_complete(void) {
    gpio_put(SDCARD_SPI_PIN_CS, 1);
    if (mutex_is_initialized(&sd_mutex)) {
        mutex_exit(&sd_mutex);
    }
}

static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    uint8_t buf[6];
    buf[0] = 0x40 | cmd;
    buf[1] = arg >> 24;
    buf[2] = arg >> 16;
    buf[3] = arg >> 8;
    buf[4] = arg;
    buf[5] = crc;
    // sdcard_spi_send(buf, 6);
    // Wait for response (response is always MSB=0)
    for (int i = 0; i < 8; ++i) {
        // uint8_t resp = sdcard_spi_xfer(0xFF);
        // if (!(resp & 0x80)) {
        //     return resp;
        // }
    }
    return SDCARD_ERR;
}

bool sd_init(void) {
    if (spi_inited) {
        return true;
    }
    mutex_init(&sd_mutex);

    spi_init(spi, SDCARD_SPI_BAUDRATE);
    spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    gpio_set_function(SDCARD_SPI_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_SPI_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_SPI_PIN_MISO, GPIO_FUNC_SPI);
    gpio_init(SDCARD_SPI_PIN_CS);
    gpio_set_dir(SDCARD_SPI_PIN_CS, GPIO_OUT);
    gpio_put(SDCARD_SPI_PIN_CS, 1);
    spi_inited = true;
    return true;
}

bool sd_present(void) {
    if (!spi_inited) {
        spi_inited = sd_init();
    }
    sd_xfer_begin();
    uint8_t response = 0;
    int result = spi_read_blocking(spi, 0xFF, &response, 1);
    sd_xfer_complete();
    
    connected = ((result == 1) && (response == 0xFF));
    return connected;
}

#endif // SD_CARD_ENABLED