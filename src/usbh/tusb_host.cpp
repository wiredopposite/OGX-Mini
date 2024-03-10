#include <stdlib.h>
#include <stdarg.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "pio_usb.h"
#include "tusb.h"

#include "tusb_host.h"
#include "tusb_host_manager.h"

// TODO figure out something to do with the leds
// tuh_mounted is always true after unplugging a gamepad, idk what's up with that

#ifdef FEATHER_RP2040
    #define PIO_USB_DP_PIN  16 // DM = 17
    #define PIN_5V_EN       18
    // #define RED_LED_PIN         13
    // #define NEOPIXEL_PWR_PIN    20
    // #define NEOPIXEL_CTRL_PIN   21 

    void board_setup() 
    {
        gpio_init(PIN_5V_EN);
        gpio_set_dir(PIN_5V_EN, GPIO_OUT);
        gpio_put(PIN_5V_EN, 1);        
    }
    
#else
    #define PIO_USB_DP_PIN  0 // DM = 1
    // #define PICO_LED_PIN    25

    void board_setup() 
    {
        // gpio_init(PICO_LED_PIN);
        // gpio_set_dir(PICO_LED_PIN, GPIO_OUT);
        // gpio_put(PICO_LED_PIN, 1);
    }

#endif

#define PIO_USB_CONFIG { PIO_USB_DP_PIN, PIO_USB_TX_DEFAULT, PIO_SM_USB_TX_DEFAULT, PIO_USB_DMA_TX_DEFAULT, PIO_USB_RX_DEFAULT, PIO_SM_USB_RX_DEFAULT, PIO_SM_USB_EOP_DEFAULT, NULL, PIO_USB_DEBUG_PIN_NONE, PIO_USB_DEBUG_PIN_NONE, false, PIO_USB_PINOUT_DPDM }

void usbh_main()
{
    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    board_setup();

    tuh_init(1);

    uint32_t fb_sent_time = to_ms_since_boot(get_absolute_time());
    const uint32_t fb_interval = 100;

    while (true)
    {
        tuh_task();

        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        if (current_time - fb_sent_time >= fb_interval) 
        {
            send_fb_data_to_gamepad();
            fb_sent_time = current_time;
        }
    }
}