#include <stdlib.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pio_usb.h"
#include "tusb.h"

#include "usbh/tusb_host.h"
#include "usbh/tusb_host_manager.h"
#include "board_config.h"

#define PIO_USB_CONFIG { PIO_USB_DP_PIN, PIO_USB_TX_DEFAULT, PIO_SM_USB_TX_DEFAULT, PIO_USB_DMA_TX_DEFAULT, PIO_USB_RX_DEFAULT, PIO_SM_USB_RX_DEFAULT, PIO_SM_USB_EOP_DEFAULT, NULL, PIO_USB_DEBUG_PIN_NONE, PIO_USB_DEBUG_PIN_NONE, false, PIO_USB_PINOUT_DPDM }

void board_setup() 
{
    #if (BOARD_ADA_FEATHER_USBH >= 1)
        // #define NEOPIXEL_PWR_PIN    20
        // #define NEOPIXEL_CTRL_PIN   21 
        #define VCC_EN_PIN 18

        gpio_init(VCC_EN_PIN);
        gpio_set_dir(VCC_EN_PIN, GPIO_OUT);
        gpio_put(VCC_EN_PIN, 1);
    #endif

    gpio_init(LED_INDICATOR_PIN);
    gpio_set_dir(LED_INDICATOR_PIN, GPIO_OUT);
    gpio_put(LED_INDICATOR_PIN, 0);    
}

void usbh_main()
{
    pio_usb_configuration_t pio_cfg = PIO_USB_CONFIG;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    board_setup();

    tuh_init(1);

    while (true)
    {
        tuh_task();
        send_fb_data_to_gamepad();
    }
}