#ifndef _WIIMOTE_BTSTACK_H_
#define _WIIMOTE_BTSTACK_H_

void wiimote_emulator_set_led(void (*led_on)(), void (*led_off)());

void wiimote_emulator(void *report);

#endif // _WIIMOTE_BTSTACK_H_
