#include "Board/Config.h"
#if defined(CONFIG_EN_USB_HOST)

#include <atomic>
#include <hardware/gpio.h>

#include "Board/board_api_private/board_api_private.h"

namespace board_api_usbh {

std::atomic<bool> host_connected_ = false;

void host_pin_isr(uint gpio, uint32_t events) {
    (void)gpio;
    (void)events;
    // Do not leave IRQs disabled while a device is attached — that prevented unplug edges from
    // ever being seen and could strand host_connected_ out of sync with the pins.
    const uint32_t dp_state = gpio_get(PIO_USB_DP_PIN);
    const uint32_t dm_state = gpio_get(PIO_USB_DP_PIN + 1);
    host_connected_.store((dp_state != 0u) || (dm_state != 0u));
}

bool host_connected() {
    return host_connected_.load();
}

void suspend_line_irq() {
    gpio_set_irq_enabled(PIO_USB_DP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    gpio_set_irq_enabled(PIO_USB_DP_PIN + 1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
}

void store_host_line_connected(bool connected) {
    host_connected_.store(connected);
}

void enable_host_line_irq_monitoring() {
    gpio_set_irq_enabled_with_callback(PIO_USB_DP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &host_pin_isr);
    gpio_set_irq_enabled_with_callback(PIO_USB_DP_PIN + 1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &host_pin_isr);
    host_pin_isr(0, 0);
}

void stop_pio_usb_host() __attribute__((weak));
void stop_pio_usb_host() {}

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

#else // !defined(CONFIG_EN_USB_HOST)

namespace board_api_usbh {
void suspend_line_irq() {}
void store_host_line_connected(bool) {}
void enable_host_line_irq_monitoring() {}
} // namespace board_api_usbh

#endif // defined(CONFIG_EN_USB_HOST)