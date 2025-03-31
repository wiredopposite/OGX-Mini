#include "Board/Config.h"
#if defined(CONFIG_EN_USB_HOST)

#include <atomic>
#include <hardware/gpio.h>

#include "Board/board_api_private/board_api_private.h"

namespace board_api_usbh {

std::atomic<bool> host_connected_ = false;

void host_pin_isr(uint gpio, uint32_t events) {
    gpio_set_irq_enabled(PIO_USB_DP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    gpio_set_irq_enabled(PIO_USB_DP_PIN + 1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

    if (gpio == PIO_USB_DP_PIN || gpio == PIO_USB_DP_PIN + 1) {
        uint32_t dp_state = gpio_get(PIO_USB_DP_PIN);
        uint32_t dm_state = gpio_get(PIO_USB_DP_PIN + 1);

        if (dp_state || dm_state) {
            host_connected_.store(true);
        } else {
            host_connected_.store(false);
            gpio_set_irq_enabled(PIO_USB_DP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
            gpio_set_irq_enabled(PIO_USB_DP_PIN + 1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
        }
    }
}

bool host_connected() {
    return host_connected_.load();
}

void init() {
#if defined(VCC_EN_PIN)
    gpio_init(VCC_EN_PIN);
    gpio_set_dir(VCC_EN_PIN, GPIO_OUT);
    gpio_put(VCC_EN_PIN, 1);
#endif 

    gpio_init(PIO_USB_DP_PIN);
    gpio_set_dir(PIO_USB_DP_PIN, GPIO_IN);
    gpio_pull_down(PIO_USB_DP_PIN);

    gpio_init(PIO_USB_DP_PIN + 1);
    gpio_set_dir(PIO_USB_DP_PIN + 1, GPIO_IN);
    gpio_pull_down(PIO_USB_DP_PIN + 1);

    if (gpio_get(PIO_USB_DP_PIN) || gpio_get(PIO_USB_DP_PIN + 1)) {
        host_connected_.store(true);
    } else {
        gpio_set_irq_enabled_with_callback(PIO_USB_DP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &host_pin_isr);
        gpio_set_irq_enabled_with_callback(PIO_USB_DP_PIN + 1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &host_pin_isr);
    }
}

} // namespace board_api_usbh

#endif // defined(CONFIG_EN_USB_HOST)