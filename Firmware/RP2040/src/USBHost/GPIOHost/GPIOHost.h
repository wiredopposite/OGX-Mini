#ifndef USBHOST_GPIOHOST_GPIOHOST_H_
#define USBHOST_GPIOHOST_GPIOHOST_H_

class Gamepad;

namespace GPIOHost {

/** Initialize PSX host. CMD pin = gpio_cmd (CLK=+1, ATT=+2), DATA=gpio_data. Call when input source is PSX_GPIO. */
void psx_host_init(unsigned pio_index, unsigned gpio_cmd, unsigned gpio_data);

/** Poll PSX controller and update gamepad if new data. Call from main loop when input is PSX_GPIO. */
void psx_host_poll(Gamepad& gamepad);

/** Initialize GameCube JoyBus host. data_pin = single wire. Call when input source is GAMECUBE_GPIO. */
void joybus_host_init(unsigned pio_index, unsigned data_pin);

/** Poll GameCube controller and update gamepad. Call from main loop when input is GAMECUBE_GPIO. */
void joybus_host_poll(Gamepad& gamepad);

/** Dreamcast Maple Bus host: init (stub until DreamPicoPort is ported). Pin A = first of two consecutive GPIOs. */
void dreamcast_host_init(unsigned pio_in_index, unsigned pio_out_index, unsigned pin_a, int dir_pin);
/** Poll Dreamcast controller and update gamepad (stub until port). */
void dreamcast_host_poll(Gamepad& gamepad);

/** Initialize N64 controller host (1-wire data pin). Call when input source is N64_GPIO. */
void n64_host_init(unsigned pio_index, unsigned data_pin);
/** Poll N64 controller and update gamepad. Call from main loop when input is N64_GPIO. */
void n64_host_poll(Gamepad& gamepad);

}  // namespace GPIOHost

#endif /* USBHOST_GPIOHOST_GPIOHOST_H_ */
