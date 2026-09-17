/* GameCube JoyBus device. Ported from PicoGamepadConverter (Loc15). */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "joybus.pio.h"
#include "USBDevice/DeviceDriver/GameCube/gc_simulator.h"

void __time_critical_func(convertToPio)(const uint8_t* command, const int len, uint32_t* result, int* resultLen) {
    if (len == 0) {
        *resultLen = 0;
        return;
    }
    *resultLen = len / 2 + 1;
    for (int i = 0; i < *resultLen; i++)
        result[i] = 0;
    for (int i = 0; i < len; i++) {
        for (int j = 0; j < 8; j++) {
            result[i / 2] += 1u << (2 * (8 * (i % 2) + j) + 1);
            result[i / 2] += (!!(command[i] & (0x80u >> j))) << (2 * (8 * (i % 2) + j));
        }
    }
    result[len / 2] += 3u << (2 * (8 * (len % 2)));
}

#define ORG 127

void gc_device_main(unsigned pio, GCReport* data, int data_pin) {
    volatile GCReport* gcReport = data;
    PIO gamecube_pio = pio ? pio1 : pio0;

    gpio_init(data_pin);
    gpio_set_dir(data_pin, GPIO_IN);
    gpio_pull_up(data_pin);

    sleep_us(100);

    uint offset = pio_add_program(gamecube_pio, &joybus_program);
    uint sm = pio_claim_unused_sm(gamecube_pio, true);
    pio_sm_config config = joybus_program_get_default_config(offset);
    pio_gpio_init(gamecube_pio, data_pin);
    sm_config_set_in_pins(&config, data_pin);
    sm_config_set_out_pins(&config, data_pin, 1);
    sm_config_set_set_pins(&config, data_pin, 1);
    float div = (float)clock_get_hz(clk_sys) / (25e6f);
    sm_config_set_clkdiv(&config, div);
    sm_config_set_out_shift(&config, true, false, 32);
    sm_config_set_in_shift(&config, false, true, 8);

    pio_sm_init(gamecube_pio, sm, offset, &config);
    pio_sm_set_enabled(gamecube_pio, sm, true);

    while (true) {
        uint8_t buffer[3];
        buffer[0] = (uint8_t)pio_sm_get_blocking(gamecube_pio, sm);
        if (buffer[0] == 0) {
            uint8_t probeResponse[3] = { 0x09, 0x00, 0x03 };
            uint32_t result[2];
            int resultLen;
            convertToPio(probeResponse, 3, result, &resultLen);
            sleep_us(6);

            pio_sm_set_enabled(gamecube_pio, sm, false);
            pio_sm_init(gamecube_pio, sm, offset + joybus_offset_outmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);
            for (int i = 0; i < resultLen; i++)
                pio_sm_put_blocking(gamecube_pio, sm, result[i]);
        } else if (buffer[0] == 0x41) {
            uint8_t originResponse[10] = { 0x00, 0x80, ORG, ORG, ORG, ORG, 0, 0, 0, 0 };
            uint32_t result[6];
            int resultLen;
            convertToPio(originResponse, 10, result, &resultLen);

            pio_sm_set_enabled(gamecube_pio, sm, false);
            pio_sm_init(gamecube_pio, sm, offset + joybus_offset_outmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);
            for (int i = 0; i < resultLen; i++)
                pio_sm_put_blocking(gamecube_pio, sm, result[i]);
        } else if (buffer[0] == 0x40) {
            buffer[0] = (uint8_t)pio_sm_get_blocking(gamecube_pio, sm);

            uint32_t result[5];
            int resultLen;
            convertToPio((const uint8_t*)gcReport, 8, result, &resultLen);

            (void)pio_sm_get_blocking(gamecube_pio, sm);
            sleep_us(7);

            pio_sm_set_enabled(gamecube_pio, sm, false);
            pio_sm_init(gamecube_pio, sm, offset + joybus_offset_outmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);
            for (int i = 0; i < resultLen; i++)
                pio_sm_put_blocking(gamecube_pio, sm, result[i]);
        } else {
            pio_sm_set_enabled(gamecube_pio, sm, false);
            sleep_us(400);
            pio_sm_init(gamecube_pio, sm, offset + joybus_offset_inmode, &config);
            pio_sm_set_enabled(gamecube_pio, sm, true);
        }
    }
}
